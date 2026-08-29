#include <nds/arm7/audio.h>
#include <nds/ipc.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/system.h>
#include <nds/timers.h>
#include "common.h"

// The lower this value, the more often sound will be synchronized.
#define SOUND_RESOLUTION 150

int channels[4] = {8,9,0,14};
const int dutyIndex[4] = {0, 1, 3, 5};
u32 schannelCR[4];

u8 backgroundSample[16];

bool currentLfsr;

void startChannel(int c);
void doCommand(u32 command);

// Use this with the 7-bit lfsr.
// Note: On a gameboy it's 127 samples, but on a ds, # samples must be a 
// multiple of 4. Previously I just repeated this 4 times. But that doesn't work 
// when the frequency is above 0xffff, apparently? So one extra byte is 
// appended instead. (I still have mild problems with frequencies >0xffff.)
u8 lfsr7NoiseSample[] ALIGN(4) = {
0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x60, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x60, 0x60, 0xa0, 0xa0, 0xa0, 0xa0, 0x60, 0xa0, 0x60, 0xa0, 0xa0, 0xa0, 0x60, 0x60, 0x60, 0x60, 0xa0, 0xa0, 0x60, 0xa0, 0xa0, 0xa0, 0x60, 0xa0, 0x60, 0x60, 0xa0, 0xa0, 0x60, 0x60, 0x60, 0xa0, 0x60, 0xa0, 0x60, 0xa0, 0xa0, 0x60, 0x60, 0x60, 0x60, 0x60, 0xa0, 0x60, 0xa0, 0xa0, 0xa0, 0xa0, 0x60, 0x60, 0x60, 0xa0, 0xa0, 0xa0, 0x60, 0xa0, 0xa0, 0x60, 0xa0, 0xa0, 0x60, 0x60, 0xa0, 0x60, 0x60, 0xa0, 0x60, 0xa0, 0x60, 0x60, 0xa0, 0x60, 0x60, 0x60, 0x60, 0xa0, 0x60, 0x60, 0xa0, 0xa0, 0xa0, 0x60, 0x60, 0xa0, 0x60, 0xa0, 0xa0, 0x60, 0xa0, 0x60, 0x60, 0x60, 0xa0, 0x60, 0x60, 0x60, 0xa0, 0xa0, 0x60, 0x60, 0xa0, 0xa0, 0x60, 0xa0, 0x60, 0xa0, 0x60, 0xa0, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0xa0, 0x60
};


// Use this with the 15-bit lfsr.
// noise.h is huge. Like, 32 kilobytes of white noise.
// I wasn't satisfied with the ds's noise channel in some situations, like when 
// the maku tree disappears in Oracle of Ages.
// If the arm7 binary hits its size limit, this may need to be reworked.
#include "noise.h"


// Callback for hyperSound / Sound Fix, which works with arm9 to synchronize 
// sound to the cycle.
void timerCallback() {
    sharedData->dsCycles+=SOUND_RESOLUTION;
    if (sharedData->cycles != -1) {
        bool doit=false;
        if (sharedData->frameFlip_Gameboy == sharedData->frameFlip_DS) {
            if (sharedData->dsCycles >= sharedData->cycles)
                doit = true;
        }
        else {
            doit = true;
        }
        if (doit) {
            sharedData->cycles = -1;
            doCommand(sharedData->message);
        }
    }
}


static void getChannelMix(int c, int* volume, int* pan) {
    if (!(sharedData->chanOn & (1 << c)) ||
            !sharedData->chanEnabled[c]) {
        *volume = 0;
        *pan = 64;
        return;
    }
    const int rightMaster = (sharedData->volControl & 7) + 1;
    const int leftMaster = ((sharedData->volControl >> 4) & 7) + 1;
    const int master = rightMaster > leftMaster ? rightMaster : leftMaster;
    const int right = (sharedData->chanOutput & (1 << c)) ? rightMaster : 0;
    const int left = (sharedData->chanOutput & (1 << (c + 4))) ? leftMaster : 0;
    const int total = left + right;

    if (total == 0 || master == 0) {
        *volume = 0;
        *pan = 64;
        return;
    }

    // SOUND_PAN linearly divides a DS channel between its two outputs. Sum
    // the desired GB left/right gains so center-panned audio is not halved.
    *volume = sharedData->chanRealVol[c] * 2 * total / master;
    if (*volume > 127)
        *volume = 127;
    *pan = (right * 127 + total / 2) / total;
}

