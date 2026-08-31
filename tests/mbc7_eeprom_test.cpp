#include "mbc7_eeprom.h"

#include <assert.h>
#include <string.h>

static void select(Mbc7Eeprom& e, uint8_t* data) {
    e.writePins(0x00, data, 256);
    e.writePins(0x80, data, 256);
}
static void bit(Mbc7Eeprom& e, uint8_t* data, bool value) {
    const uint8_t base = (uint8_t)(0x80 | (value ? 2 : 0));
    e.writePins(base, data, 256);
    e.writePins((uint8_t)(base | 0x40), data, 256);
    e.writePins(base, data, 256);
}
static void command(Mbc7Eeprom& e, uint8_t* data, unsigned value) {
    select(e, data);
    for (int i = 10; i >= 0; --i) bit(e, data, (value & (1u << i)) != 0);
}
static void writeBits(Mbc7Eeprom& e, uint8_t* data, uint16_t value) {
    for (int i = 15; i >= 0; --i) bit(e, data, (value & (1u << i)) != 0);
}
static uint16_t readBits(Mbc7Eeprom& e, uint8_t* data) {
    uint16_t result = 0;
    for (int i = 0; i < 16; ++i) {
        bit(e, data, false);
        result = (uint16_t)((result << 1) | (e.readPins() & 1));
    }
    return result;
}
static unsigned normalCommand(unsigned operation, unsigned address) {
    return 0x400 | ((operation & 3) << 8) | (address & 0x7f);
}
static unsigned specialCommand(unsigned subcommand) {
    return 0x400 | ((subcommand & 3) << 6);
}

int main() {
    assert(Mbc7Eeprom::sensorValue(0, 128, false) == 0x81d0);
    assert(Mbc7Eeprom::sensorValue(128, 128, true) == 0x81d0);
    assert(Mbc7Eeprom::sensorValue(0, 128, true) == 0x8240);
    assert(Mbc7Eeprom::sensorValue(255, 128, true) == 0x8161);
    assert(Mbc7Eeprom::sensorValue(0, 96, true) == 0x8240);
    assert(Mbc7Eeprom::sensorValue(191, 96, true) == 0x8162);

    uint8_t data[256];
    memset(data, 0xff, sizeof(data));
    Mbc7Eeprom e;
    command(e, data, normalCommand(1, 3));
    writeBits(e, data, 0x1234);
    assert(data[6] == 0xff && data[7] == 0xff);

    command(e, data, specialCommand(3));
    assert(e.isWriteEnabled());
    command(e, data, normalCommand(1, 3));
    writeBits(e, data, 0x1234);
    assert(data[6] == 0x34 && data[7] == 0x12);
    command(e, data, normalCommand(2, 3));
    assert(readBits(e, data) == 0x1234);

    command(e, data, normalCommand(3, 3));
    assert(data[6] == 0xff && data[7] == 0xff);
    command(e, data, specialCommand(1));
    writeBits(e, data, 0xa55a);
    assert(data[0] == 0x5a && data[1] == 0xa5);
    assert(data[254] == 0x5a && data[255] == 0xa5);
    command(e, data, specialCommand(2));
    for (unsigned i = 0; i < sizeof(data); ++i) assert(data[i] == 0xff);

    command(e, data, specialCommand(0));
    assert(!e.isWriteEnabled());
    uint8_t state[16] = {};
    e.save(state, sizeof(state));
    Mbc7Eeprom restored;
    assert(restored.load(state, sizeof(state)));
    assert(!restored.isWriteEnabled());
    assert(!restored.load(state, 2));
    return 0;
}
