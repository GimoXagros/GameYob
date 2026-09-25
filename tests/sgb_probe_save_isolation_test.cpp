// Reuse the production-linked lifecycle harness's synthetic file and platform
// stubs. No user ROM, save, RTC, or printer path is opened by this test.
#define main lifecycle_harness_main
#include "romfile_bios_lifecycle_test.cpp"
#undef main
bool borderProbeForceAllocFailure = false;

static void makeProbeRom(const std::string& path, u8 ramCode) {
    writeFixture(path.c_str(), 0x10000, true, true);
    FILE* file = fopen(path.c_str(), "r+b");
    assert(file);
    fseek(file, 0x146, SEEK_SET); fputc(0x03, file);
    fseek(file, 0x14b, SEEK_SET); fputc(0x33, file);
    fseek(file, 0x147, SEEK_SET); fputc(0x13, file); // MBC3 + RTC.
    fseek(file, 0x149, SEEK_SET); fputc(ramCode, file);
    fclose(file);
}

static int checkProbe(const std::string& path, u8 ramCode,
        bool timeout, bool interrupted, bool cancelled) {
    makeProbeRom(path, ramCode);
    RomFile rom(path.c_str());
    Gameboy instance;
    gameboy = &instance;
    instance.setRomFile(&rom);
    if (instance.loadSave(-1) != 0) return 1;
    const int ramBytes = instance.getNumSramBanks() * 0x2000;
    if (ramBytes) {
        instance.externRam[0] = 0x5a;
        instance.externRam[ramBytes - 1] = 0x6b;
    }
    instance.gbClock.mbc3.s = 17;
    if (instance.saveGame() != 0) return 2;
    gbcModeOption = 2;
    sgbModeOption = 1;
    biosEnabled = 0;
    sgbBordersEnabled = true;
    probingForBorder = true;
    instance.init();
    if (!instance.sgbMode || !probingForBorder) return 3;
    if (ramBytes) {
        instance.writeSram(0, 0xa5);
        instance.externRam[ramBytes - 1] = 0xc7;
    }
    instance.gbClock.mbc3.s = 42;
    if (instance.saveGame() == 0) return 4;
    if (interrupted) {
        instance.unloadRom();
        gameboy = NULL;
        probingForBorder = false;
        return liveFiles == 0 ? 0 : 5;
    }
    if (cancelled) {
        sgbBordersEnabled = false;
        instance.updateVBlank();
    } else if (timeout) {
        for (int i = 0; i < 450; ++i)
            instance.updateVBlank();
    } else {
        probingForBorder = false; // PCT_TRN transitions before reset/init.
        instance.init();
    }
    if (instance.gbMode != CGB || instance.sgbMode || probingForBorder)
        return 6;
    if (ramBytes && (instance.externRam[0] != 0x5a ||
            instance.externRam[ramBytes - 1] != 0x6b)) {
        fprintf(stderr, "probe_to_cgb RAM mismatch code=%02x\n", ramCode);
        return 7;
    }
    if (instance.gbClock.mbc3.s != 17) return 8;
    instance.unloadRom();
    gameboy = NULL;
    return liveFiles == 0 ? 0 : 9;
}

int main(int argc, char** argv) {
    assert(argc == 1);
    const std::string stem = std::string(argv[0]) + ".probe";
    const u8 ramCodes[] = {0x00, 0x02, 0x04}; // 0, 8, 128 KiB.
    for (size_t i = 0; i < sizeof(ramCodes); ++i) {
        for (int exitPath = 0; exitPath < 4; ++exitPath) {
            const std::string path = stem + std::to_string(i) +
                std::to_string(exitPath) + ".gb";
            const int result = checkProbe(path, ramCodes[i],
                exitPath == 1, exitPath == 2, exitPath == 3);
            if (result) return 10 + result;
        }
    }
    const std::string failurePath = stem + "-alloc-failure.gb";
    makeProbeRom(failurePath, 0x04);
    RomFile rom(failurePath.c_str());
    Gameboy instance;
    gameboy = &instance;
    instance.setRomFile(&rom);
    if (instance.loadSave(-1) != 0) return 31;
    instance.externRam[0] = 0x5a;
    probingForBorder = true;
    borderProbeForceAllocFailure = true;
    instance.init();
    borderProbeForceAllocFailure = false;
    if (probingForBorder || instance.sgbMode || instance.gbMode != CGB ||
            instance.externRam[0] != 0x5a)
        return 32;
    instance.unloadRom();
    gameboy = NULL;
    return 0;
}
