#include <3ds.h>
#include "soundengine.h"
#include "gameboy.h"
#include "menu.h"
#include "gb_noise.h"
#include "console.h"
#include <math.h>
#include <time.h>

#define FRAMES_PER_BUFFER 8
#define AUDIO_BUFFER_COUNT 3
#define CSND_BUFFER_COUNT 2
#define AUDIO_BUFFER_ALIGNMENT 0x80

#define CYCLES_UNTIL_SAMPLE (0x54)
#define AUDIO_FREQUENCY (CYCLES_PER_FRAME * 59.7 / CYCLES_UNTIL_SAMPLE)
#define SAMPLES_PER_FRAME ((CYCLES_PER_FRAME + CYCLES_UNTIL_SAMPLE - 1) / CYCLES_UNTIL_SAMPLE)
#define AUDIO_BUFFER_SIZE ((SAMPLES_PER_FRAME + 1) * FRAMES_PER_BUFFER)
#define AUDIO_BUFFER_BYTES (AUDIO_BUFFER_SIZE * sizeof(s16))
#define AUDIO_BUFFER_STRIDE ((AUDIO_BUFFER_BYTES + AUDIO_BUFFER_ALIGNMENT - 1) & ~(AUDIO_BUFFER_ALIGNMENT - 1))
#define AUDIO_ALLOCATION_SIZE (AUDIO_BUFFER_COUNT * AUDIO_BUFFER_STRIDE)
#define AUDIO_CHANNEL 0

enum AudioBackend {
    AUDIO_BACKEND_NONE = 0,
    AUDIO_BACKEND_NDSP,
    AUDIO_BACKEND_CSND
};

bool audioInitialized = false;
bool firstAudioBufferLogged = false;
AudioBackend audioBackend = AUDIO_BACKEND_NONE;
s16* bufferDat = NULL;
s16* buffers[AUDIO_BUFFER_COUNT] = {NULL, NULL, NULL};
ndspWaveBuf waveBuffers[AUDIO_BUFFER_COUNT];
int csndBufferChannels[AUDIO_BUFFER_COUNT] = {-1, -1, -1};
int activeAudioBufferCount = AUDIO_BUFFER_COUNT;
Result csndInitResult = -1;
Result ndspInitResult = -1;
Result lastAudioQueueResult = 0;
u32 csndChannelMask = 0;
u32 queuedAudioBuffers = 0;
u32 nonZeroAudioSamples = 0;

int recordingBuffer = 0;
int recordingPos = 0;
int framecnt;

bool chanEnabled[4] = {true, true, true, true};

namespace {

Result queueCsndBuffer(int channel, const s16* data, u32 byteCount) {
    if (channel < 0 || !(csndChannelMask & BIT(channel)) || !data || !byteCount)
        return 1;

    // csndPlaySound() ends in csndExecCmds(true), whose implementation busy
    // waits without a timeout. A lost CSND completion therefore freezes the
    // entire emulator (video, audio and input). Submit the equivalent channel
    // command asynchronously so the application loop always remains alive.
    u32 timer = CSND_TIMER((u32)AUDIO_FREQUENCY);
    if (timer < 0x0042)
        timer = 0x0042;
    else if (timer > 0xffff)
        timer = 0xffff;

    u32 flags = SOUND_ONE_SHOT | SOUND_FORMAT_16BIT |
        SOUND_LINEAR_INTERP;
    flags &= ~0xffff001f;
    flags |= SOUND_ENABLE | SOUND_CHANNEL(channel) | (timer << 16);
    const u32 volumes = CSND_VOL(1.0f, 0.0f);
    CSND_SetChnRegs(flags, osConvertVirtToPhys(data), 0, byteCount,
        volumes, volumes);
    return csndExecCmds(false);
}

void stopCsndChannels() {
    for (int i = 0; i < activeAudioBufferCount; ++i) {
        if (csndBufferChannels[i] >= 0)
            CSND_SetPlayStateR(csndBufferChannels[i], 0);
    }
    // Never use the unbounded wait mode here either. The buffers remain
    // allocated until after csndExit(), so asynchronous completion is safe.
    csndExecCmds(false);
}

} // namespace


