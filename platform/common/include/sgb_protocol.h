#ifndef SGB_PROTOCOL_H
#define SGB_PROTOCOL_H

#include <stdint.h>

namespace sgb_protocol {

struct PacketHeader {
    uint8_t command;
    uint8_t packetCount;
    bool valid;
};

PacketHeader decodeHeader(uint8_t value);
bool isImplementedCommand(uint8_t command);
uint8_t boundedDataSndLength(uint8_t requested);

} // namespace sgb_protocol

#endif
