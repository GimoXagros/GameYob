#pragma once

#include <stdint.h>

namespace romlayout {

const int MAX_ROM_BANKS = 0x200;
const int ROM_BANK_SIZE = 0x4000;

int bankCountForSize(uint64_t sizeBytes);
bool isSupportedSize(uint64_t sizeBytes);
int normalizeBank(int requestedBank, int bankCount);
int ramBankCount(uint8_t headerValue);

} // namespace romlayout
