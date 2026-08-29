#pragma once

#include <stdint.h>

struct Mmm01Mapping {
    int lowerRomBank;
    int upperRomBank;
    int ramBank;
    int romBase;
    int romMask;
    bool ramEnabled;
};

// Register-level MMM01 state. Keep this POD so save states can serialize each
// field explicitly without depending on compiler struct padding.
struct Mmm01State {
    bool mapped;
    bool ramEnabled;
    bool multiplex;
    bool modeLocked;
    uint8_t romBank;
    uint8_t ramBank;
    uint8_t romMask;
    uint8_t ramMask;
    uint8_t mode;

    void reset();
    void write(uint16_t address, uint8_t value);
    Mmm01Mapping mapping(int romBanks) const;
};

bool isMmm01CartridgeType(uint8_t type);
