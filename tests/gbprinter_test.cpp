#define _CRT_SECURE_NO_WARNINGS
#define GBPRINTER_TEST
#include "../platform/common/gbprinter.cpp"
#include <cassert>
#include <cstdarg>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static int icon = ICON_NULL;
static char lastLog[512];
void displayIcon(int value) { icon = value; }
void printLog(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(lastLog, sizeof(lastLog), format, args);
    va_end(args);
}

static unsigned le32(const std::vector<u8>& data, size_t offset) {
    return data.at(offset) | (unsigned(data.at(offset + 1)) << 8) |
           (unsigned(data.at(offset + 2)) << 16) |
           (unsigned(data.at(offset + 3)) << 24);
}
static std::vector<u8> bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    assert(file.good());
    return std::vector<u8>((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
}
static u8 sendPacket(u8 cmd, const std::vector<u8>& data = {},
                     u8 compressed = 0, bool validChecksum = true) {
    assert(data.size() < 65536);
    unsigned check = cmd + compressed + (data.size() & 255) + (data.size() >> 8);
    assert(sendGbPrinterByte(0x88) == 0);
    assert(sendGbPrinterByte(0x33) == 0);
    sendGbPrinterByte(cmd);
    sendGbPrinterByte(compressed);
    sendGbPrinterByte(static_cast<u8>(data.size()));
    sendGbPrinterByte(static_cast<u8>(data.size() >> 8));
    for (u8 b : data) { sendGbPrinterByte(b); check += b; }
    check = (check + (validChecksum ? 0 : 1)) & 0xffff;
    sendGbPrinterByte(static_cast<u8>(check));
    sendGbPrinterByte(static_cast<u8>(check >> 8));
    assert(sendGbPrinterByte(0) == 0x81);
    return sendGbPrinterByte(0);
}
static void finishPrint() {
    updateGbPrinter();
    assert(sendPacket(0x0f) == 0x06);
    for (int i = 0; i < 120; ++i) updateGbPrinter();
    assert(icon == ICON_NULL);
    assert(sendPacket(0x0f) == 0x04);
}

int main() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() /
        ("gameyob-printer-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(dir));
    const std::string base = (dir / "synthetic").string();
    printerSetTestBasename(base.c_str());
    initGbPrinter();
    assert(sendPacket(1) == 0);
    assert(sendPacket(0x0f) == 0);
    std::vector<u8> tile(16, 0);
    for (int i = 0; i < 16; i += 2) tile[i] = 0xff; // color index 1.
    assert(sendPacket(4, tile, 0, false) == 0);
    assert(imageSize == 0 && (status & CHECKSUM_ERROR));
    assert(sendPacket(4, tile) & CHECKSUM_ERROR);
    assert(imageSize == 16 && (status & READY));
    sendPacket(4, std::vector<u8>(16, 0)); // Split DATA, same sheet.
    assert(imageSize == 32);
    unsigned retained = imageSize;
    sendPacket(4, std::vector<u8>(16, 0xff), 0, false);
    assert(imageSize == retained && image[0] == 0xff);
    assert(sendPacket(2, {1, 0x13, 0xe4, 0x40}, 0, false) & READY);
    assert(printCounter == 0 && imageSize == retained);
    // A truncated RLE run and an oversized packet cannot alter prior DATA.
    sendPacket(4, {0x82}, 1);
    assert((status & PACKET_ERROR) && imageSize == retained);
    sendPacket(4, {0x7f, 0x12}, 1);
    assert((status & PACKET_ERROR) && imageSize == retained);
    sendPacket(4, {0xff}, 1); // Missing repeated byte is invalid.
    assert((status & PACKET_ERROR) && imageSize == retained);
    sendPacket(4, {0xff, 0x12}, 1); // Manual: 0xFF repeats 129 bytes.
    assert(imageSize == retained + 129);
    retained = imageSize;
    sendPacket(4, std::vector<u8>(641, 0));
    assert((status & PACKET_ERROR) && imageSize == retained);
    sendPacket(4);
    sendPacket(2, {2, 0, 0xe4, 0x40});
    assert((status & PACKET_ERROR) && printCounter == 0);
    sendPacket(4); // Empty DATA required before PRINT.
    sendPacket(2, {1, 0x13, 0xe4, 0x40});
    const fs::path first = dir / "synthetic-0.bmp";
    printerTestFailure = 1;
    updateGbPrinter();
    assert(!fs::exists(first) && imageSize == retained && (status & READY));
    assert(std::string(lastLog).find("write/flush") != std::string::npos);
    sendPacket(2, {1, 0x13, 0xe4, 0x40});
    printerTestFailure = 2;
    updateGbPrinter();
    assert(!fs::exists(first) && imageSize == retained);
    sendPacket(2, {1, 0x13, 0xe4, 0x40});
    printerTestFailure = 3;
    updateGbPrinter();
    assert(!fs::exists(first) && imageSize == retained);
    printerTestFailure = 0;
    sendPacket(2, {1, 0x13, 0xe4, 0x40});
    finishPrint();
    const auto bmp = bytes(first); // Independent BMP layout/pixel assertions.
    assert(bmp.size() == 70 + 160 * 16 / 2);
    assert(bmp[0] == 'B' && bmp[1] == 'M');
    assert(le32(bmp, 2) == bmp.size() && le32(bmp, 10) == 70);
    assert(le32(bmp, 18) == 160 && int32_t(le32(bmp, 22)) == -16);
    assert(bmp[28] == 4 && le32(bmp, 34) == 1280);
    assert(bmp[70] == 0x11 && bmp[71] == 0x11); // First tile row.
    assert(bmp[78] == 0 && bmp[70 + 80 * 8] == 0);
    assert(bmp[54] == 255 && bmp[58] == 170);
    // Existing output survives a later failed job. RLE literal then repeat.
    sendPacket(4, {0x00, 0xff, 0x80, 0x00}, 1);
    assert(imageSize == 3);
    sendPacket(4);
    sendPacket(2, {1, 0, 0xe4, 0x40});
    printerTestFailure = 2;
    updateGbPrinter();
    assert(bytes(first) == bmp && !fs::exists(dir / "synthetic-1.bmp"));
    printerTestFailure = 0;
    sendPacket(2, {1, 0, 0xe4, 0x40});
    finishPrint();
    assert(fs::exists(dir / "synthetic-1.bmp"));
    // ROM switch/reset owns a new basename and discards the old pending job.
    sendPacket(4, tile);
    const std::string secondBase = (dir / "next-rom").string();
    printerSetTestBasename(secondBase.c_str());
    initGbPrinter();
    assert(imageSize == 0);
    sendPacket(4, tile);
    sendPacket(4);
    sendPacket(2, {1, 0, 0xe4, 0x40});
    finishPrint();
    assert(fs::exists(dir / "next-rom-0.bmp"));
    assert(!fs::exists(dir / "synthetic-2.bmp"));
    // A missing output directory is distinct from a malformed packet.
    const std::string badBase = (dir / "missing" / "broken").string();
    printerSetTestBasename(badBase.c_str());
    initGbPrinter();
    sendPacket(4, tile);
    sendPacket(4);
    sendPacket(2, {1, 0, 0xe4, 0x40});
    updateGbPrinter();
    assert(imageSize == 16 && !fs::exists(dir / "missing"));
    assert(std::string(lastLog).find("open failed") != std::string::npos);
    // Incomplete packets time out without accepting partial image bytes.
    sendGbPrinterByte(0x88);
    sendGbPrinterByte(0x33);
    sendGbPrinterByte(4);
    for (int i = 0; i < 6; ++i) updateGbPrinter();
    assert(phase == 0 && imageSize == 16);
    // Captured relative output directory survives a later cwd change.
    const fs::path originalCwd = fs::current_path();
    fs::current_path(dir);
    printerSetTestBasename("relative");
    initGbPrinter();
    fs::current_path(originalCwd);
    sendPacket(4, tile);
    sendPacket(4);
    sendPacket(2, {1, 0, 0xe4, 0x40});
    finishPrint();
    assert(fs::exists(dir / "relative-0.bmp"));
    std::cout << "gbprinter_test PASS synthetic output: " << dir.string() << "\n";
}
