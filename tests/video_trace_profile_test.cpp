#include <assert.h>
#include "../platform/ds/arm9/source/video_trace_profile.h"

int main() {
    VideoTraceProfile probe(449, 0);
    assert(probe.observe(false, 450) == VideoTraceProfile::NONE);
    assert(probe.observe(true, 0) == VideoTraceProfile::NONE);
    assert(probe.phase() == VideoTraceProfile::WARMUP);

    bool fastForwardMode = true;
    VideoTraceProfile profile(4, &fastForwardMode);
    assert(profile.observe(false, 4) == VideoTraceProfile::NONE);
    for (unsigned frame = 5; frame < 184; ++frame)
        assert(profile.observe(true, frame) == VideoTraceProfile::NONE);
    assert(profile.observe(true, 184) == VideoTraceProfile::START_WINDOW);
    assert(profile.phase() == VideoTraceProfile::NORMAL);
    assert(!profile.wantsFastForward());
    assert(!fastForwardMode);
    for (unsigned frame = 185; frame < 204; ++frame)
        assert(profile.observe(true, frame) == VideoTraceProfile::NONE);
    assert(profile.observe(true, 204) == VideoTraceProfile::END_WINDOW);
    assert(fastForwardMode); // Restored before an exporter can fail.
    assert(profile.observe(true, 205) == VideoTraceProfile::NONE);
    assert(profile.nextWindow());
    assert(profile.phase() == VideoTraceProfile::FAST);
    assert(profile.wantsFastForward());
    assert(fastForwardMode);
    for (unsigned frame = 205; frame < 224; ++frame)
        assert(profile.observe(true, frame) == VideoTraceProfile::NONE);
    assert(profile.observe(true, 224) == VideoTraceProfile::END_WINDOW);
    assert(fastForwardMode);
    assert(profile.nextWindow());
    assert(profile.phase() == VideoTraceProfile::RECOVERY);
    assert(!profile.wantsFastForward());
    assert(!fastForwardMode);
    for (unsigned frame = 225; frame < 244; ++frame)
        assert(profile.observe(true, frame) == VideoTraceProfile::NONE);
    assert(profile.observe(true, 244) == VideoTraceProfile::END_WINDOW);
    assert(fastForwardMode);
    assert(!profile.nextWindow());
    assert(profile.phase() == VideoTraceProfile::DONE);
    assert(profile.observe(true, 248) == VideoTraceProfile::NONE);
    return 0;
}