void audioInit() {
    if (audioInitialized)
        return;

    bufferDat = (s16*)linearAlloc(
        AUDIO_ALLOCATION_SIZE);
    if (!bufferDat)
        return;

    // Prefer the original hardware CSND path for 3DSX. It does not depend on
    // an extracted dspfirm.cdc and was the backend used by the native port.
    csndInitResult = csndInit();
    if (R_SUCCEEDED(csndInitResult)) {
        csndChannelMask = csndChannels;
        int count = 0;
        for (int channel = 8;
                channel < CSND_NUM_CHANNELS && count < CSND_BUFFER_COUNT;
                ++channel) {
            if (csndChannelMask & BIT(channel))
                csndBufferChannels[count++] = channel;
        }
        if (count >= 2) {
            activeAudioBufferCount = CSND_BUFFER_COUNT;
            audioBackend = AUDIO_BACKEND_CSND;
        }
        else {
            csndExit();
            csndInitResult = 1;
        }
    }

    if (audioBackend == AUDIO_BACKEND_NONE) {
        // NDSP remains available for environments that expose dsp::DSP and
        // have sdmc:/3ds/dspfirm.cdc (or an emulator HLE equivalent).
        ndspInitResult = ndspInit();
        if (R_SUCCEEDED(ndspInitResult)) {
            activeAudioBufferCount = AUDIO_BUFFER_COUNT;
            audioBackend = AUDIO_BACKEND_NDSP;
        }
    }

    if (audioBackend == AUDIO_BACKEND_NONE) {
        linearFree(bufferDat);
        bufferDat = NULL;
        return;
    }

    audioInitialized = true;
    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++)
        buffers[i] = reinterpret_cast<s16*>(
            reinterpret_cast<u8*>(bufferDat) + i * AUDIO_BUFFER_STRIDE);
    memset(bufferDat, 0, AUDIO_ALLOCATION_SIZE);
    memset(waveBuffers, 0, sizeof(waveBuffers));

    if (audioBackend == AUDIO_BACKEND_NDSP) {
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspChnReset(AUDIO_CHANNEL);
        ndspChnSetInterp(AUDIO_CHANNEL, NDSP_INTERP_LINEAR);
        ndspChnSetRate(AUDIO_CHANNEL, AUDIO_FREQUENCY);
        ndspChnSetFormat(AUDIO_CHANNEL, NDSP_FORMAT_MONO_PCM16);
        float mix[12] = {0};
        mix[0] = 1.0f;
        mix[1] = 1.0f;
        ndspChnSetMix(AUDIO_CHANNEL, mix);
        printLog("3DS audio: NDSP fallback ready at %.0f Hz\n",
            AUDIO_FREQUENCY);
    }
    else {
        printLog("3DS audio: CSND ready at %.0f Hz (%d buffers)\n",
            AUDIO_FREQUENCY, activeAudioBufferCount);
    }
}

void audioExit() {
    if (audioBackend == AUDIO_BACKEND_NDSP) {
        ndspChnWaveBufClear(AUDIO_CHANNEL);
        ndspExit();
    }
    else if (audioBackend == AUDIO_BACKEND_CSND) {
        stopCsndChannels();
        csndExit();
    }
    audioBackend = AUDIO_BACKEND_NONE;
    audioInitialized = false;
    if (bufferDat) {
        linearFree(bufferDat);
        bufferDat = NULL;
    }
    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++)
        buffers[i] = NULL;
}

void initSampler() {
    if (!audioInitialized)
        return;
    if (audioBackend == AUDIO_BACKEND_NDSP)
        ndspChnWaveBufClear(AUDIO_CHANNEL);
    else
        stopCsndChannels();
    memset(bufferDat, 0, AUDIO_ALLOCATION_SIZE);
    memset(waveBuffers, 0, sizeof(waveBuffers));
    recordingBuffer = 0;
    recordingPos = 0;
    framecnt = 0;
    firstAudioBufferLogged = false;
    lastAudioQueueResult = 0;
    queuedAudioBuffers = 0;
    nonZeroAudioSamples = 0;
    printLog("3DS audio backend: %s at %.0f Hz\n",
        audioBackend == AUDIO_BACKEND_NDSP ? "NDSP" : "CSND",
        AUDIO_FREQUENCY);
}

