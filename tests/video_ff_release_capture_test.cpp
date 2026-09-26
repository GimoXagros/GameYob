#include <assert.h>
#include "../platform/ds/arm9/source/video_ff_release_capture.h"

int main() {
    VideoFfReleaseCapture capture;
    assert(capture.observe(0, false, 0, false) == VideoFfReleaseCapture::NONE);
    // Manual ROM selection: no autoload setting is involved.
    assert(capture.observe(1, true, 1, false) == VideoFfReleaseCapture::NEW_ROM);
    for (unsigned f = 2; f <= 20; ++f)
        assert(capture.observe(1, true, f, true) == VideoFfReleaseCapture::NONE);
    assert(capture.observe(1, true, 21, false) == VideoFfReleaseCapture::RELEASE);
    assert(capture.releaseGuest() == 21);
    for (unsigned f = 22; f < 29; ++f)
        assert(capture.observe(1, true, f, false) == VideoFfReleaseCapture::NONE);
    assert(capture.observe(1, true, 29, false) == VideoFfReleaseCapture::READY);
    assert(capture.observe(1, true, 30, false) == VideoFfReleaseCapture::NONE);

    VideoFfReleaseCapture cancelled;
    assert(cancelled.observe(2, true, 10, true) == VideoFfReleaseCapture::NEW_ROM);
    assert(cancelled.observe(2, true, 11, false) == VideoFfReleaseCapture::RELEASE);
    assert(cancelled.observe(2, false, 11, false) == VideoFfReleaseCapture::CANCEL);
    assert(cancelled.observe(2, true, 12, false) == VideoFfReleaseCapture::NONE);
    assert(cancelled.observe(2, true, 13, true) == VideoFfReleaseCapture::NONE);
    assert(cancelled.observe(2, true, 14, false) == VideoFfReleaseCapture::RELEASE);
    assert(cancelled.observe(3, true, 1, false) == VideoFfReleaseCapture::CANCEL);
    assert(cancelled.observe(3, true, 2, false) == VideoFfReleaseCapture::NONE);
    assert(cancelled.observe(3, true, 3, true) == VideoFfReleaseCapture::NONE);
    assert(cancelled.observe(3, true, 4, false) == VideoFfReleaseCapture::RELEASE);
    assert(cancelled.observe(3, true, 0, false) == VideoFfReleaseCapture::CANCEL);
    return 0;
}
