// Reuse the production-linked lifecycle harness's synthetic file and platform
// stubs. No user ROM, save, RTC, or printer path is opened by this test.
#define main lifecycle_harness_main
#include "romfile_bios_lifecycle_test.cpp"
#undef main

int main(int argc, char** argv) {
    assert(argc == 1);
    const std::string path = std::string(argv[0]) + ".probe.gb";
    writeFixture(path.c_str(), 0x10000, true, true);
    FILE* fixture = fopen(path.c_str(), "r+b");
    assert(fixture);
    fseek(fixture, 0x146, SEEK_SET);
    fputc(0x03, fixture); // SGB-enhanced dual-mode synthetic cartridge.
    fseek(fixture, 0x14b, SEEK_SET);
    fputc(0x33, fixture);
    fseek(fixture, 0x147, SEEK_SET);
    fputc(0x13, fixture); // MBC3 + RAM + RTC.
    fclose(fixture);

    RomFile rom(path.c_str());
    Gameboy instance;
    gameboy = &instance;
    instance.setRomFile(&rom);
    assert(instance.loadSave(-1) == 0); // In-memory only; no output save.
    assert(instance.externRam);
    instance.externRam[0] = 0x5a;
    gbcModeOption = 2;
    sgbModeOption = 1;
    biosEnabled = 0;
    sgbBordersEnabled = true;
    probingForBorder = true;
    instance.init();
    assert(instance.sgbMode && probingForBorder);

    // A temporary border-search boot must not become part of the actual save.
    instance.writeSram(0, 0xa5);
    if (instance.externRam[0] != 0xa5) return 2;
    probingForBorder = false; // The same transition used by PCT_TRN/timeout.
    instance.init();
    if (instance.gbMode != CGB || instance.sgbMode) return 3;
    if (instance.externRam[0] != 0x5a) {
        fprintf(stderr,
            "first mismatch phase=probe_to_cgb SRAM expected=5a actual=%02x\n",
            instance.externRam[0]);
        return 4;
    }
    instance.unloadRom();
    gameboy = NULL;
    return 0;
}
