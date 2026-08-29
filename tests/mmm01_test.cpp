#include <assert.h>
#include <stdio.h>

#include "mmm01.h"

static void testPowerOnMapping() {
    Mmm01State state;
    state.reset();
    const Mmm01Mapping mapping = state.mapping(32);
    assert(!state.mapped);
    assert(mapping.lowerRomBank == 30);
    assert(mapping.upperRomBank == 31);
}

static void testMappedBanking() {
    Mmm01State state;
    state.reset();
    state.write(0x2000, 0x23); // mid=1, low=3
    state.write(0x6000, 0x18); // lock low bits 2-3 for game selection
    state.write(0x4000, 0x10); // high ROM bit
    state.write(0x0000, 0x40); // enter mapped mode

    Mmm01Mapping mapping = state.mapping(512);
    assert(state.mapped);
    assert(mapping.lowerRomBank == 0xa0);
    assert(mapping.upperRomBank == 0xa3);

    state.write(0x2000, 0x1f);
    mapping = state.mapping(512);
    assert(mapping.upperRomBank == 0xb3);
}

static void testBankZeroAlias() {
    Mmm01State state;
    state.reset();
    state.write(0x2000, 0x10);
    state.write(0x6000, 0x3c); // mask low bits 1-4
    state.write(0x0000, 0x40);
    const Mmm01Mapping mapping = state.mapping(128);
    assert(mapping.lowerRomBank == 0x10);
    assert(mapping.upperRomBank == 0x11);
}

static void testRamAndMultiplex() {
    Mmm01State state;
    state.reset();
    state.write(0x2000, 0x61); // mid=3, low=1
    state.write(0x4000, 0x0b); // high RAM=2, low RAM=3
    state.write(0x6000, 0x41); // multiplex, mode 1
    state.write(0x0000, 0x4a); // enable RAM and map
    const Mmm01Mapping mapping = state.mapping(512);
    assert(mapping.ramEnabled);
    assert(mapping.upperRomBank == 0x61);
    assert(mapping.ramBank == 0x0b);
}

static void testLocks() {
    Mmm01State state;
    state.reset();
    state.write(0x2000, 0x04);
    state.write(0x4000, 0x40); // lock mode writes
    state.write(0x6000, 0x09); // mask ROM low bit 2; mode write ignored
    assert(state.mode == 0);
    state.write(0x0000, 0x40);
    state.write(0x2000, 0x00);
    assert((state.romBank & 0x04) != 0);
}

int main() {
    assert(isMmm01CartridgeType(0x0b));
    assert(isMmm01CartridgeType(0x0c));
    assert(isMmm01CartridgeType(0x0d));
    assert(!isMmm01CartridgeType(0x15));
    testPowerOnMapping();
    testMappedBanking();
    testBankZeroAlias();
    testRamAndMultiplex();
    testLocks();
    puts("MMM01 tests passed");
    return 0;
}