// Called once every 4 cycles
void addSample(s16 sample) {
    if (!audioInitialized || recordingPos >= AUDIO_BUFFER_SIZE)
        return;
    buffers[recordingBuffer][recordingPos++] = sample;
    if (sample != 0)
        nonZeroAudioSamples++;
}

void swapBuffers() {
    if (!audioInitialized)
        return;

    framecnt++;
    if (framecnt < FRAMES_PER_BUFFER && recordingPos < AUDIO_BUFFER_SIZE)
        return;

    if (recordingPos <= 0)
        return;
    const int sampleCount = recordingPos;

    if (audioBackend == AUDIO_BACKEND_NDSP) {
        ndspWaveBuf* wave = &waveBuffers[recordingBuffer];
        if (wave->status == NDSP_WBUF_QUEUED ||
                wave->status == NDSP_WBUF_PLAYING)
            return;
        memset(wave, 0, sizeof(*wave));
        wave->data_pcm16 = buffers[recordingBuffer];
        wave->nsamples = sampleCount;
        DSP_FlushDataCache(wave->data_pcm16,
            sampleCount * sizeof(s16));
        ndspChnWaveBufAdd(AUDIO_CHANNEL, wave);
        lastAudioQueueResult = 0;
    }
    else {
        const u32 byteCount = sampleCount * sizeof(s16);
        GSPGPU_FlushDataCache(buffers[recordingBuffer], byteCount);
        const int channel = csndBufferChannels[recordingBuffer];
        lastAudioQueueResult = queueCsndBuffer(channel,
            buffers[recordingBuffer], byteCount);
        if (lastAudioQueueResult != 0) {
            printLog("3DS audio: CSND queue failed (%08lX)\n",
                (unsigned long)lastAudioQueueResult);
            return;
        }
    }
    queuedAudioBuffers++;
    if (!firstAudioBufferLogged) {
        printLog("3DS audio: queued %u PCM16 samples\n",
            (unsigned)sampleCount);
        firstAudioBufferLogged = true;
    }

    recordingBuffer = (recordingBuffer + 1) % activeAudioBufferCount;
    recordingPos = 0;
    framecnt = 0;
}

void printAudioInfo() {
    clearConsole();
    printf("3DS Audio Status\n\n");
    const char* backend = "Unavailable";
    if (audioBackend == AUDIO_BACKEND_CSND)
        backend = "CSND (hardware primary)";
    else if (audioBackend == AUDIO_BACKEND_NDSP)
        backend = "NDSP (fallback)";
    printf("Backend: %s\n", backend);
    printf("Initialized: %s\n", audioInitialized ? "yes" : "no");
    printf("Rate: %.0f Hz\n", AUDIO_FREQUENCY);
    printf("Buffers: %d\n", activeAudioBufferCount);
    printf("Queued: %lu\n", (unsigned long)queuedAudioBuffers);
    printf("Non-zero samples: %lu\n", (unsigned long)nonZeroAudioSamples);
    printf("Last queue: %08lX\n", (unsigned long)lastAudioQueueResult);
    printf("CSND init: %08lX\n", (unsigned long)csndInitResult);
    printf("CSND channels: %08lX\n", (unsigned long)csndChannelMask);
    printf("NDSP init: %08lX\n", (unsigned long)ndspInitResult);
}


SoundEngine::SoundEngine(Gameboy* g)
{
    setGameboy(g);
}

SoundEngine::~SoundEngine() {

}

void SoundEngine::setGameboy(Gameboy* g) {
    gameboy = g;
}

