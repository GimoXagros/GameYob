#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../platform/ds/arm9/source/video_trace_profile.h"
#include "../platform/ds/arm9/source/video_frame_trace.h"

static unsigned reads = 0;
static bool injectMask = false;
static bool injectFastMismatch = false;
static bool omitUpload = false;
static uint32_t overwritten = 4;

unsigned readVideoFrameTrace(VideoFrameEvent* output, unsigned capacity) {
    if (!capacity || reads >= 60)
        return 0;
    *output = VideoFrameEvent();
    output->guestFrame = 201 + reads / 3;
    output->type = reads % 3 == 0 ? VIDEO_GUEST_COMPLETE :
                   reads % 3 == 1 ? VIDEO_PUBLISH : VIDEO_UPLOAD_END;
    if (omitUpload && reads == 2)
        output->type = VIDEO_HOST_VBLANK;
    output->fastForward = injectFastMismatch && reads == 10 ? 0 : 1;
    output->gfxMask = injectMask && reads == 10;
    output->tileQueueLength = 7;
    ++reads;
    return 1;
}
uint32_t videoFrameTraceOverwritten() { return overwritten; }
void freezeVideoFrameTrace() {}
void resumeVideoFrameTrace() {}

int main(int argc, char** argv) {
    assert(argc == 2); // Caller supplies a fresh temp directory plus slash.
    char path[512];
    snprintf(path, sizeof(path), "%sgameyob_video_profile_fast_forward_00.csv",
             argv[1]);
    FILE* seed = fopen(path, "wb");
    assert(seed);
    assert(fputs("preserve-me\n", seed) >= 0);
    assert(fclose(seed) == 0);

    assert(exportVideoTraceProfileCsv(VideoTraceProfile::FAST, 1, 201, 220, 4,
                                      argv[1]) == VIDEO_PROFILE_WRITTEN);
    char buffer[2048];
    FILE* original = fopen(path, "rb");
    assert(original);
    assert(fgets(buffer, sizeof(buffer), original));
    assert(strcmp(buffer, "preserve-me\n") == 0);
    assert(fclose(original) == 0);

    snprintf(path, sizeof(path), "%sgameyob_video_profile_fast_forward_01.csv",
             argv[1]);
    FILE* output = fopen(path, "rb");
    assert(output);
    const size_t length = fread(buffer, 1, sizeof(buffer) - 1, output);
    buffer[length] = 0;
    assert(fclose(output) == 0);
    assert(strstr(buffer, "profile,fast_forward"));
    assert(strstr(buffer, "attempt,1"));
    assert(strstr(buffer, "overwritten_delta,0"));
    assert(strstr(buffer, "publishes,20"));
    assert(strstr(buffer, "fast_forward,gb_mode,sgb_mode,gfx_mask,"));
    assert(strstr(buffer, "0,201,0,0,2,"));
    reads = 0;
    injectMask = true;
    assert(exportVideoTraceProfileCsv(VideoTraceProfile::FAST, 2, 201, 220, 4,
                                      argv[1]) == VIDEO_PROFILE_INVALID);
    snprintf(path, sizeof(path), "%sgameyob_video_profile_fast_forward_02.csv",
             argv[1]);
    assert(fopen(path, "rb") == 0); // Invalid evidence creates no file.
    reads = 0;
    injectMask = false;
    injectFastMismatch = true;
    assert(exportVideoTraceProfileCsv(VideoTraceProfile::FAST, 2, 201, 220, 4,
                                      argv[1]) == VIDEO_PROFILE_INVALID);
    reads = 0;
    injectFastMismatch = false;
    omitUpload = true;
    assert(exportVideoTraceProfileCsv(VideoTraceProfile::FAST, 2, 201, 220, 4,
                                      argv[1]) == VIDEO_PROFILE_INVALID);
    reads = 0;
    omitUpload = false;
    overwritten = 5;
    assert(exportVideoTraceProfileCsv(VideoTraceProfile::FAST, 2, 201, 220, 4,
                                      argv[1]) == VIDEO_PROFILE_INVALID);
    assert(fopen(path, "rb") == 0);
    reads = 0;
    overwritten = 4;
    assert(exportVideoTraceProfileCsv(VideoTraceProfile::FAST, 2, 201, 220, 4,
                    "missing-profile-directory/") == VIDEO_PROFILE_IO_ERROR);
    return 0;
}
