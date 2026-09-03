#pragma once
#include <stddef.h>

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
