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

static void testIncrementalRomIdentifier() {
    static const uint8_t first[] = {1, 2, 3};
    static const uint8_t second[] = {4, 5, 6};
    static const uint8_t combined[] = {1, 2, 3, 4, 5, 6};
    uint32_t identifier = nifi::romIdentifier(first, sizeof(first));
    identifier = nifi::romIdentifierUpdate(identifier, second,
                                            sizeof(second));
    assert(identifier == nifi::romIdentifier(combined, sizeof(combined)));
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
    for (size_t cut = 0; cut < size; ++cut)
        assert(nifi::decodePacket(packet, cut, &view) != nifi::DECODE_OK);
    for (size_t offset = 0; offset < size; ++offset) {
        uint8_t changed[64];
        memcpy(changed, packet, size);
        changed[offset] ^= 0x80;
        assert(nifi::decodePacket(changed, size, &view) != nifi::DECODE_OK);
    }
}

static void testSequenceAndAckBoundaries() {
    nifi::AckTracker ack;
    ack.begin(0xffff);
    assert(!ack.accept(0xfffe) && ack.waiting());
    assert(ack.accept(0xffff) && !ack.waiting());
    assert(!ack.accept(0xffff)); // duplicate/stale ACK
    ack.begin(0);
    assert(!ack.accept(0xffff) && ack.accept(0));
    assert(nifi::sequenceNewer(0, 0xffff));
    assert(nifi::sequenceNewer(1, 0xffff));
    assert(!nifi::sequenceNewer(0xffff, 0));
    assert(!nifi::sequenceNewer(7, 7));
    assert(!nifi::sequenceNewer(0x8000, 0));
}

static void testFragmentSequence() {
    nifi::FragmentSequence fragments;
    assert(fragments.accept(0x900, 3, 0, 0x400, 0x400) ==
        nifi::FRAGMENT_ACCEPTED);
    assert(fragments.accept(0x900, 3, 0, 0x400, 0x400) ==
        nifi::FRAGMENT_DUPLICATE);
    assert(fragments.accept(0x900, 3, 2, 0x100, 0x400) ==
        nifi::FRAGMENT_GAP);
    assert(fragments.accept(0x900, 3, 1, 0x400, 0x400) ==
        nifi::FRAGMENT_INVALID);
    assert(fragments.accept(0x900, 3, 0, 0x400, 0x400) ==
        nifi::FRAGMENT_ACCEPTED);
    assert(fragments.accept(0x900, 3, 1, 0x400, 0x400) ==
        nifi::FRAGMENT_ACCEPTED);
    assert(fragments.accept(0x900, 3, 2, 0x100, 0x400) ==
        nifi::FRAGMENT_COMPLETE);
    assert(fragments.accept(0, 1, 0, 0, 0x400) == nifi::FRAGMENT_INVALID);
    assert(fragments.accept(0x401, 1, 0, 0x401, 0x400) ==
        nifi::FRAGMENT_INVALID);
    assert(fragments.accept(0x400, 2, 0, 0x400, 0x400) ==
        nifi::FRAGMENT_INVALID);
}

static void testFixedSeedMutations() {
    uint8_t packet[128];
    uint8_t payload[64];
    for (size_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)i;
    nifi::PacketHeader header;
    memset(&header, 0, sizeof(header));
    header.payloadSize = sizeof(payload);
    header.fragmentCount = 1;
    header.totalSize = sizeof(payload);
    const size_t size = nifi::encodePacket(packet, sizeof(packet), header, payload);
    uint32_t seed = 0x91e10da5U;
    nifi::PacketView view;
    for (int i = 0; i < 4096; ++i) {
        uint8_t changed[128];
        memcpy(changed, packet, size);
        seed = seed * 1664525U + 1013904223U;
        changed[seed % size] ^= uint8_t(1U << ((seed >> 24) & 7));
        assert(nifi::decodePacket(changed, size, &view) != nifi::DECODE_OK);
    }
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
    nifi::FrameDeadline deadline(3);
    assert(!deadline.expired());
    assert(!deadline.advance());
    assert(!deadline.advance());
    assert(deadline.advance());
    assert(deadline.expired());
    assert(deadline.elapsed() == 3);

    testRoundTrip();
    testKnownCrc32Vector();
    testIncrementalRomIdentifier();
    testRejectsTruncationAndCorruption();
    testSequenceAndAckBoundaries();
    testFragmentSequence();
    testFixedSeedMutations();
    testIdentityBounds();
    return 0;
}