void SoundEngine::init() {
    for (int i=0; i<4; i++) {
        chanPolarity[i] = 1;
        chanPolarityCounter[i] = 0;
    }
    cyclesUntilSample = CYCLES_UNTIL_SAMPLE;
    lfsr = gbNoiseReset();
    chan3WavPos = 0;

    initSampler();

    refresh();
}

void SoundEngine::refresh() {
    // Ordering note: Writing a byte to FF26 with bit 7 set enables writes to
    // the other registers. With bit 7 unset, writes are ignored.
    handleSoundRegister(0x26, gameboy->readIO(0x26));

    for (int i=0x10; i<=0x3F; i++) {
        if (i == 0x14 || i == 0x19 || i == 0x1e || i == 0x23)
            // Don't restart the sound channels.
            handleSoundRegister(i, gameboy->readIO(i)&~0x80);
        else
            handleSoundRegister(i, gameboy->readIO(i));
    }

    if (gameboy->readIO(0x26) & 1)
        handleSoundRegister(0x14, gameboy->readIO(0x14)|0x80);
    if (gameboy->readIO(0x26) & 2)
        handleSoundRegister(0x19, gameboy->readIO(0x19)|0x80);
    if (gameboy->readIO(0x26) & 4)
        handleSoundRegister(0x1e, gameboy->readIO(0x1e)|0x80);
    if (gameboy->readIO(0x26) & 8)
        handleSoundRegister(0x23, gameboy->readIO(0x23)|0x80);

    unmute();
}

void SoundEngine::mute() {
    muted = true;
}

void SoundEngine::unmute() {
    muted = false;
}


