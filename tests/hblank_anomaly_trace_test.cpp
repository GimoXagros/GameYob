#include <assert.h>
#include "../platform/ds/arm9/source/hblank_anomaly_trace.h"

int main() {
    assert(hblankAnomalyFlags(100, 100, 2, false) == 0);
    assert(hblankAnomalyFlags(100, 100, 0, false) == HBLANK_LATE_ENTRY);
    assert(hblankAnomalyFlags(100, 101, 2, false) == HBLANK_CROSSED_LINE);
    assert(hblankAnomalyFlags(100, 101, 0, true) ==
           (HBLANK_RETRY_PREVIOUS | HBLANK_LATE_ENTRY |
            HBLANK_CROSSED_LINE));
    // HBlank and VBlank asserted together outside visible scanout is not a
    // late guest-line event; line wrapping must not flood the anomaly ring.
    assert(hblankAnomalyFlags(192, 193, 3, false) == 0);

    HBlankAnomalyRing<2> ring;
    HBlankAnomalyEvent event = {};
    event.hostFrame = 1;
    ring.record(event);
    event.hostFrame = 2;
    ring.record(event);
    event.hostFrame = 3;
    ring.record(event);
    assert(ring.overwritten() == 1);
    ring.freeze();
    event.hostFrame = 4;
    ring.record(event);
    assert(ring.pop(&event) && event.hostFrame == 2);
    assert(ring.pop(&event) && event.hostFrame == 3);
    assert(!ring.pop(&event));
    ring.resume();
    event.hostFrame = 5;
    ring.record(event);
    assert(ring.pop(&event) && event.hostFrame == 5);
    ring.clear();
    assert(ring.overwritten() == 0 && !ring.pop(&event));
    return 0;
}
