// Synthetic ROM with the production Gameboy/MMU serial scheduler and parser.
// The FileHandle/platform boundaries come from the lifecycle harness.
#define PRINTER_SERIAL_TEST
#define main lifecycle_harness_main
#include "romfile_bios_lifecycle_test.cpp"
#undef main
#include <vector>
extern Gameboy* gbUno;

static int exchange(Gameboy& gb, u8 sent, u8 expected) {
    gb.ioRam[0x0f] &= ~INT_SERIAL;
    gb.writeIO(0x01, sent); // FF01/SB
    gb.writeIO(0x02, 0x81); // FF02/SC, internal clock + start
    if (gb.serialCounter != clockSpeed / 1024) return 1;
    gb.runEmul();
    if (gb.serialCounter || (gb.ioRam[0x02] & 0x80) ||
            !(gb.ioRam[0x0f] & INT_SERIAL) || gb.ioRam[0x01] != expected)
        return 2;
    // Another guest frame must not deliver the completed byte twice.
    gb.ioRam[0x0f] &= ~INT_SERIAL;
    gb.runEmul();
    if (gb.ioRam[0x0f] & INT_SERIAL) return 3;
    return 0;
}

static int packet(Gameboy& gb, u8 command, const std::vector<u8>& data,
        int expectedStatus) {
    unsigned checksum = command + (data.size() & 255) + (data.size() >> 8);
    std::vector<u8> frame = {0x88, 0x33, command, 0,
        static_cast<u8>(data.size()), static_cast<u8>(data.size() >> 8)};
    for (u8 value : data) { frame.push_back(value); checksum += value; }
    frame.push_back(static_cast<u8>(checksum));
    frame.push_back(static_cast<u8>(checksum >> 8));
    frame.push_back(0); frame.push_back(0);
    for (size_t i = 0; i < frame.size(); ++i) {
        const int response = i == frame.size() - 2 ? 0x81 :
            i == frame.size() - 1 ? expectedStatus : 0;
        const int result = exchange(gb, frame[i], response);
        if (result) return 10 + result;
    }
    return 0;
}

static int runCase(const std::string& path, bool color) {
    writeFixture(path.c_str(), 0x8000, true, color);
    FILE* fixture = fopen(path.c_str(), "r+b");
    assert(fixture);
    fseek(fixture, 0x100, SEEK_SET);
    fputc(0x18, fixture); fputc(0xfe, fixture); // JR -2, stable guest loop.
    fclose(fixture);
    printerEnabled = true;
    gbcModeOption = color ? 2 : 0;
    sgbModeOption = 0;
    biosEnabled = 0;
    RomFile rom(path.c_str());
    Gameboy gb;
    gameboy = &gb;
    gbUno = &gb;
    hostGb = &gb;
    gb.setRomFile(&rom);
    gb.init();
    gb.ioRam[0xff] = 0; // Keep the serial IRQ pending for assertion.
    int result = packet(gb, 1, {}, 0); // INIT.
    if (!result) result = packet(gb, 0x0f, {}, 0); // STATUS.
    std::vector<u8> tile(16, 0);
    for (int i = 0; i < 16; i += 2) tile[i] = 0xff;
    if (!result) result = packet(gb, 4, tile, 0); // DATA.
    if (!result) result = packet(gb, 4, {}, 0x08); // DATA end.
    if (!result) result = packet(gb, 2, {1, 0x13, 0xe4, 0x40}, 0x08);
    for (int i = 0; !result && i < 120; ++i) gb.updateVBlank();
    const std::string bmpPath = std::string(rom.getStorageBasename()) + "-0.bmp";
    FILE* bmp = fopen(bmpPath.c_str(), "rb");
    if (!bmp) result = 20;
    else {
        unsigned char header[72] = {};
        if (fread(header, 1, sizeof(header), bmp) != sizeof(header) ||
                header[0] != 'B' || header[1] != 'M' ||
                header[18] != 160 || header[22] != 0xf0 ||
                header[70] != 0x11 || header[71] != 0x11)
            result = 21;
        fclose(bmp);
    }
    gb.unloadRom();
    gameboy = NULL;
    gbUno = NULL;
    hostGb = NULL;
    return result;
}

int main(int argc, char** argv) {
    assert(argc == 1);
    const std::string stem = std::string(argv[0]) + ".serial";
    int result = runCase(stem + "-gb.gb", false);
    if (!result) result = runCase(stem + "-cgb.gbc", true);
    return result;
}