void SoundEngine::updateSound(int cycles)
{
    if (soundDisabled)
        return;

//	chanOn[0] = 0;
//	chanOn[1] = 0;
//	chanOn[2] = 0;
//	chanOn[3] = 0;

	if (chan1SweepTime != 0)
	{
		chan1SweepCounter -= cycles;
		while (chan1SweepCounter <= 0)
		{
			chan1SweepCounter = (clockSpeed/(128/chan1SweepTime))+chan1SweepCounter;
			chanFreq[0] += (chanFreq[0]>>chan1SweepAmount)*chan1SweepDir;

			if (chanFreq[0] > 0x7FF)
			{
				chanOn[0] = 0;
				gameboy->clearSoundChannel(CHAN_1);
			}
		}
        if (chanOn[0])
            setSoundEventCycles(chan1SweepCounter);
	}
	for (int i=0; i<2; i++)
	{
		if (chanOn[i])
		{
			if (chanEnvSweep[i] != 0)
			{
				chanEnvCounter[i] -= cycles;
				if (chanEnvCounter[i] <= 0) {
					chanEnvCounter[i] = chanEnvSweep[i]*clockSpeed/64;
					chanVol[i] += chanEnvDir[i];
					if (chanVol[i] < 0)
						chanVol[i] = 0;
					if (chanVol[i] > 0xF)
						chanVol[i] = 0xF;
				}
                setSoundEventCycles(chanEnvCounter[i]);
			}

			if (chanUseLen[i])
			{
				chanLenCounter[i] -= cycles;
				if (chanLenCounter[i] <= 0) {
					chanOn[i] = 0;
					if (i==0)
						gameboy->clearSoundChannel(CHAN_1);
					else
						gameboy->clearSoundChannel(CHAN_2);
				}
                else
                    setSoundEventCycles(chanLenCounter[i]);
			}
		}

	}

	// Channel 3
	if (chanOn[2])
	{
		if (chanUseLen[2])
		{
			chanLenCounter[2] -= cycles;
			if (chanLenCounter[2] <= 0) {
				chanOn[2] = 0;
				gameboy->clearSoundChannel(CHAN_3);
			}
            else
                setSoundEventCycles(chanLenCounter[2]);
		}
	}
	if (chanOn[3])
	{
		chanEnvCounter[3] -= cycles;
		if (chanEnvSweep[3] != 0) {
            if (chanEnvCounter[3] <= 0) {
                chanEnvCounter[3] = chanEnvSweep[3]*clockSpeed/64;
                chanVol[3] += chanEnvDir[3];
                if (chanVol[3] < 0)
                    chanVol[3] = 0;
                if (chanVol[3] > 0xF)
                    chanVol[3] = 0xF;
            }
            setSoundEventCycles(chanEnvCounter[3]);
		}

		if (chanUseLen[3]) {
			chanLenCounter[3] -= cycles;
			if (chanLenCounter[3] <= 0) {
				chanOn[3] = 0;
				gameboy->clearSoundChannel(CHAN_4);
			}
            else
                setSoundEventCycles(chanLenCounter[3]);
		}
	}

    if (!audioInitialized)
        return;

    cyclesUntilSample -= cycles;
    while (cyclesUntilSample <= 0) {
        int c = CYCLES_UNTIL_SAMPLE;
        cyclesUntilSample += c;

        s16 tone = 0, tone1 = 0, tone2 = 0;

        for (int j=0; j<2; j++) {
            if (!chanEnabled[j])
                continue;
            if (chanOn[j]) {
                chanPolarityCounter[j] -= c;
                while (chanPolarityCounter[j] <= 0)
                {
                    int oldPolarityCounter = chanPolarityCounter[j];
                    chanPolarity[j] *= -1;

                    if (chanDuty[j] == 0)
                    {
                        if (chanPolarity[j] == 1)
                            chanPolarityCounter[j] = clockSpeed/((double)131072/(2048-chanFreq[j]))*(((double)1/8));
                        else
                            chanPolarityCounter[j] = clockSpeed/((double)131072/(2048-chanFreq[j]))*(((double)7/8));
                    }
                    else if (chanDuty[j] == 1)
                    {
                        if (chanPolarity[j] == 1)
                            chanPolarityCounter[j] = clockSpeed/((double)131072/(2048-chanFreq[j]))*(((double)1/4));
                        else
                            chanPolarityCounter[j] = clockSpeed/((double)131072/(2048-chanFreq[j]))*(((double)3/4));
                    }
                    else if (chanDuty[j] == 2)
                        chanPolarityCounter[j] = clockSpeed/((double)131072/(2048-chanFreq[j]))/2;
                    else if (chanDuty[j] == 3)
                    {
                        if (chanPolarity[j] == 1)
                            chanPolarityCounter[j] = clockSpeed/((double)131072/(2048-chanFreq[j]))*(((double)3/4));
                        else
                            chanPolarityCounter[j] = clockSpeed/((double)131072/(2048-chanFreq[j]))*(((double)1/4));
                    }
                    chanPolarityCounter[j] += oldPolarityCounter;
                }

                if (chanToOut1[j])
                    tone1 += chanPolarity[j] * chanVol[j];
                if (chanToOut2[j])
                    tone2 += chanPolarity[j] * chanVol[j];
            }
        }

        if (chanEnabled[2] && chanOn[2]) {
            chanPolarityCounter[2] -= c;
            while (chanPolarityCounter[2] <= 0)
            {
                chanPolarityCounter[2] = clockSpeed/(65536/(2048-chanFreq[2])*32) + chanPolarityCounter[2];
                //chanPolarityCounter[2] = clockSpeed/((131072/(2048-chanFreq[2]))*16);
                chanFreqClocks[2] = chanPolarityCounter[2];

                chan3WavPos++;
                if (chan3WavPos >= 32)
                    chan3WavPos = 0;
            }

            static double analog[] = { -1, -0.8667, -0.7334, -0.6, -0.4668, -0.3335, -0.2, -0.067, 0.0664, 0.2, 0.333, 0.4668, 0.6, 0.7334, 0.8667, 1  } ;

            if (chanVol[2] >= 0)
            {
                int wavTone = gameboy->ioRam[0x30+(chan3WavPos/2)];
                wavTone = chan3WavPos%2? wavTone&0xF : wavTone>>4;
                if (chanToOut1[2])
                    tone1 += (analog[wavTone])*(0xF >> chanVol[2]);
                if (chanToOut2[2])
                    tone2 += (analog[wavTone])*(0xF >> chanVol[2]);
            }

        }
        if (chanEnabled[3] && chanOn[3]) {
            int polarityLen = clockSpeed/((int)(524288 / chan4FreqRatio) >> (chanFreq[3]+1));
            chanPolarityCounter[3] -= c;
            int flips = -(chanPolarityCounter[3] - polarityLen) / polarityLen;
            chanPolarityCounter[3] += flips*polarityLen;

            lfsr = gbNoiseAdvance(lfsr, chan4Width != 0, flips);
            chanPolarity[3] = gbNoisePolarity(lfsr);

            if (chanToOut1[3])
                tone1 += chanPolarity[3]*chanVol[3];
            if (chanToOut2[3])
                tone2 += chanPolarity[3]*chanVol[3];
        }

        tone1 *= SO1Vol;
        tone2 *= SO2Vol;

        tone = tone1 + tone2;

        addSample(tone*0x10);
    }
}


