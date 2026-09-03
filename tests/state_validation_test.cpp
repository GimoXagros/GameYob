#include "state_validation.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>

int main() {
    for (int version = 1; version <= 8; ++version)
        assert(stateVersionSupported(version, 8));
    assert(!stateVersionSupported(0, 8));
    assert(!stateVersionSupported(-1, 8));
    assert(!stateVersionSupported(9, 8));
    assert(!statePayloadFits(0, 0, 4));
    assert(!statePayloadFits(3, 0, 4));
    assert(statePayloadFits(4, 0, 4));
    assert(!statePayloadFits(-1, 0, 0));
    assert(!statePayloadFits(100, SIZE_MAX, 1));
    assert(!statePayloadFits(100, 1, SIZE_MAX));
    assert(statePayloadFits(100, 20, 80));
    assert(!statePayloadFits(99, 20, 80));
    for (int wram = 0; wram < 8; ++wram)
        for (int vram = 0; vram < 2; ++vram)
            assert(stateMemoryBanksValid(wram, vram));
    assert(!stateMemoryBanksValid(-1, 0));
    assert(!stateMemoryBanksValid(8, 0));
    assert(!stateMemoryBanksValid(1, -1));
    assert(!stateMemoryBanksValid(1, 2));
    assert(!stateMemoryBanksValid(INT_MAX, INT_MAX));
}
