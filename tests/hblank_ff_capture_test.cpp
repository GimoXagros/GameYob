#include <assert.h>
#include "../platform/ds/arm9/source/hblank_ff_capture.h"

int main() {
    HBlankFfCapture capture;
    assert(capture.observe(0, false, 0, false) == HBlankFfCapture::NONE);
    assert(capture.observe(1, true, 100, false) == HBlankFfCapture::CLEAR);
    assert(capture.observe(1, true, 101, true) == HBlankFfCapture::CLEAR);
    assert(capture.observe(1, true, 102, true) == HBlankFfCapture::NONE);
    assert(capture.observe(1, true, 103, false) == HBlankFfCapture::RELEASE);
    assert(capture.pending());
    assert(capture.observe(1, false, 103, false) == HBlankFfCapture::NONE);
    capture.exported();
    assert(!capture.pending());
    assert(capture.observe(1, true, 104, true) == HBlankFfCapture::CLEAR);
    // Opening the menu clears the input, but is not a gameplay L release.
    assert(capture.observe(1, false, 105, false) == HBlankFfCapture::NONE);
    assert(!capture.pending());
    assert(capture.observe(1, true, 106, false) == HBlankFfCapture::NONE);
    assert(capture.observe(1, true, 107, true) == HBlankFfCapture::CLEAR);
    assert(capture.observe(2, false, 1, false) == HBlankFfCapture::CLEAR);
    assert(!capture.pending());
    assert(capture.observe(2, true, 2, true) == HBlankFfCapture::CLEAR);
    assert(capture.observe(2, true, 3, false) == HBlankFfCapture::RELEASE);
    assert(capture.observe(2, true, 0, false) == HBlankFfCapture::CLEAR);
    assert(!capture.pending());
    return 0;
}
