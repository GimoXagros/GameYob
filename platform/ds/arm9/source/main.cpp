#include <nds.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
//#include <iostream>
//#include <string>
#include "gbgfx.h"
#include "inputhelper.h"
#include "filechooser.h"
#include "timer.h"
#include "soundengine.h"
#include "gameboy.h"
#include "main.h"
#include "console.h"
#include "nifi.h"
#include "cheats.h"
#include "gbs.h"
#include "common.h"
#include "romfile.h"
#include "menu.h"
#include "gbmanager.h"
#include "config.h"
#include "error.h"
#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_FF_RELEASE)
#include "video_frame_trace.h"
#include "video_ff_release_capture.h"
#elif defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_PUBLICATION_ONLY)
#include "video_frame_trace.h"
#include "video_trace_profile.h"
#endif

void updateVBlank();

volatile SharedData* sharedData;
volatile SharedData* dummySharedData;

// This is used to signal sleep mode starting or ending.
void fifoValue32Handler(u32 value, void* user_data) {
    static u8 scalingWasOn;
    static bool soundWasDisabled;
    static bool wasPaused;
    switch(value) {
        case FIFOMSG_LID_CLOSED:
            // Entering sleep mode
            scalingWasOn = sharedData->scalingOn;
            soundWasDisabled = soundDisabled;
            wasPaused = mgr_isPaused();

            sharedData->scalingOn = 0;
            soundDisabled = true;
            mgr_pause();
            break;
        case FIFOMSG_LID_OPENED:
            // Exiting sleep mode
            sharedData->scalingOn = scalingWasOn;
            soundDisabled = soundWasDisabled;
            if (!wasPaused)
                mgr_unpause();
            // Time isn't incremented properly in sleep mode, compensate here.
            time(&rawTime);
            lastRawTime = rawTime;
            break;
    }
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    REG_POWERCNT = POWER_ALL & ~(POWER_MATRIX | POWER_3D_CORE); // don't need 3D
    consoleDebugInit(DebugDevice_CONSOLE);
    lcdMainOnTop();

    defaultExceptionHandler();

    time(&rawTime);
    lastRawTime = rawTime;
    timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(1), clockUpdater);

    /* Reset the EZ3in1 if present */
    if (!isDSiMode()) {
        sysSetCartOwner(BUS_OWNER_ARM9);

        GBA_BUS[0x0000] = 0xF0;
        GBA_BUS[0x1000] = 0xF0;
    }

    fifoSetValue32Handler(FIFO_USER_02, fifoValue32Handler, NULL);

    SharedData* sharedAllocation =
        (SharedData*)calloc(1, sizeof(SharedData));
    SharedData* dummyAllocation =
        (SharedData*)calloc(1, sizeof(SharedData));
    if (!sharedAllocation || !dummyAllocation)
        fatalerr("Unable to allocate ARM7 shared memory.");
    sharedData = (SharedData*)memUncached(sharedAllocation);
    dummySharedData = (SharedData*)memUncached(dummyAllocation);
    sharedData->scalingOn = false;
    sharedData->enableSleepMode = true;
    // It might make more sense to use "fifoSendAddress" here.
    // However there may have been something wrong with it in dsi mode.
    fifoSendValue32(FIFO_USER_03, ((u32)sharedData)&0x00ffffff);


    initInput();

    mgr_init();
    setMenuDefaults();
    readConfigFile();

    lcdMainOnBottom();
    swiWaitForVBlank();
    swiWaitForVBlank();
    // initGFX is called in gameboy->init, but I also call it from here to
    // set up the vblank handler asap.
    initGFX();

    consoleInitialized = false;

    const char* autoloadRom = getAutoloadRomPath();
    if (autoloadRom && *autoloadRom) {
        // Match the file chooser's DS/DSi behavior: relative autoload names
        // are resolved from rompath, preserving the exact FAT LFN bytes.
        loadFileChooserState(&romChooserState);
        fs_cacheDirectoryAliases();
        fs_closeDirectory();
        mgr_loadRom(autoloadRom);
        updateScreens();
    }
    else if (argc >= 2) {
        char* filename = argv[1];
        mgr_loadRom(filename);
        updateScreens();
    }
    else {
        mgr_selectRom();
    }