void setChannelVolume(int c, bool write) {
    int channel = channels[c];

    int volume, pan;
    getChannelMix(c, &volume, &pan);

    schannelCR[c] &= ~(0x7f | SOUND_PAN(127));
    schannelCR[c] |= volume | SOUND_PAN(pan);
    if (write) {
        SCHANNEL_CR(channel) &= ~(0x7f | SOUND_PAN(127));
        SCHANNEL_CR(channel) |= volume | SOUND_PAN(pan);
    }
}
void updateChannel(int c, bool write) {
    int channel = channels[c];

    if (!(sharedData->chanOn & (1<<c)) || !sharedData->chanEnabled[c]) {
        schannelCR[c] &= ~0x7f;
        SCHANNEL_CR(channel) &= ~0x7f; // Set volume to zero
        return;
    }

    SCHANNEL_TIMER(channel) = SOUND_FREQ(sharedData->chanRealFreq[c]);
    if (c < 2) {
        schannelCR[c] &= ~(7<<24);
        schannelCR[c] |= dutyIndex[sharedData->chanDuty[c]] << 24;
    }
    else if (c == 3) {
        if (currentLfsr != sharedData->lfsr7Bit)
            startChannel(c);
    }
    if (write) {
        SCHANNEL_CR(channel) &= ~(7<<24);
        SCHANNEL_CR(channel) |= schannelCR[c] & (7<<24);
    }
    setChannelVolume(c, write);
}


void startChannel(int c) {
    int channel = channels[c];

    if (!sharedData->chanEnabled[c]) {
        SCHANNEL_CR(channel) = 0;
        return;
    }

    if (c == 2) {
        SCHANNEL_SOURCE(channel) = (u32)sharedData->sampleData;
        SCHANNEL_REPEAT_POINT(channel) = 0;
        SCHANNEL_LENGTH(channel) = 0x20>>2;
        schannelCR[c] = (0 << 29) | SOUND_REPEAT;
    }
    else if (c == 3) {
        SCHANNEL_CR(channel) = 0; // Why does this help? It seems to make other channels worse.

        currentLfsr = sharedData->lfsr7Bit;
        if (sharedData->lfsr7Bit) {
            SCHANNEL_SOURCE(channel) = (u32)lfsr7NoiseSample;
            SCHANNEL_LENGTH(channel) = 128>>2;
        }
        else {
            SCHANNEL_SOURCE(channel) = (u32)lfsr15NoiseSample;
            SCHANNEL_LENGTH(channel) = 32768>>2;
        }
        SCHANNEL_REPEAT_POINT(channel) = 0;
        schannelCR[c] = (0 << 29) | SOUND_REPEAT;
    }
    else { // PSG channels
        schannelCR[c] = (3 << 29);
    }

    updateChannel(c, false);

    SCHANNEL_CR(channel) = schannelCR[c] | SCHANNEL_ENABLE;
}

void updateMasterVolume() {
    const int rightMaster = (sharedData->volControl & 7) + 1;
    const int leftMaster = ((sharedData->volControl >> 4) & 7) + 1;
    const int master = rightMaster > leftMaster ? rightMaster : leftMaster;
    const int dsMaster = (master * 127 + 4) / 8;
    if (dsMaster != (REG_SOUNDCNT & 0x7f)) {
        REG_SOUNDCNT &= ~0x7f;
        REG_SOUNDCNT |= dsMaster;
    }

    // NR50 changes also alter the relative pan/volume of every routed channel.
    for (int channel=0; channel<4; ++channel)
        setChannelVolume(channel, true);

    // Each sound channel enabled in NR51 adds a bit of a "background tone".
    // I'm not really sure if this behaviour is correct.
    // I'm trying to strike a balance between the "Warlocked/Perfect Dark/Alone 
    // in the Dark" sound effects, and the annoying clicking in some games.
    int left=0, right=0;
    int i;
    for (i=0; i<4; i++) {
        if (sharedData->chanOutput & (1<<i))
            right += rightMaster;
        if (sharedData->chanOutput & (1<<(i+4)))
            left += leftMaster;
    }
    int total = left + right;
    int vol = master ? total * 0x20 / master : 0;
    int pan = total ? (right * 127 + total/2) / total : 64;
    if (vol > 0x7f)
        vol = 0x7f;
    SCHANNEL_CR(1) &= ~(0x7f | SOUND_PAN(127));
    if (sharedData->chanOutput)
        SCHANNEL_CR(1) |= vol | SOUND_PAN(pan);
}

