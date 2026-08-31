#include "sgb_protocol.h"

namespace sgb_protocol {

PacketHeader decodeHeader(uint8_t value) {
    PacketHeader header;
    header.command = value >> 3;
    header.packetCount = value & 7;
    header.valid = header.packetCount != 0 && header.command < 32;
    return header;
}

bool isImplementedCommand(uint8_t command) {
    // Commands 0x00-0x19 are represented by the current dispatch table.
    // 0x1A-0x1F are reserved and intentionally ignored.
    return command <= 0x19;
}

uint8_t boundedDataSndLength(uint8_t requested) {
    return requested > 11 ? 11 : requested;
}

} // namespace sgb_protocol