#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_FF_RELEASE)
    VideoFfReleaseCapture ffReleaseCapture;
    unsigned ffReleaseVcount = 0;
#elif defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_PUBLICATION_ONLY)
    // This scripted diagnostic runs only for an explicitly autoloaded ROM.
    // Keep the 180-frame warmup out of the trace ring and never write in IRQ.
    const bool runVideoProfile = autoloadRom && *autoloadRom;
    VideoTraceProfile videoProfile(gameboy ? gameboy->gameboyFrameCounter : 0,
                                   &fastForwardMode);
    uint32_t profileFirstGuestFrame = 0;
    uint32_t profileOverwrittenBefore = 0;
    if (runVideoProfile)
        freezeVideoFrameTrace();
#endif
    for (;;) {
        mgr_runFrame();
        mgr_updateVBlank();
#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_FF_RELEASE)
        const uintptr_t activeRom = gameboy && gameboy->isRomLoaded() ?
            (uintptr_t)gameboy->getRomFile() : 0;
        const bool captureUsable = !probingForBorder && !isMenuOn() &&
            !isFileChooserOn() && !mgr_isPaused();
        const VideoFfReleaseCapture::Action releaseAction =
            ffReleaseCapture.observe(activeRom, captureUsable,
                gameboy ? gameboy->gameboyFrameCounter : 0,
                fastForwardKey || fastForwardMode);
        if (releaseAction == VideoFfReleaseCapture::NEW_ROM ||
                releaseAction == VideoFfReleaseCapture::CANCEL) {
            VideoFrameEvent discarded;
            while (readVideoFrameTrace(&discarded, 1) == 1) {}
            if (!isMenuOn() && !isFileChooserOn())
                resumeVideoFrameTrace();
            if (releaseAction == VideoFfReleaseCapture::CANCEL)
                printMenuMessage("FF trace cancelled; hold and release L again.");
        } else if (releaseAction == VideoFfReleaseCapture::RELEASE) {
            ffReleaseVcount = REG_VCOUNT;
        } else if (releaseAction == VideoFfReleaseCapture::READY) {
            freezeVideoFrameTrace();
            if (exportVideoFfReleaseTrace(ffReleaseCapture.releaseGuest(),
                                          ffReleaseVcount))
                printMenuMessage("FF release trace saved in current folder.");
            else
                printMenuMessage("FF release trace failed; no valid file.");
        }
#elif defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_PUBLICATION_ONLY)
        if (runVideoProfile && gameboy) {
            const uint32_t guestFrame = gameboy->gameboyFrameCounter;
            const VideoTraceProfile::Action action = videoProfile.observe(
                !probingForBorder, gfxMask == 0, guestFrame);
            if (action == VideoTraceProfile::START_WINDOW) {
                VideoFrameEvent discarded;
                while (readVideoFrameTrace(&discarded, 1) == 1) {}
                profileFirstGuestFrame = guestFrame + 1;
                profileOverwrittenBefore = videoFrameTraceOverwritten();
                resumeVideoFrameTrace();
            } else if (action == VideoTraceProfile::ABORT_WINDOW) {
                freezeVideoFrameTrace();
                printLog("Video publication profile discarded a masked window.\n");
            } else if (action == VideoTraceProfile::END_WINDOW) {
                // observe() restores the pre-profile FF state before I/O.
                freezeVideoFrameTrace();
                const VideoTraceProfileExportResult result =
                    exportVideoTraceProfileCsv(videoProfile.phase(),
                        videoProfile.attempt(), profileFirstGuestFrame,
                        guestFrame, profileOverwrittenBefore);
                if (result == VIDEO_PROFILE_INVALID) {
                    videoProfile.retryInvalidEvidence(guestFrame);
                    printLog("Video publication profile discarded invalid evidence.\n");
                } else if (result == VIDEO_PROFILE_IO_ERROR) {
                    videoProfile.stop();
                    printLog("Video publication profile export failed.\n");
                } else if (videoProfile.nextWindow()) {
                    VideoFrameEvent discarded;
                    while (readVideoFrameTrace(&discarded, 1) == 1) {}
                    profileFirstGuestFrame = guestFrame + 1;
                    profileOverwrittenBefore = videoFrameTraceOverwritten();
                    resumeVideoFrameTrace();
                }
            }
        }
#endif
    }

    return 0;
}
