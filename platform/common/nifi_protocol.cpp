#include "nifi_protocol.h"

#include <string.h>

namespace {

uint16_t read16(const uint8_t* input) {
    return (uint16_t)(input[0] | ((uint16_t)input[1] << 8));
}

uint32_t read32(const uint8_t* input) {
    return (uint32_t)input[0] |
        ((uint32_t)input[1] << 8) |
        ((uint32_t)input[2] << 16) |
        ((uint32_t)input[3] << 24);
}

void write16(uint8_t* output, uint16_t value) {
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8);
}

void write32(uint8_t* output, uint32_t value) {
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8);
    output[2] = (uint8_t)(value >> 16);
    output[3] = (uint8_t)(value >> 24);
}

size_t boundedLength(const char* text, size_t maximum) {
    if (!text)
        return 0;
    size_t length = 0;
    while (length < maximum && text[length])
        ++length;
    return length;
}

} // namespace

namespace nifi {

uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (uint32_t)-(int)(crc & 1));
    }
    return ~crc;
}

uint32_t romIdentifier(const uint8_t* header, size_t length) {
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < length; ++i) {
        hash ^= header[i];
        hash *= 16777619U;
    }
    return hash;
}

size_t encodePacket(uint8_t* output, size_t outputCapacity,
        const PacketHeader& header, const uint8_t* payload) {
    const size_t packetSize = HEADER_SIZE + header.payloadSize;
    if (!output || outputCapacity < packetSize)
        return 0;
    if (header.payloadSize && !payload)
        return 0;
    if (!header.fragmentCount || header.fragmentIndex >= header.fragmentCount)
        return 0;
    if (header.fragmentCount == 1 &&
            (header.fragmentIndex != 0 || header.totalSize != header.payloadSize))
        return 0;

    memset(output, 0, HEADER_SIZE);
    output[0] = 'Y';
    output[1] = 'O';
    output[2] = 'B';
    output[3] = '2';
    output[4] = PROTOCOL_VERSION;
    output[5] = header.command;
    output[6] = header.flags;
    output[7] = HEADER_SIZE;
    write32(output + 8, header.hostId);
    write16(output + 12, header.sequence);
    write16(output + 14, header.ackSequence);
    write16(output + 16, header.payloadSize);
    output[18] = header.fragmentIndex;
    output[19] = header.fragmentCount;
    write32(output + 20, header.totalSize);
    write32(output + 24, header.romId);
    if (header.payloadSize)
        memcpy(output + HEADER_SIZE, payload, header.payloadSize);
    write32(output + 28, crc32(output, packetSize));
    return packetSize;
}

DecodeResult decodePacket(const uint8_t* packet, size_t packetLength,
        PacketView* output) {
    if (!packet || !output || packetLength < HEADER_SIZE)
        return DECODE_TOO_SHORT;
    if (packet[0] != 'Y' || packet[1] != 'O' ||
            packet[2] != 'B' || packet[3] != '2')
        return DECODE_BAD_MAGIC;
    if (packet[4] != PROTOCOL_VERSION)
        return DECODE_BAD_VERSION;
    if (packet[7] != HEADER_SIZE)
        return DECODE_BAD_HEADER_SIZE;

    PacketHeader header;
    header.command = packet[5];
    header.flags = packet[6];
    header.hostId = read32(packet + 8);
    header.sequence = read16(packet + 12);
    header.ackSequence = read16(packet + 14);
    header.payloadSize = read16(packet + 16);
    header.fragmentIndex = packet[18];
    header.fragmentCount = packet[19];
    header.totalSize = read32(packet + 20);
    header.romId = read32(packet + 24);

    if ((size_t)header.payloadSize != packetLength - HEADER_SIZE)
        return DECODE_BAD_LENGTH;
    if (!header.fragmentCount || header.fragmentIndex >= header.fragmentCount)
        return DECODE_BAD_FRAGMENT;
    if (header.fragmentCount == 1 &&
            (header.fragmentIndex != 0 || header.totalSize != header.payloadSize))
        return DECODE_BAD_FRAGMENT;

    uint8_t headerCopy[HEADER_SIZE];
    memcpy(headerCopy, packet, HEADER_SIZE);
    const uint32_t expectedCrc = read32(headerCopy + 28);
    memset(headerCopy + 28, 0, 4);

    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i < HEADER_SIZE; ++i) {
        crc ^= headerCopy[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (uint32_t)-(int)(crc & 1));
    }
    for (size_t i = HEADER_SIZE; i < packetLength; ++i) {
        crc ^= packet[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (uint32_t)-(int)(crc & 1));
    }
    if (~crc != expectedCrc)
        return DECODE_BAD_CHECKSUM;

    output->header = header;
    output->payload = packet + HEADER_SIZE;
    return DECODE_OK;
}

size_t encodeIdentity(uint8_t* output, size_t outputCapacity, uint8_t linkType,
        uint32_t romId, const char* filename, const char* romTitle) {
    const size_t filenameLength = boundedLength(filename, 0xffff);
    const size_t titleLength = boundedLength(romTitle, 0xff);
    const size_t totalLength = 12 + filenameLength + titleLength;
    if (!output || outputCapacity < totalLength)
        return 0;

    memset(output, 0, 12);
    output[0] = linkType;
    write32(output + 4, romId);
    write16(output + 8, (uint16_t)filenameLength);
    output[10] = (uint8_t)titleLength;
    if (filenameLength)
        memcpy(output + 12, filename, filenameLength);
    if (titleLength)
        memcpy(output + 12 + filenameLength, romTitle, titleLength);
    return totalLength;
}

bool decodeIdentity(const uint8_t* payload, size_t payloadLength,
        uint8_t* linkType, uint32_t* romId,
        char* filename, size_t filenameCapacity,
        char* romTitle, size_t romTitleCapacity) {
    if (!payload || payloadLength < 12 || !filename || !romTitle ||
            !filenameCapacity || !romTitleCapacity)
        return false;

    const size_t filenameLength = read16(payload + 8);
    const size_t titleLength = payload[10];
    if (filenameLength + titleLength > payloadLength - 12)
        return false;
    if (filenameLength >= filenameCapacity || titleLength >= romTitleCapacity)
        return false;

    if (linkType)
        *linkType = payload[0];
    if (romId)
        *romId = read32(payload + 4);
    memcpy(filename, payload + 12, filenameLength);
    filename[filenameLength] = '\0';
    memcpy(romTitle, payload + 12 + filenameLength, titleLength);
    romTitle[titleLength] = '\0';
    return true;
}

} // namespace nifi
