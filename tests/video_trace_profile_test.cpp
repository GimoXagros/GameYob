#include <assert.h>
#include "../platform/ds/arm9/source/video_trace_profile.h"

static void advance(VideoTraceProfile& profile, unsigned first, unsigned last) {
    for (unsigned frame = first; frame <= last; ++frame)
        assert(profile.observe(true, true, frame) == VideoTraceProfile::NONE);
}

int main() {
    VideoTraceProfile probe(449, 0);
    assert(probe.observe(false, false, 450) == VideoTraceProfile::NONE);
    assert(probe.observe(true, true, 0) == VideoTraceProfile::NONE);
    assert(probe.phase() == VideoTraceProfile::WARMUP);

    bool fastForwardMode = true;
    VideoTraceProfile profile(4, &fastForwardMode);
    advance(profile, 5, 183);
    assert(profile.observe(true, true, 184) == VideoTraceProfile::NONE);
    assert(profile.phase() == VideoTraceProfile::STABLE);
    advance(profile, 185, 243);
    assert(profile.observe(true, true, 244) == VideoTraceProfile::START_WINDOW);
    assert(profile.phase() == VideoTraceProfile::NORMAL && !fastForwardMode);
    advance(profile, 245, 263);
    assert(profile.observe(true, true, 264) == VideoTraceProfile::END_WINDOW);
    assert(fastForwardMode); // Restored before an exporter can fail.
    assert(profile.nextWindow());
    assert(profile.phase() == VideoTraceProfile::FAST && fastForwardMode);
    advance(profile, 265, 283);
    assert(profile.observe(true, true, 284) == VideoTraceProfile::END_WINDOW);
    assert(fastForwardMode);
    assert(profile.nextWindow());
    assert(profile.phase() == VideoTraceProfile::RECOVERY && !fastForwardMode);
    advance(profile, 285, 303);
    assert(profile.observe(true, true, 304) == VideoTraceProfile::END_WINDOW);
    assert(fastForwardMode);
    assert(!profile.nextWindow());
    assert(profile.phase() == VideoTraceProfile::DONE);

    bool ff = false;
    VideoTraceProfile masked(0, &ff);
    advance(masked, 1, 179);
    assert(masked.observe(true, true, 180) == VideoTraceProfile::NONE);
    advance(masked, 181, 209);
    assert(masked.observe(true, false, 210) == VideoTraceProfile::NONE);
    advance(masked, 211, 269);
    assert(masked.observe(true, true, 270) == VideoTraceProfile::START_WINDOW);
    advance(masked, 271, 289);
    assert(masked.observe(true, true, 290) == VideoTraceProfile::END_WINDOW);
    assert(masked.nextWindow() && ff);
    assert(masked.observe(true, false, 291) == VideoTraceProfile::ABORT_WINDOW);
    assert(masked.attempt() == 2 && masked.phase() == VideoTraceProfile::WARMUP);
    assert(!ff); // Abort restores FF before any logging or export.
    advance(masked, 292, 470);
    assert(masked.observe(true, true, 471) == VideoTraceProfile::NONE);
    advance(masked, 472, 530);
    assert(masked.observe(true, true, 531) == VideoTraceProfile::START_WINDOW);
    assert(masked.observe(true, false, 532) == VideoTraceProfile::ABORT_WINDOW);
    assert(masked.attempt() == 3 && !ff);
    advance(masked, 533, 711);
    assert(masked.observe(true, true, 712) == VideoTraceProfile::NONE);
    advance(masked, 713, 771);
    assert(masked.observe(true, true, 772) == VideoTraceProfile::START_WINDOW);
    assert(masked.observe(true, false, 773) == VideoTraceProfile::ABORT_WINDOW);
    assert(masked.phase() == VideoTraceProfile::DONE && !ff);

    VideoTraceProfile invalid(0, &ff);
    advance(invalid, 1, 179);
    assert(invalid.observe(true, true, 180) == VideoTraceProfile::NONE);
    advance(invalid, 181, 239);
    assert(invalid.observe(true, true, 240) == VideoTraceProfile::START_WINDOW);
    advance(invalid, 241, 259);
    assert(invalid.observe(true, true, 260) == VideoTraceProfile::END_WINDOW);
    assert(invalid.retryInvalidEvidence(260));
    assert(invalid.phase() == VideoTraceProfile::WARMUP && !ff);
    invalid.stop();
    assert(invalid.phase() == VideoTraceProfile::DONE && !ff);
    return 0;
}
