#include "gb_noise.h"

#include <assert.h>

static unsigned period(bool width7) {
    const uint16_t initial = gbNoiseReset();
    uint16_t state = initial;
    unsigned clocks = 0;
    do {
        state = gbNoiseStep(state, width7);
        ++clocks;
        assert(clocks <= 32767);
    } while (width7 ? (state & 0x7f) != (initial & 0x7f) : state != initial);
    return clocks;
}

int main() {
    assert(gbNoiseReset() == 0x7fff);
    assert(period(false) == 32767);
    assert(period(true) == 127);

    uint16_t state = gbNoiseAdvance(gbNoiseReset(), false, 1);
    assert(state == 0x3fff);
    assert(gbNoisePolarity(state) == -1);
    state = gbNoiseAdvance(state, false, 14);
    assert(gbNoisePolarity(state) == 1);

    int counter = 1000;
    assert(gbNoiseElapsedClocks(&counter, 100, 84) == 0);
    assert(counter == 916);

    counter = 0;
    assert(gbNoiseElapsedClocks(&counter, 100, 84) == 1);
    assert(counter == 16);

    counter = 100;
    assert(gbNoiseElapsedClocks(&counter, 100, 100) == 1);
    assert(counter == 100);

    counter = 0;
    assert(gbNoiseElapsedClocks(&counter, 100, 100) == 2);
    assert(counter == 100);

    assert(gbNoiseElapsedClocks(&counter, 0, 84) == 0);
    return 0;
}