void SoundEngine::setSoundEventCycles(int cycles) {
    if (cyclesToSoundEvent > cycles) {
        cyclesToSoundEvent = cycles;
    }
}

void SoundEngine::soundUpdateVBlank() {
    if (soundDisabled)
        return;
    swapBuffers();
}

void SoundEngine::updateSoundSample() {
}


void SoundEngine::handleSoundRegister(u8 ioReg, u8 val)
{
	switch (ioReg)
	{
		// CHANNEL 1
		// Sweep
		case 0x10:
			//if (val&7 != 0)
			//	printf("sweep\n");
			chan1SweepTime = (val>>4)&0x7;
			if (chan1SweepTime != 0)
				chan1SweepCounter = clockSpeed/(128/chan1SweepTime);
			chan1SweepDir = (val&0x8) ? -1 : 1;
			chan1SweepAmount = (val&0x7);
			break;
		// Length / Duty
		case 0x11:
			chanLen[0] = val&0x3F;
			chanLenCounter[0] = (64-chanLen[0])*clockSpeed/256;
			chanDuty[0] = val>>6;
			break;
		// Envelope
		case 0x12:
			chanVol[0] = val>>4;
			if (val & 0x8)
				chanEnvDir[0] = 1;
			else
				chanEnvDir[0] = -1;
			chanEnvSweep[0] = val&0x7;
			break;
		// Frequency (low)
		case 0x13:
			chanFreq[0] &= 0x700;
			chanFreq[0] |= val;
			break;
		// Frequency (high)
		case 0x14:
			chanFreq[0] &= 0xFF;
			chanFreq[0] |= (val&0x7)<<8;
			if (val & 0x80)
			{
				chanLenCounter[0] = (64-chanLen[0])*clockSpeed/256;
				chanOn[0] = 1;
				chanVol[0] = gameboy->ioRam[0x12]>>4;
				if (chan1SweepTime != 0)
					chan1SweepCounter = clockSpeed/(128/chan1SweepTime);
				gameboy->setSoundChannel(CHAN_1);
			}
			if (val & 0x40)
				chanUseLen[0] = 1;
			else
				chanUseLen[0] = 0;
			break;
		// CHANNEL 2
		// Length / Duty
		case 0x16:
			chanLen[1] = val&0x3F;
			chanLenCounter[1] = (64-chanLen[1])*clockSpeed/256;
			chanDuty[1] = val>>6;
			break;
		// Envelope
		case 0x17:
			chanVol[1] = val>>4;
			if (val & 0x8)
				chanEnvDir[1] = 1;
			else
				chanEnvDir[1] = -1;
			chanEnvSweep[1] = val&0x7;
			break;
		// Frequency (low)
		case 0x18:
			chanFreq[1] &= 0x700;
			chanFreq[1] |= val;
			break;
		// Frequency (high)
		case 0x19:
			chanFreq[1] &= 0xFF;
			chanFreq[1] |= (val&0x7)<<8;
			if (val & 0x80)
			{
				chanLenCounter[1] = (64-chanLen[1])*clockSpeed/256;
				chanOn[1] = 1;
				chanVol[1] = gameboy->ioRam[0x17]>>4;
				gameboy->setSoundChannel(CHAN_2);
			}
			if (val & 0x40)
				chanUseLen[1] = 1;
			else
				chanUseLen[1] = 0;
			break;
		// CHANNEL 3
		// On/Off
		case 0x1A:
            if ((val & 0x80) == 0)
            {
                chanOn[2] = 0;
                gameboy->clearSoundChannel(CHAN_4);
                //buf.clear();
                //printf("chan3off\n");
            }
            else
            {
                //chanOn[2] = 1;
                //printf("chan3on?\n");
            }
			break;
		// Length
		case 0x1B:
			chanLen[2] = val;
			break;
		// Volume
		case 0x1C:
            if (chanVol[2] != ((val>>5)&3)) {
                chanVol[2] = (val>>5)&3;
                chanVol[2]--;
            }
            break;
		// Frequency (low)
		case 0x1D:
			chanFreq[2] &= 0xFF00;
			chanFreq[2] |= val;
			break;
		// Frequency (high)
		case 0x1E:
			chanFreq[2] &= 0xFF;
			chanFreq[2] |= (val&7)<<8;
			if ((val & 0x80) && (gameboy->ioRam[0x1A] & 0x80))
			{
				//buf.clear();
				chanOn[2] = 1;
				chanLenCounter[2] = (256-chanLen[2])*clockSpeed/256;
				gameboy->setSoundChannel(CHAN_3);
			}
			if (val & 0x40)
			{
				chanUseLen[2] = 1;
				//printf("useLen\n");
			}
			else
			{
				chanUseLen[2] = 0;
			}
			break;
		// CHANNEL 4
		// Length
		case 0x20:
			chanLen[3] = val&0x1F;
			break;
		// Volume
		case 0x21:
			chanVol[3] = val>>4;
			if (val & 0x8)
				chanEnvDir[3] = 1;
			else
				chanEnvDir[3] = -1;
			chanEnvSweep[3] = val&0x7;
			break;
		// Frequency
		case 0x22:
			chanFreq[3] = val>>4;
			chan4FreqRatio = val&0x7;
			if (chan4FreqRatio == 0)
				chan4FreqRatio = 0.5;
			chan4Width = !!(val&0x8);
            printLog("Freq %x\n", chanFreq[3]);
			break;
		// Start
		case 0x23:
			if (val&0x80)
			{
				chanLenCounter[3] = (64-chanLen[3])*clockSpeed/256;
				chanVol[3] = gameboy->ioRam[0x21]>>4;
				chanOn[3] = 1;
                lfsr = gbNoiseReset();
			}
			chanUseLen[3] = !!(val&0x40);
			break;
		case 0x24:
			//printf("Access volume\n");
			SO1Vol = val&0x7;
			SO2Vol = (val>>4)&0x7;
			break;
		case 0x25:
			chanToOut1[0] = !!(val&0x1);
			chanToOut1[1] = !!(val&0x2);
			chanToOut1[2] = !!(val&0x4);
			chanToOut1[3] = !!(val&0x8);
			chanToOut2[0] = !!(val&0x10);
			chanToOut2[1] = !!(val&0x20);
			chanToOut2[2] = !!(val&0x40);
			chanToOut2[3] = !!(val&0x80);
			break;
		case 0x26:
			if (!(val&0x80))
			{
				chanOn[0] = 0;
				chanOn[1] = 0;
				chanOn[2] = 0;
				chanOn[3] = 0;
				gameboy->clearSoundChannel(CHAN_1);
				gameboy->clearSoundChannel(CHAN_2);
				gameboy->clearSoundChannel(CHAN_3);
				gameboy->clearSoundChannel(CHAN_4);
			}
			break;
		default:
			break;
	}
}

// Global functions

void muteSND() {

}
void unmuteSND() {

}
void enableChannel(int i) {
    chanEnabled[i] = true;
}
void disableChannel(int i) {
    chanEnabled[i] = false;
}
