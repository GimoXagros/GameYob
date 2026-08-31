#include "sgb_protocol.h"

#include <assert.h>

int main() {
    sgb_protocol::PacketHeader header = sgb_protocol::decodeHeader(0);
    assert(!header.valid && header.command == 0 && header.packetCount == 0);
    for (unsigned command = 0; command < 32; ++command) {
        for (unsigned count = 1; count <= 7; ++count) {
            header = sgb_protocol::decodeHeader((uint8_t)((command << 3) | count));
            assert(header.valid);
            assert(header.command == command);
            assert(header.packetCount == count);
        }
        assert(sgb_protocol::isImplementedCommand((uint8_t)command) ==
                (command <= 0x19));
    }
    assert(sgb_protocol::boundedDataSndLength(0) == 0);
    assert(sgb_protocol::boundedDataSndLength(11) == 11);
    assert(sgb_protocol::boundedDataSndLength(12) == 11);
    assert(sgb_protocol::boundedDataSndLength(255) == 11);
    return 0;
}
