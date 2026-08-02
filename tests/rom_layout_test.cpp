#include "rom_layout.h"

#include <assert.h>

static void testFileSizeBankCount() {
    assert(romlayout::bankCountForSize(0x8000) == 2);
    assert(romlayout::bankCountForSize(72ULL * 0x4000) == 72);
    assert(romlayout::bankCountForSize(0x8001) == 3);
}

static void testHardwareSizeBoundary() {
    assert(romlayout::isSupportedSize(8ULL * 1024 * 1024));
    assert(!romlayout::isSupportedSize(8ULL * 1024 * 1024 + 1));
}

static void testNonPowerOfTwoMirroring() {
    assert(romlayout::normalizeBank(71, 72) == 71);
    assert(romlayout::normalizeBank(72, 72) == 0);
    assert(romlayout::normalizeBank(79, 72) == 7);
}

static void testRamHeaderSizes() {
    assert(romlayout::ramBankCount(0x00) == 0);
    assert(romlayout::ramBankCount(0x01) == 1);
    assert(romlayout::ramBankCount(0x03) == 4);
    assert(romlayout::ramBankCount(0x04) == 16);
    assert(romlayout::ramBankCount(0x05) == 8);
    assert(romlayout::ramBankCount(0xff) == -1);
}

int main() {
    testFileSizeBankCount();
    testHardwareSizeBoundary();
    testNonPowerOfTwoMirroring();
    testRamHeaderSizes();
    return 0;
}
