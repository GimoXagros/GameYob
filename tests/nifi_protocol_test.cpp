#include "nifi_protocol.h"

#include <assert.h>
#include <string.h>

static void testRoundTrip() {
    uint8_t packet[128];
    const uint8_t payload[] = {1, 2, 3, 4, 5};
    nifi::PacketHeader header;
    memset(&header, 0, sizeof(header));
    header.command = 7;
    header.flags = nifi::FLAG_ACK_REQUIRED;
    header.hostId = 0x12345678;
    header.sequence = 42;
    header.ackSequence = 41;
    header.payloadSize = sizeof(payload);
    header.fragmentCount = 1;
    header.totalSize = sizeof(payload);
    header.romId = 0xaabbccdd;

    const size_t size = nifi::encodePacket(packet, sizeof(packet), header, payload);
    assert(size == nifi::HEADER_SIZE + sizeof(payload));

    nifi::PacketView view;
    assert(nifi::decodePacket(packet, size, &view) == nifi::DECODE_OK);
    assert(view.header.sequence == 42);
    assert(view.header.ackSequence == 41);
    assert(view.header.romId == 0xaabbccdd);
    assert(memcmp(view.payload, payload, sizeof(payload)) == 0);
}

static void testKnownCrc32Vector() {
    static const uint8_t input[] = "123456789";
    assert(nifi::crc32(input, sizeof(input) - 1) == 0xcbf43926U);
}

static void testRejectsTruncationAndCorruption() {
    uint8_t packet[64];
    const uint8_t payload[] = {9, 8, 7};
    nifi::PacketHeader header;
    memset(&header, 0, sizeof(header));
    header.payloadSize = sizeof(payload);
    header.fragmentCount = 1;
    header.totalSize = sizeof(payload);
    const size_t size = nifi::encodePacket(packet, sizeof(packet), header, payload);

    nifi::PacketView view;
    assert(nifi::decodePacket(packet, size - 1, &view) == nifi::DECODE_BAD_LENGTH);
    packet[size - 1] ^= 0x80;
    assert(nifi::decodePacket(packet, size, &view) == nifi::DECODE_BAD_CHECKSUM);
}

static void testIdentityBounds() {
    uint8_t payload[128];
    const size_t size = nifi::encodeIdentity(payload, sizeof(payload), 1,
            0x10203040, "/gb/한글 게임.gbc", "한글 제목");
    assert(size > 12);

    char filename[64];
    char title[32];
    uint8_t linkType = 0;
    uint32_t romId = 0;
    assert(nifi::decodeIdentity(payload, size, &linkType, &romId,
            filename, sizeof(filename), title, sizeof(title)));
    assert(linkType == 1);
    assert(romId == 0x10203040);
    assert(strcmp(filename, "/gb/한글 게임.gbc") == 0);
    assert(strcmp(title, "한글 제목") == 0);

    assert(!nifi::decodeIdentity(payload, size, &linkType, &romId,
            filename, 4, title, sizeof(title)));
    assert(!nifi::decodeIdentity(payload, size - 1, &linkType, &romId,
            filename, sizeof(filename), title, sizeof(title)));
}

int main() {
    testRoundTrip();
    testKnownCrc32Vector();
    testRejectsTruncationAndCorruption();
    testIdentityBounds();
    return 0;
}
