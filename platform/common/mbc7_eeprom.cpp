#include "mbc7_eeprom.h"

#include <string.h>

namespace { const size_t kStateSize = 10; }

Mbc7Eeprom::Mbc7Eeprom() { reset(); }

void Mbc7Eeprom::reset() {
    phase = COMMAND;
    shift = output = 0;
    bitCount = address = 0;
    writeEnabled = chipSelected = clockHigh = modified = false;
    dataOut = true;
    dataIn = false;
}

void Mbc7Eeprom::beginCommand() {
    phase = COMMAND;
    shift = 0;
    bitCount = 0;
    dataOut = true;
}

uint16_t Mbc7Eeprom::readWord(const uint8_t* storage, size_t storageSize,
        uint8_t wordAddress) const {
    const size_t offset = (size_t)wordAddress * 2;
    if (!storage || offset + 1 >= storageSize) return 0xffff;
    return storage[offset] | ((uint16_t)storage[offset + 1] << 8);
}

void Mbc7Eeprom::writeWord(uint8_t* storage, size_t storageSize,
        uint8_t wordAddress, uint16_t value) {
    const size_t offset = (size_t)wordAddress * 2;
    if (!storage || offset + 1 >= storageSize) return;
    storage[offset] = value & 0xff;
    storage[offset + 1] = value >> 8;
    modified = true;
}

void Mbc7Eeprom::decodeCommand(uint8_t* storage, size_t storageSize) {
    const uint16_t command = shift;
    const unsigned operation = (command >> 8) & 3;
    address = command & 0x7f;
    bitCount = 0;
    shift = 0;
    if (operation == 2) {
        output = readWord(storage, storageSize, address);
        phase = READ_DATA;
    }
    else if (operation == 1) {
        phase = WRITE_DATA;
    }
    else if (operation == 3) {
        if (writeEnabled) writeWord(storage, storageSize, address, 0xffff);
        beginCommand();
    }
    else {
        switch ((command >> 6) & 3) {
        case 0: writeEnabled = false; beginCommand(); break; // EWDS
        case 1: phase = WRITE_ALL_DATA; break;               // WRAL
        case 2:                                              // ERAL
            if (writeEnabled && storage) {
                memset(storage, 0xff, storageSize < 256 ? storageSize : 256);
                modified = true;
            }
            beginCommand();
            break;
        case 3: writeEnabled = true; beginCommand(); break;  // EWEN
        }
    }
}

void Mbc7Eeprom::clockBit(bool bit, uint8_t* storage, size_t storageSize) {
    if (phase == READ_DATA) {
        dataOut = (output & 0x8000) != 0;
        output <<= 1;
        if (++bitCount == 16) {
            // Keep the final DO bit visible until the caller lowers CS.
            phase = COMMAND;
            shift = 0;
            bitCount = 0;
        }
        return;
    }
    if (phase == COMMAND && bitCount == 0 && !bit) {
        dataOut = true;
        return;
    }
    shift = (uint16_t)((shift << 1) | (bit ? 1 : 0));
    ++bitCount;
    if (phase == COMMAND && bitCount == 11) {
        decodeCommand(storage, storageSize);
    }
    else if ((phase == WRITE_DATA || phase == WRITE_ALL_DATA) &&
            bitCount == 16) {
        if (writeEnabled) {
            if (phase == WRITE_DATA) writeWord(storage, storageSize, address, shift);
            else {
                const size_t words = (storageSize < 256 ? storageSize : 256) / 2;
                for (size_t i = 0; i < words; ++i)
                    writeWord(storage, storageSize, (uint8_t)i, shift);
            }
        }
        beginCommand();
    }
}

void Mbc7Eeprom::writePins(uint8_t value, uint8_t* storage,
        size_t storageSize) {
    const bool selected = (value & 0x80) != 0;
    const bool clock = (value & 0x40) != 0;
    dataIn = (value & 0x02) != 0;
    if (!selected) {
        if (chipSelected) beginCommand();
        chipSelected = false;
        clockHigh = clock;
        return;
    }
    if (!chipSelected) beginCommand();
    chipSelected = true;
    if (!clockHigh && clock)
        clockBit(dataIn, storage, storageSize);
    clockHigh = clock;
}

uint8_t Mbc7Eeprom::readPins() const {
    return (uint8_t)((dataOut ? 1 : 0) | (dataIn ? 2 : 0) |
            (clockHigh ? 0x40 : 0) | (chipSelected ? 0x80 : 0));
}

bool Mbc7Eeprom::consumeModified() {
    const bool result = modified;
    modified = false;
    return result;
}

size_t Mbc7Eeprom::stateSize() { return kStateSize; }

uint16_t Mbc7Eeprom::sensorValue(int position, int neutralPosition,
        bool touching) {
    const int center = 0x81d0;
    if (!touching || neutralPosition <= 0)
        return (uint16_t)center;
    int delta = ((neutralPosition - position) * 0x70) / neutralPosition;
    if (delta > 0x70) delta = 0x70;
    if (delta < -0x70) delta = -0x70;
    return (uint16_t)(center + delta);
}

void Mbc7Eeprom::save(uint8_t* data, size_t size) const {
    if (!data || size < kStateSize) return;
    data[0] = (uint8_t)phase;
    data[1] = shift & 0xff; data[2] = shift >> 8;
    data[3] = output & 0xff; data[4] = output >> 8;
    data[5] = bitCount; data[6] = address;
    data[7] = writeEnabled ? 1 : 0;
    data[8] = chipSelected ? 1 : 0;
    data[9] = (clockHigh ? 1 : 0) | (dataOut ? 2 : 0) |
        (dataIn ? 4 : 0);
}

bool Mbc7Eeprom::load(const uint8_t* data, size_t size) {
    if (!data || size < kStateSize || data[0] > WRITE_ALL_DATA || data[5] > 16)
        return false;
    phase = (Phase)data[0];
    shift = (uint16_t)(data[1] | ((uint16_t)data[2] << 8));
    output = (uint16_t)(data[3] | ((uint16_t)data[4] << 8));
    bitCount = data[5]; address = data[6] & 0x7f;
    writeEnabled = data[7] != 0; chipSelected = data[8] != 0;
    clockHigh = (data[9] & 1) != 0; dataOut = (data[9] & 2) != 0;
    dataIn = (data[9] & 4) != 0;
    modified = false;
    return true;
}
