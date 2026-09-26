#include "video_trace_profile.h"

#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_PUBLICATION_ONLY)
#include "video_frame_trace.h"
#include "video_ff_release_capture.h"
#include <errno.h>
#include <stdio.h>

#ifndef GIT_REVISION
#define GIT_REVISION "unknown"
#endif
#ifndef GAMEYOB_VERSION
#define GAMEYOB_VERSION "unknown"
#endif

#ifndef GAMEYOB_VIDEO_FF_RELEASE
VideoTraceProfileExportResult exportVideoTraceProfileCsv(
    VideoTraceProfile::Phase phase, unsigned attempt,
    uint32_t firstGuestFrame, uint32_t lastGuestFrame,
    uint32_t overwrittenBefore, const char* directory) {
    // Frozen-ring copy is diagnostic-only and remains outside the display IRQ.
    static VideoFrameEvent events[256];
    unsigned count = 0;
    while (count < 256 && readVideoFrameTrace(&events[count], 1) == 1)
        ++count;
    VideoFrameEvent extra;
    const uint32_t overwrittenDelta =
        videoFrameTraceOverwritten() - overwrittenBefore;
    if (count == 256 && readVideoFrameTrace(&extra, 1) == 1)
        return VIDEO_PROFILE_INVALID;

    unsigned completes = 0, publishes = 0, uploads = 0;
    bool pendingUpload = false;
    uint32_t pendingGuest = 0;
    const unsigned expectedFastForward = phase == VideoTraceProfile::FAST;
    bool valid = lastGuestFrame - firstGuestFrame == 19 &&
                 overwrittenDelta == 0;
    for (unsigned i = 0; i < count; ++i) {
        const VideoFrameEvent& event = events[i];
        if (event.gfxMask || event.fastForward != expectedFastForward)
            valid = false;
        if (event.type == VIDEO_GUEST_COMPLETE) {
            ++completes;
            if (event.guestFrame != firstGuestFrame + completes - 1)
                valid = false;
        } else if (event.type == VIDEO_PUBLISH) {
            ++publishes;
            if (pendingUpload || event.guestFrame < firstGuestFrame ||
                    event.guestFrame > lastGuestFrame)
                valid = false;
            pendingUpload = true;
            pendingGuest = event.guestFrame;
        } else if (event.type == VIDEO_UPLOAD_END) {
            ++uploads;
            if (!pendingUpload || event.guestFrame != pendingGuest)
                valid = false;
            pendingUpload = false;
        }
    }
    if (!valid || pendingUpload || completes != 20 || publishes == 0 ||
            publishes != uploads)
        return VIDEO_PROFILE_INVALID;

    char path[256];
    FILE* output = 0;
    for (unsigned number = 0; number < 100; ++number) {
        const int length = snprintf(path, sizeof(path),
            "%sgameyob_video_profile_%s_%02u.csv",
            directory ? directory : "", VideoTraceProfile::phaseName(phase),
            number);
        if (length < 0 || (unsigned)length >= sizeof(path))
            return VIDEO_PROFILE_IO_ERROR;
        // Exclusive creation is mandatory: never replace a previous trace.
        output = fopen(path, "wx");
        if (output || errno != EEXIST)
            break;
    }
    if (!output)
        return VIDEO_PROFILE_IO_ERROR;

    bool ok = fprintf(output, "profile,%s\nrevision,%s\nversion,%s\n"
        "attempt,%u\nfirst_guest,%lu\nlast_guest,%lu\noverwritten_delta,%lu\n"
        "guest_completes,%u\npublishes,%u\nupload_ends,%u\n",
        VideoTraceProfile::phaseName(phase), GIT_REVISION, GAMEYOB_VERSION,
        attempt,
        (unsigned long)firstGuestFrame, (unsigned long)lastGuestFrame,
        (unsigned long)overwrittenDelta, completes, publishes, uploads) > 0;
    ok = ok && fprintf(output, "host,guest,published,line,type,draw,render,"
        "vram_c,vram_d,ready,scale,filter,capture,main,sub,"
        "fast_forward,gb_mode,sgb_mode,gfx_mask,tile_queue,map_queue\n") > 0;
    for (unsigned i = 0; ok && i < count; ++i) {
        const VideoFrameEvent& event = events[i];
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
    }
    const int closeResult = fclose(output);
    return ok && closeResult == 0 ? VIDEO_PROFILE_WRITTEN :
                                    VIDEO_PROFILE_IO_ERROR;
}
#else
bool exportVideoFfReleaseTrace(uint32_t releaseGuest,
                               unsigned releaseVcount) {
    static VideoFrameEvent events[256];
    unsigned count = 0;
    while (count < 256 && readVideoFrameTrace(&events[count], 1) == 1)
        ++count;
    unsigned ffPublishes = 0, normalPublishes = 0;
    unsigned ffUploads = 0, normalUploads = 0;
    for (unsigned i = 0; i < count; ++i) {
        const VideoFrameEvent& event = events[i];
        if (event.type == VIDEO_PUBLISH) {
            if (event.fastForward && event.guestFrame <= releaseGuest)
                ++ffPublishes;
            if (!event.fastForward && event.guestFrame > releaseGuest)
                ++normalPublishes;
        } else if (event.type == VIDEO_UPLOAD_END) {
            if (event.fastForward && event.guestFrame <= releaseGuest)
                ++ffUploads;
            if (!event.fastForward && event.guestFrame > releaseGuest)
                ++normalUploads;
        }
    }
    if (!ffPublishes || !normalPublishes || !ffUploads || !normalUploads)
        return false;

    char path[64];
    FILE* output = 0;
    for (unsigned number = 0; number < 100; ++number) {
        const int length = snprintf(path, sizeof(path),
            "gameyob_ff_release_trace_%02u.csv", number);
        if (length < 0 || (unsigned)length >= sizeof(path))
            return false;
        output = fopen(path, "wx");
        if (output || errno != EEXIST)
            break;
    }
    if (!output)
        return false;
    bool ok = fprintf(output,
        "mode,ff_release\nrevision,%s\nversion,%s\n"
        "release_guest,%lu\nrelease_vcount,%u\n"
        "overwritten,%lu\nff_publishes,%u\nnormal_publishes,%u\n"
        "ff_uploads,%u\nnormal_uploads,%u\n",
        GIT_REVISION, GAMEYOB_VERSION, (unsigned long)releaseGuest,
        releaseVcount, (unsigned long)videoFrameTraceOverwritten(),
        ffPublishes, normalPublishes, ffUploads, normalUploads) > 0;
    ok = ok && fprintf(output,
        "host,guest,published,line,type,draw,render,vram_c,vram_d,ready,"
        "scale,filter,capture,main,sub,fast_forward,gb_mode,sgb_mode,"
        "gfx_mask,tile_queue,map_queue\n") > 0;
    for (unsigned i = 0; ok && i < count; ++i) {
        const VideoFrameEvent& event = events[i];
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
    }
    return fclose(output) == 0 && ok;
}
#endif
#endif
