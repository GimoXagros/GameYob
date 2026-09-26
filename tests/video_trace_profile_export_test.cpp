#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../platform/ds/arm9/source/video_trace_profile.h"
#include "../platform/ds/arm9/source/video_frame_trace.h"

static unsigned reads = 0;

unsigned readVideoFrameTrace(VideoFrameEvent* output, unsigned capacity) {
    if (!capacity || reads++)
        return 0;
    *output = VideoFrameEvent();
    output->guestFrame = 201;
    output->type = VIDEO_PUBLISH;
    output->fastForward = 1;
    output->tileQueueLength = 7;
    return 1;
}
uint32_t videoFrameTraceOverwritten() { return 4; }
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

    assert(exportVideoTraceProfileCsv(VideoTraceProfile::FAST, 201, 220, 3,
                                      argv[1]));
    char buffer[1024];
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
    assert(strstr(buffer, "overwritten_delta,1"));
    assert(strstr(buffer, "fast_forward,gb_mode,sgb_mode,gfx_mask,"));
    assert(strstr(buffer, "0,201,0,0,2,"));
    assert(!exportVideoTraceProfileCsv(VideoTraceProfile::NORMAL, 0, 0, 0,
                                       "missing-profile-directory/"));
    return 0;
}
