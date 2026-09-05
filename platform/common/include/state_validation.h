#pragma once
#include <stddef.h>
#include <stdint.h>

// File layout is unchanged. Reject unsafe inputs before they reach mapMemory.
inline bool stateVersionSupported(int version, int latest) {
    return version >= 1 && version <= latest;
}

inline bool statePayloadFits(int fileSize, size_t offset, size_t length) {
    return fileSize >= 0 && offset <= (size_t)fileSize &&
        length <= (size_t)fileSize - offset;
}

inline bool stateMemoryBanksValid(int wramBank, int vramBank) {
    return wramBank >= 0 && wramBank < 8 && vramBank >= 0 && vramBank < 2;
}

inline bool stateBoolByteValid(unsigned value) { return value <= 1; }

inline bool stateSgbScalarsValid(int packetLength, int packetsTransferred,
        int packetBit, unsigned command, unsigned gfxMask,
        unsigned dataLength) {
    return packetLength >= 0 && packetLength <= 7 &&
        packetsTransferred >= 0 && packetsTransferred <= packetLength &&
        packetBit >= -1 && packetBit <= 128 && command < 32 &&
        gfxMask <= 3 && dataLength <= 11;
}

inline bool stateSgbHostHeaderValid(uint32_t magic, uint32_t version,
        size_t available, size_t expectedSize) {
    return magic == 0x53474248U && version == 1 && available >= expectedSize;
}

inline size_t stateStructBytesForVersion(int version, size_t version1Bytes,
        size_t version2Bytes, size_t currentBytes) {
    if (version <= 1) return version1Bytes;
    if (version == 2) return version2Bytes;
    return currentBytes;
}

inline bool stateAdvance(size_t* cursor, size_t amount, size_t fileSize) {
    if (!cursor || *cursor > fileSize || amount > fileSize - *cursor)
        return false;
    *cursor += amount;
    return true;
}

// Calculate the complete payload bound before the emulator's large RAM arrays
// are touched. Sizes are supplied by the native ABI so legacy raw structs keep
// their existing layout without duplicating the arrays in memory.
inline bool stateExpectedPayloadSize(size_t fileSize, int version,
        size_t fixedBytes, size_t stateBytes, size_t mapperBytes,
        bool sgbEnabled, size_t legacySgbBytes, size_t extendedSgbBytes,
        bool hostPresent, size_t hostBytes, size_t* expectedSize) {
    size_t cursor = 0;
    if (!stateAdvance(&cursor, fixedBytes, fileSize) ||
            !stateAdvance(&cursor, stateBytes, fileSize) ||
            !stateAdvance(&cursor, mapperBytes, fileSize))
        return false;
    if (version >= 3) {
        if (!stateAdvance(&cursor, sizeof(bool), fileSize))
            return false;
        if (sgbEnabled &&
                (!stateAdvance(&cursor, legacySgbBytes, fileSize) ||
                 (version >= 7 &&
                  (!stateAdvance(&cursor, extendedSgbBytes, fileSize) ||
                   (hostPresent &&
                    !stateAdvance(&cursor, hostBytes, fileSize))))))
            return false;
    }
    if (expectedSize) *expectedSize = cursor;
    return true;
}
