#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#define _chdir chdir
#endif
#include "../platform/ds/arm9/source/video_ff_release_capture.h"
#include "../platform/ds/arm9/source/video_frame_trace.h"

static unsigned cursor = 0;
static unsigned available = 4;
unsigned readVideoFrameTrace(VideoFrameEvent* output, unsigned capacity) {
    if (!capacity || cursor >= available)
        return 0;
    *output = VideoFrameEvent();
    output->guestFrame = cursor < 2 ? 10 : 11;
    output->type = cursor % 2 == 0 ? VIDEO_PUBLISH : VIDEO_UPLOAD_END;
    output->fastForward = cursor < 2;
    ++cursor;
    return 1;
}
uint32_t videoFrameTraceOverwritten() { return 1000; }
void freezeVideoFrameTrace() {}
void resumeVideoFrameTrace() {}

int main(int argc, char** argv) {
    assert(argc == 2); // Run in an isolated, fresh temporary directory.
    char path[512];
    snprintf(path, sizeof(path), "%s/gameyob_ff_release_trace_00.csv", argv[1]);
    FILE* seed = fopen(path, "wb");
    assert(seed && fputs("preserve-me\n", seed) >= 0 && fclose(seed) == 0);
    const int changed = _chdir(argv[1]);
    assert(changed == 0);
    assert(exportVideoFfReleaseTrace(10, 121));
    FILE* old = fopen("gameyob_ff_release_trace_00.csv", "rb");
    assert(old);
    char line[256];
    assert(fgets(line, sizeof(line), old));
    assert(strcmp(line, "preserve-me\n") == 0);
    assert(fclose(old) == 0);
    FILE* output = fopen("gameyob_ff_release_trace_01.csv", "rb");
    assert(output);
    char buffer[2048];
    const size_t length = fread(buffer, 1, sizeof(buffer)-1, output);
    buffer[length] = 0;
    assert(fclose(output) == 0);
    assert(strstr(buffer, "mode,ff_release"));
    assert(strstr(buffer, "release_guest,10"));
    assert(strstr(buffer, "overwritten,1000"));
    assert(strstr(buffer, "ff_publishes,1"));
    assert(strstr(buffer, "normal_publishes,1"));
    cursor = 0;
    available = 0;
    assert(!exportVideoFfReleaseTrace(10, 121));
    cursor = 0;
    available = 2;
    assert(!exportVideoFfReleaseTrace(10, 121));
    assert(fopen("gameyob_ff_release_trace_02.csv", "rb") == 0);
    return 0;
}