void setHyperSound(int enabled) {
    if (enabled)
        timerStart(1, ClockDivider_1, TIMER_FREQ(4194304/SOUND_RESOLUTION), timerCallback);
    else
        timerStop(1);
}

void doCommand(u32 command) {
	int cmd = (command>>20)&0xf;
	int data = command & 0xFFFF;
    int i;
	
    switch(cmd) {

        case GBSND_UPDATE_COMMAND:
            if (data == 4) {
                for (i=0; i<4; i++) {
                    updateChannel(i, true);
                }
            }
            else
                updateChannel(data, true);
            break;

        case GBSND_START_COMMAND:
            startChannel(data);
            break;

        case GBSND_VOLUME_COMMAND:
            setChannelVolume(data, true);
            break;

        case GBSND_MASTER_VOLUME_COMMAND:
            updateMasterVolume();
            break;

        case GBSND_KILL_COMMAND:
            //SCHANNEL_CR(channel) &= ~SCHANNEL_ENABLE;
            break;

        case GBSND_MUTE_COMMAND:
            // This does not touch the "background hum" to prevent clicking.
            for (i=0; i<4; i++) {
                SCHANNEL_CR(channels[i]) &= ~0x7f;
            }
            break;

        case GBSND_UNMUTE_COMMAND:
            for (i=0; i<4; i++) {
                if ((sharedData->chanOn & (1<<i)) && sharedData->chanEnabled[i])
                    setChannelVolume(i, true);
            }
            break;

        case GBSND_HYPERSOUND_ENABLE_COMMAND:
            setHyperSound(data);
            break;

        case GBSND_SGB_BUFFER_COMMAND:
            SCHANNEL_CR(15) = 0;
            if (sharedData->sgbHostAudio) {
                SCHANNEL_SOURCE(15) = (u32)sharedData->sgbHostPcm[data & 1];
                SCHANNEL_REPEAT_POINT(15) = 0;
                SCHANNEL_LENGTH(15) = (548 * sizeof(s16)) >> 2;
                SCHANNEL_TIMER(15) = SOUND_FREQ(32768);
                SCHANNEL_CR(15) = SCHANNEL_ENABLE | SOUND_ONE_SHOT |
                    SOUND_FORMAT_16BIT | SOUND_VOL(64) | SOUND_PAN(64);
            }
            break;

        default:
            return;
    }
}

void gameboySoundCommandHandler(u32 command, void* userdata) {
    sharedData->fifosReceived++;
    doCommand(command);
}

void installGameboySoundFIFO() {
    fifoSetValue32Handler(FIFO_USER_01, gameboySoundCommandHandler, 0);
    sharedData->cycles = -1;
    sharedData->fifosSent = 0;
    sharedData->fifosReceived = 0;
    sharedData->frameFlip_DS = 0;
    sharedData->frameFlip_Gameboy = 0;
    setHyperSound(1);

    int i;
    for (i=0; i<16; i++)
        backgroundSample[i] = 0x7f;

    // The gameboy produces a sort of background "hum".
    // By simply existing, this allows for games to adjust global volume to make 
    // certain complex sound effects - even when all other channels are muted.
    // Eg. Warlocked, Perfect Dark.
    // This is the cause of "clicking" in certain games. It exists to an extent 
    // on real gameboys as well.
    SCHANNEL_SOURCE(1) = (u32)backgroundSample;
    SCHANNEL_REPEAT_POINT(1) = 0;
    SCHANNEL_LENGTH(1) = 16>>2;
    SCHANNEL_CR(1) = SCHANNEL_ENABLE | SOUND_VOL(0) | 
        SOUND_PAN(64) | (0 << 29) | SOUND_REPEAT;
}
