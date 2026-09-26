#include "video_trace_profile.h"

#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_PUBLICATION_ONLY)
#include "video_frame_trace.h"
#include <errno.h>
#include <stdio.h>

#ifndef GIT_REVISION
#define GIT_REVISION "unknown"
#endif
#ifndef GAMEYOB_VERSION
#define GAMEYOB_VERSION "unknown"
#endif

bool exportVideoTraceProfileCsv(VideoTraceProfile::Phase phase,
                                uint32_t firstGuestFrame,
                                uint32_t lastGuestFrame,
                                uint32_t overwrittenBefore,
                                const char* directory) {
    char path[256];
    FILE* output = 0;
    for (unsigned number = 0; number < 100; ++number) {
        const int length = snprintf(path, sizeof(path),
            "%sgameyob_video_profile_%s_%02u.csv",
            directory ? directory : "", VideoTraceProfile::phaseName(phase),
            number);
        if (length < 0 || (unsigned)length >= sizeof(path))
            return false;
        // Exclusive creation is mandatory: never replace a previous trace.
        output = fopen(path, "wx");
        if (output || errno != EEXIST)
            break;
    }
    if (!output)
        return false;

    const uint32_t overwrittenDelta =
        videoFrameTraceOverwritten() - overwrittenBefore;
    bool ok = fprintf(output, "profile,%s\nrevision,%s\nversion,%s\n"
        "first_guest,%lu\nlast_guest,%lu\noverwritten_delta,%lu\n",
        VideoTraceProfile::phaseName(phase), GIT_REVISION, GAMEYOB_VERSION,
        (unsigned long)firstGuestFrame, (unsigned long)lastGuestFrame,
        (unsigned long)overwrittenDelta) > 0;
    ok = ok && fprintf(output, "host,guest,published,line,type,draw,render,"
        "vram_c,vram_d,ready,scale,filter,capture,main,sub,"
        "fast_forward,gb_mode,sgb_mode,gfx_mask,tile_queue,map_queue\n") > 0;
    VideoFrameEvent event;
    unsigned count = 0;
    while (ok && count < 256 && readVideoFrameTrace(&event, 1) == 1) {
        ok = fprintf(output,
            "%lu,%lu,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%lu,%lu,%lu,"
            "%u,%u,%u,%u,%u,%u\n",
            (unsigned long)event.hostFrame,
            (unsigned long)event.guestFrame,
            (unsigned long)event.publishedFrame,
            event.physicalLine, event.type, event.drawingBuffer,
            event.renderingBuffer, event.vramC, event.vramD,
            event.transferReady, event.scalingMode, event.filterMode,
            (unsigned long)event.captureControl,
            (unsigned long)event.displayControlMain,
            (unsigned long)event.displayControlSub,
            event.fastForward, event.gbMode, event.sgbMode, event.gfxMask,
            event.tileQueueLength, event.mapQueueLength) > 0;
        ++count;
    }
    if (count == 256 && readVideoFrameTrace(&event, 1) == 1)
        ok = false;
    const int closeResult = fclose(output);
    return ok && closeResult == 0;
}
#endif
