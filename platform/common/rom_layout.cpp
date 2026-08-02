#include "rom_layout.h"

namespace romlayout {

int bankCountForSize(uint64_t sizeBytes) {
    if (sizeBytes == 0)
        return 0;
    return static_cast<int>((sizeBytes + ROM_BANK_SIZE - 1) / ROM_BANK_SIZE);
}

bool isSupportedSize(uint64_t sizeBytes) {
    const int banks = bankCountForSize(sizeBytes);
    return banks > 0 && banks <= MAX_ROM_BANKS;
}

int normalizeBank(int requestedBank, int bankCount) {
    if (bankCount <= 0 || requestedBank < 0)
        return -1;
    if (requestedBank < bankCount)
        return requestedBank;
    return requestedBank % bankCount;
}

int ramBankCount(uint8_t headerValue) {
    switch (headerValue) {
        case 0x00:
            return 0;
        case 0x01: // 2 KiB, mirrored in one 8 KiB bank
        case 0x02:
            return 1;
        case 0x03:
            return 4;
        case 0x04:
            return 16;
        case 0x05:
            return 8;
        default:
            return -1;
    }
}

} // namespace romlayout
