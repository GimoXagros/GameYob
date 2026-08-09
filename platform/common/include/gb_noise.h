#pragma once

#include <stdint.h>

uint16_t gbNoiseReset();
uint16_t gbNoiseStep(uint16_t lfsr, bool width7);
uint16_t gbNoiseAdvance(uint16_t lfsr, bool width7, unsigned clocks);
unsigned gbNoiseElapsedClocks(int* counter, int period, int elapsed);
int gbNoisePolarity(uint16_t lfsr);
