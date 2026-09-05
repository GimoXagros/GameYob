#pragma once

#include <stddef.h>
#include <stdint.h>

namespace nifi {

class FrameDeadline {
public:
    explicit FrameDeadline(uint32_t frameLimit)
        : elapsedFrames(0), limit(frameLimit) {}

    bool expired() const { return limit != 0 && elapsedFrames >= limit; }
    bool advance() {
        if (elapsedFrames != ~static_cast<uint32_t>(0))
            ++elapsedFrames;
        return expired();
    }
    uint32_t elapsed() const { return elapsedFrames; }

private:
    uint32_t elapsedFrames;
    uint32_t limit;
};

class AckTracker {
public:
    AckTracker() : sequence(0), pending(false) {}
    void begin(uint16_t expected) { sequence = expected; pending = true; }
    bool accept(uint16_t received) {
        if (!pending || received != sequence) return false;
        pending = false;
        return true;
    }
    bool waiting() const { return pending; }
private:
    uint16_t sequence;
    bool pending;
};

inline bool sequenceNewer(uint16_t candidate, uint16_t reference) {
    const uint16_t distance = uint16_t(candidate - reference);
    return distance != 0 && distance < 0x8000;
}

enum {
    PROTOCOL_VERSION = 3,
    HEADER_SIZE = 32,
    FLAG_ACK_REQUIRED = 1,
    // A fragment packet contains 0x400 data bytes plus a 0x10 wrapper.
    MAX_PACKET_PAYLOAD = 0x410,
    MAX_FRAGMENT_COUNT = 255
};

struct PacketHeader {
    uint8_t command;
    uint8_t flags;
    uint32_t hostId;
    uint16_t sequence;
    uint16_t ackSequence;
    uint16_t payloadSize;
    uint8_t fragmentIndex;
    uint8_t fragmentCount;
    uint32_t totalSize;
    uint32_t romId;
};

struct PacketView {
    PacketHeader header;
    const uint8_t* payload;
};

enum DecodeResult {
    DECODE_OK = 0,
    DECODE_TOO_SHORT,
    DECODE_BAD_MAGIC,
    DECODE_BAD_VERSION,
    DECODE_BAD_HEADER_SIZE,
    DECODE_BAD_LENGTH,
    DECODE_BAD_FRAGMENT,
    DECODE_BAD_CHECKSUM
};

enum FragmentResult {
    FRAGMENT_ACCEPTED,
    FRAGMENT_COMPLETE,
    FRAGMENT_DUPLICATE,
    FRAGMENT_INVALID,
    FRAGMENT_GAP
};

class FragmentSequence {
public:
    FragmentSequence() { reset(); }
    void reset() { total = 0; count = next = 0; active = false; }
    FragmentResult accept(uint32_t totalSize, uint8_t fragmentCount,
            uint8_t fragmentIndex, size_t payloadSize,
            size_t fragmentSize);
private:
    uint32_t total;
    uint8_t count;
    uint8_t next;
    bool active;
};

uint32_t crc32(const uint8_t* data, size_t length);
uint32_t romIdentifier(const uint8_t* header, size_t length);
uint32_t romIdentifierUpdate(uint32_t identifier,
        const uint8_t* data, size_t length);

size_t encodePacket(uint8_t* output, size_t outputCapacity,
        const PacketHeader& header, const uint8_t* payload);
DecodeResult decodePacket(const uint8_t* packet, size_t packetLength,
        PacketView* output);

size_t encodeIdentity(uint8_t* output, size_t outputCapacity, uint8_t linkType,
        uint32_t romId, const char* filename, const char* romTitle);
bool decodeIdentity(const uint8_t* payload, size_t payloadLength,
        uint8_t* linkType, uint32_t* romId,
        char* filename, size_t filenameCapacity,
        char* romTitle, size_t romTitleCapacity);

} // namespace nifi
