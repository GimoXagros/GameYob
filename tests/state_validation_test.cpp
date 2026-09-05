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
    assert(stateBoolByteValid(0));
    assert(stateBoolByteValid(1));
    assert(!stateBoolByteValid(2));
    assert(stateSgbScalarsValid(7, 7, 128, 31, 3, 11));
    assert(stateSgbScalarsValid(0, 0, -1, 0, 0, 0));
    assert(!stateSgbScalarsValid(8, 0, 0, 0, 0, 0));
    assert(!stateSgbScalarsValid(2, 3, 0, 0, 0, 0));
    assert(!stateSgbScalarsValid(1, 0, 129, 0, 0, 0));
    assert(!stateSgbScalarsValid(1, 0, 0, 32, 0, 0));
    assert(!stateSgbScalarsValid(1, 0, 0, 0, 4, 0));
    assert(!stateSgbScalarsValid(1, 0, 0, 0, 0, 12));
    assert(stateSgbHostHeaderValid(0x53474248U, 1, 263496, 263496));
    assert(!stateSgbHostHeaderValid(0, 1, 263496, 263496));
    assert(!stateSgbHostHeaderValid(0x53474248U, 2, 263496, 263496));
    assert(!stateSgbHostHeaderValid(0x53474248U, 1, 263495, 263496));

    assert(stateStructBytesForVersion(1, 80, 84, 88) == 80);
    assert(stateStructBytesForVersion(2, 80, 84, 88) == 84);
    assert(stateStructBytesForVersion(8, 80, 84, 88) == 88);

    // Synthetic fixtures exercise every byte truncation without containing a
    // ROM or relying on a platform ABI. Version 1-2 have no mapper/SGB tail.
    for (int version = 1; version <= 8; ++version) {
        const size_t stateBytes = stateStructBytesForVersion(version, 80, 84, 88);
        const size_t mapperBytes = version >= 3 ? 9 : 0;
        const bool sgb = version >= 3;
        const size_t legacySgb = 374;
        const size_t extendedSgb = version >= 7 ? 4130 : 0;
        const bool host = version >= 7;
        const size_t hostBytes = 263496;
        const size_t expected = 100 + stateBytes + mapperBytes +
            (version >= 3 ? sizeof(bool) : 0) +
            (sgb ? legacySgb + extendedSgb + (host ? hostBytes : 0) : 0);
        for (size_t cut = 0; cut < expected; ++cut)
            assert(!stateExpectedPayloadSize(cut, version, 100, stateBytes,
                mapperBytes, sgb, legacySgb, extendedSgb, host, hostBytes, 0));
        size_t result = 0;
        assert(stateExpectedPayloadSize(expected, version, 100, stateBytes,
            mapperBytes, sgb, legacySgb, extendedSgb, host, hostBytes, &result));
        assert(result == expected);
        assert(stateExpectedPayloadSize(expected + 17, version, 100, stateBytes,
            mapperBytes, sgb, legacySgb, extendedSgb, host, hostBytes, &result));
        assert(result == expected);
    }

    size_t ignored = 0;
    assert(!stateExpectedPayloadSize(100, 8, SIZE_MAX, 1, 0, false,
        0, 0, false, 0, &ignored));

    // Fixed-seed scalar mutation corpus. Any accepted value must satisfy the
    // same explicit bounds used by the loader.
    uint32_t seed = 0x5a17c9e3U;
    for (int i = 0; i < 4096; ++i) {
        seed = seed * 1664525U + 1013904223U;
        const int length = (int)(seed & 15) - 4;
        const int transferred = (int)((seed >> 4) & 15) - 4;
        const int bit = (int)((seed >> 8) & 255) - 32;
        const unsigned command = (seed >> 16) & 63;
        const unsigned mask = (seed >> 22) & 7;
        const unsigned data = (seed >> 25) & 31;
        if (stateSgbScalarsValid(length, transferred, bit, command, mask, data)) {
            assert(length >= 0 && length <= 7);
            assert(transferred >= 0 && transferred <= length);
            assert(bit >= -1 && bit <= 128);
            assert(command < 32 && mask <= 3 && data <= 11);
        }
    }
}
