#include "gb_noise.h"

#include <limits.h>
#include <stdint.h>

uint16_t gbNoiseReset() {
    return 0x7fff;
}

uint16_t gbNoiseStep(uint16_t lfsr, bool width7) {
    const uint16_t feedback = (lfsr ^ (lfsr >> 1)) & 1;
    lfsr = (lfsr >> 1) | (feedback << 14);
    if (width7)
        lfsr = (lfsr & ~(1 << 6)) | (feedback << 6);
    return lfsr;
}

uint16_t gbNoiseAdvance(uint16_t lfsr, bool width7, unsigned clocks) {
    while (clocks--)
        lfsr = gbNoiseStep(lfsr, width7);
    return lfsr;
}

unsigned gbNoiseElapsedClocks(int* counter, int period, int elapsed) {
    if (!counter || period <= 0 || elapsed <= 0)
        return 0;

    const int64_t remaining = (int64_t)*counter - elapsed;
    if (remaining > 0) {
        *counter = (int)remaining;
        return 0;
    }

    // The old 3DS renderer calculated this value with signed division and
    // then passed it to gbNoiseAdvance() as unsigned.  After an NR43 period
    // change, a still-positive counter could therefore become a negative
    // clock count and turn into a multi-billion-iteration loop.  Advancing
    // only after the counter reaches zero also preserves the old <= 0 edge
    // behaviour without allowing a negative result.
    uint64_t clocks = (uint64_t)(-remaining) / (uint64_t)period + 1;
    if (clocks > UINT_MAX)
        clocks = UINT_MAX;

    int64_t next = remaining + (int64_t)clocks * period;
    if (next > INT_MAX)
        next = INT_MAX;
    *counter = (int)next;
    return (unsigned)clocks;
}

int gbNoisePolarity(uint16_t lfsr) {
    return (lfsr & 1) ? -1 : 1;
}
