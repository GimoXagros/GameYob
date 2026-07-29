#include "gb_noise.h"

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

int gbNoisePolarity(uint16_t lfsr) {
    return (lfsr & 1) ? -1 : 1;
}
