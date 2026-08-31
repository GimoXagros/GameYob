#ifndef MBC7_EEPROM_H
#define MBC7_EEPROM_H

#include <stddef.h>
#include <stdint.h>

class Mbc7Eeprom {
public:
    enum Phase { COMMAND, READ_DATA, WRITE_DATA, WRITE_ALL_DATA };

    Mbc7Eeprom();
    void reset();
    void writePins(uint8_t value, uint8_t* storage, size_t storageSize);
    uint8_t readPins() const;
    bool consumeModified();

    bool isWriteEnabled() const { return writeEnabled; }
    Phase getPhase() const { return phase; }
    void save(uint8_t* data, size_t size) const;
    bool load(const uint8_t* data, size_t size);
    static size_t stateSize();
    static uint16_t sensorValue(int position, int neutralPosition,
            bool touching);

private:
    void beginCommand();
    void clockBit(bool bit, uint8_t* storage, size_t storageSize);
    void decodeCommand(uint8_t* storage, size_t storageSize);
    uint16_t readWord(const uint8_t* storage, size_t storageSize,
            uint8_t address) const;
    void writeWord(uint8_t* storage, size_t storageSize, uint8_t address,
            uint16_t value);

    Phase phase;
    uint16_t shift;
    uint16_t output;
    uint8_t bitCount;
    uint8_t address;
    bool writeEnabled;
    bool chipSelected;
    bool clockHigh;
    bool dataOut;
    bool dataIn;
    bool modified;
};

#endif
