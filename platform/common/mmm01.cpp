#include "mmm01.h"

#include "rom_layout.h"

namespace {

int normalizeBank(int bank, int count) {
    const int normalized = romlayout::normalizeBank(bank, count);
    return normalized < 0 ? 0 : normalized;
}

} // namespace

bool isMmm01CartridgeType(uint8_t type) {
    return type == 0x0b || type == 0x0c || type == 0x0d;
}

void Mmm01State::reset() {
    mapped = false;
    ramEnabled = false;
    multiplex = false;
    modeLocked = false;
    romBank = 1;
    ramBank = 0;
    romMask = 0;
    ramMask = 0;
    mode = 0;
}

void Mmm01State::write(uint16_t address, uint8_t value) {
    value &= 0x7f;

    if (address < 0x2000) {
        ramEnabled = (value & 0x0f) == 0x0a;
        if (!mapped) {
            ramMask = (value >> 4) & 0x03;
            if (value & 0x40)
                mapped = true;
        }
        return;
    }

    if (address < 0x4000) {
        const uint8_t lowMask = romMask & 0x1e;
        const uint8_t oldLow = romBank & 0x1f;
        const uint8_t requestedLow = value & 0x1f;
        const uint8_t newLow = (oldLow & lowMask) |
            (requestedLow & static_cast<uint8_t>(~lowMask));
        romBank = (romBank & 0x60) | newLow;
        if (!mapped)
            romBank = (romBank & 0x1f) | (value & 0x60);
        return;
    }

    if (address < 0x6000) {
        const uint8_t lowMask = ramMask & 0x03;
        const uint8_t oldLow = ramBank & 0x03;
        const uint8_t requestedLow = value & 0x03;
        const uint8_t newLow = (oldLow & lowMask) |
            (requestedLow & static_cast<uint8_t>(~lowMask));
        ramBank = (ramBank & 0x7c) | newLow;
        if (!mapped) {
            ramBank = newLow | (value & 0x7c);
            modeLocked = (value & 0x40) != 0;
        }
        return;
    }

    if (!modeLocked)
        mode = value & 1;
    if (!mapped) {
        // Register bits 1-5 correspond to ROM-bank-low mask bits 0-4. The
        // hardware ignores the lowest mask bit, so bank bit 0 stays writable.
        romMask = ((value >> 1) & 0x1f) & 0x1e;
        multiplex = (value & 0x40) != 0;
    }
}

Mmm01Mapping Mmm01State::mapping(int romBanks) const {
    Mmm01Mapping result = {};
    result.ramEnabled = ramEnabled;
    result.romMask = romMask;

    if (!mapped) {
        result.lowerRomBank = normalizeBank(romBanks - 2, romBanks);
        result.upperRomBank = normalizeBank(romBanks - 1, romBanks);
        result.ramBank = 0;
        result.romBase = result.lowerRomBank;
        return result;
    }

    const int low = romBank & 0x1f;
    const int mid = (romBank >> 5) & 0x03;
    const int high = (ramBank >> 4) & 0x03;
    const int ramLow = ramBank & 0x03;
    const int ramHigh = (ramBank >> 2) & 0x03;
    const int selectedLow = low & romMask;
    int switchLow = low;
    if ((low & static_cast<int>(~romMask) & 0x1f) == 0)
        switchLow |= 1;

    if (!multiplex) {
        result.romBase = (high << 7) | (mid << 5) | selectedLow;
        result.lowerRomBank = normalizeBank(result.romBase, romBanks);
        result.upperRomBank = normalizeBank(
            (high << 7) | (mid << 5) | switchLow, romBanks);
        const int effectiveRamLow = mode ? ramLow : (ramLow & ramMask);
        result.ramBank = (ramHigh << 2) | effectiveRamLow;
    }
    else {
        const int lowerMiddle = mode ? ramLow : (ramLow & ramMask);
        result.romBase = (high << 7) | (lowerMiddle << 5) | selectedLow;
        result.lowerRomBank = normalizeBank(result.romBase, romBanks);
        result.upperRomBank = normalizeBank(
            (high << 7) | (ramLow << 5) | switchLow, romBanks);
        result.ramBank = (ramHigh << 2) | mid;
    }

    return result;
}
