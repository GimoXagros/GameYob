#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef GBPRINTER_TEST
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
typedef unsigned char u8;
#define MAX_FILENAME_LEN 768
enum { ICON_NULL, ICON_PRINTER };
void displayIcon(int);
void printLog(const char*, ...);
#else
#include <unistd.h>
#include "gameboy.h"
#include "gbprinter.h"
#include "gbgfx.h"
#include "console.h"
#include "romfile.h"
#endif

// Protocol: Pan Docs e196504 (2026-04-19). Printable RAM is 160x200/4.
enum { WIDTH = 160, MAX_IMAGE = 8000, MAX_PACKET = 0x280 };
enum { READY = 0x08, REQUESTED = 0x04, PRINTING = 0x02,
       CHECKSUM_ERROR = 0x01, PACKET_ERROR = 0x10 };
static u8 image[MAX_IMAGE], packet[MAX_PACKET];
static unsigned imageSize, packetSize, packetRead, checksum, expectedChecksum, phase;
static unsigned packetIdleFrames;
static u8 command, compression, status, palette;
static bool packetInvalid, emptyDataReceived;
static int printCounter, nextNumber;
static char outputBase[MAX_FILENAME_LEN];
static void makeOutputPathStable() {
    // Sidecar basenames may be relative (notably a FAT 8.3 fallback). Pin
    // their directory when the ROM is selected, before a menu changes cwd.
    if (!outputBase[0] || outputBase[0] == '/' || strchr(outputBase, ':'))
        return;
    char cwd[MAX_FILENAME_LEN], full[MAX_FILENAME_LEN];
#if defined(GBPRINTER_TEST) && defined(_WIN32)
    char* resolved = _getcwd(cwd, sizeof(cwd));
#else
    char* resolved = getcwd(cwd, sizeof(cwd));
#endif
    if (!resolved) { outputBase[0] = '\0'; return; }
    const int n = snprintf(full, sizeof(full), "%s/%s", cwd, outputBase);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(full))
        { outputBase[0] = '\0'; return; }
    memcpy(outputBase, full, static_cast<size_t>(n) + 1);
}
#ifdef GBPRINTER_TEST
static const char* printerTestBasename;
void printerSetTestBasename(const char* name) { printerTestBasename = name; }
static int printerTestFailure; // 1: short write, 2: flush, 3: rename
#endif
static size_t checkedWrite(const void* data, size_t size, FILE* f) {
#ifdef GBPRINTER_TEST
    if (printerTestFailure == 1) return 0;
#endif
    return fwrite(data, 1, size, f);
}
static int checkedFlush(FILE* f) {
#ifdef GBPRINTER_TEST
    if (printerTestFailure == 2) return EOF;
#endif
    return fflush(f);
}
static int checkedRename(const char* from, const char* to) {
#ifdef GBPRINTER_TEST
    if (printerTestFailure == 3) return -1;
#endif
    return rename(from, to);
}

static void clearPacket() {
    phase = packetSize = packetRead = checksum = expectedChecksum = 0;
    packetIdleFrames = 0;
    packetInvalid = false;
}
static void clearImage() {
    if (status & PRINTING) displayIcon(ICON_NULL);
    imageSize = 0;
    memset(image, 0, sizeof(image));
    emptyDataReceived = false;
    status = 0;
    printCounter = 0;
}
void initGbPrinter() {
    clearPacket();
    clearImage();
    nextNumber = 0;
    palette = 0xe4;
    outputBase[0] = '\0';
#ifdef GBPRINTER_TEST
    if (printerTestBasename)
        snprintf(outputBase, sizeof(outputBase), "%s", printerTestBasename);
#else
    if (gameboy && gameboy->getRomFile()) {
        const char* base = gameboy->getRomFile()->getStorageBasename();
        if (base && snprintf(outputBase, sizeof(outputBase), "%s", base) >=
                static_cast<int>(sizeof(outputBase)))
            outputBase[0] = '\0';
    }
#endif
    makeOutputPathStable();
}
static bool decodeData(u8* result, unsigned capacity, unsigned* resultSize) {
    unsigned out = 0;
    if (!compression) {
        if (packetSize > capacity) return false;
        if (result) memcpy(result, packet, packetSize);
        *resultSize = packetSize;
        return true;
    }
    for (unsigned pos = 0; pos < packetSize;) {
        const u8 control = packet[pos++];
        if (control < 0x80) {
            const unsigned count = control + 1;
            if (count > packetSize - pos || count > capacity - out) return false;
            if (result) memcpy(result + out, packet + pos, count);
            pos += count;
            out += count;
        }
        else {
            // Nintendo's printer manual includes 0xFF: repeat 129 bytes.
            if (pos == packetSize) return false;
            const unsigned count = control - 0x80 + 2;
            if (count > capacity - out) return false;
            if (result) memset(result + out, packet[pos], count);
            ++pos;
            out += count;
        }
    }
    *resultSize = out;
    return true;
}
static void executePacket() {
    if (checksum != expectedChecksum) {
        status |= CHECKSUM_ERROR;
        printLog("Printer checksum mismatch (%04x/%04x)\n", checksum, expectedChecksum);
        return;
    }
    status &= ~CHECKSUM_ERROR;
    if (packetInvalid || compression > 1) {
        status |= PACKET_ERROR;
        printLog("Printer packet length/compression error\n");
        return;
    }
    if (command == 1 && packetSize == 0) {
        clearImage();
        return;
    }
    if (command == 4) {
        if (printCounter) {
            status |= PACKET_ERROR;
            printLog("Printer DATA received while printing\n");
            return;
        }
        // Validate the entire encoded stream and the remaining printer RAM
        // before writing any pixel bytes. Compressed DATA may expand beyond
        // the 640-byte packet limit without exceeding the 8 KiB image RAM.
        unsigned decodedSize = 0;
        const unsigned remaining = MAX_IMAGE - imageSize;
        if (!decodeData(NULL, remaining, &decodedSize)) {
            status |= PACKET_ERROR;
            printLog("Printer DATA decode or image overflow\n");
            return;
        }
        if (decodedSize) {
            unsigned committedSize = 0;
            if (!decodeData(image + imageSize, remaining, &committedSize) ||
                    committedSize != decodedSize) {
                status |= PACKET_ERROR;
                printLog("Printer DATA second-pass mismatch\n");
                return;
            }
            imageSize += decodedSize;
            emptyDataReceived = false;
            status |= READY;
        }
        else emptyDataReceived = true;
        status &= ~PACKET_ERROR;
        return;
    }
    if (command == 2 && packetSize == 4) {
        const u8 sheets = packet[0];
        if (sheets == 0) { // Line-feed only, no image output.
            status &= ~PACKET_ERROR;
            return;
        }
        if (sheets != 1 || !imageSize || !emptyDataReceived || printCounter) {
            status |= PACKET_ERROR;
            printLog("Printer PRINT unsupported copies or incomplete image\n");
            return;
        }
        palette = packet[2];
        status &= ~PACKET_ERROR;
        printCounter = 1; // Never perform filesystem I/O in serial completion.
        return;
    }
    if (command == 0x0f && packetSize == 0) {
        status &= ~PACKET_ERROR;
        return;
    }
    status |= PACKET_ERROR;
    printLog("Printer unsupported command or length (%02x/%u)\n", command, packetSize);
}
u8 sendGbPrinterByte(u8 value) {
    u8 answer = 0;
    packetIdleFrames = 0;
    switch (phase) {
    case 0:
        if (value == 0x88) phase = 1;
        return 0;
    case 1:
        phase = value == 0x33 ? 2 : value == 0x88 ? 1 : 0;
        return 0;
    case 2: command = value; checksum = value; phase = 3; break;
    case 3: compression = value; checksum = (checksum + value) & 0xffff; phase = 4; break;
    case 4: packetSize = value; checksum = (checksum + value) & 0xffff; phase = 5; break;
    case 5:
        packetSize |= static_cast<unsigned>(value) << 8;
        checksum = (checksum + value) & 0xffff;
        packetRead = 0;
        packetInvalid = packetSize > MAX_PACKET;
        phase = packetSize ? 6 : 7;
        break;
    case 6:
        checksum = (checksum + value) & 0xffff;
        if (packetRead < MAX_PACKET) packet[packetRead] = value;
        if (++packetRead == packetSize) phase = 7;
        break;
    case 7: expectedChecksum = value; phase = 8; break;
    case 8: expectedChecksum |= static_cast<unsigned>(value) << 8; phase = 9; break;
    case 9: answer = 0x81; phase = 10; break;
    case 10:
        answer = status; // Command takes effect after this response byte.
        executePacket();
        clearPacket();
        return answer;
    }
    return answer;
}
static void put32(u8* p, unsigned n) {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<u8>(n >> (i * 8));
}
static bool saveImage() {
    if (!outputBase[0]) {
        printLog("Printer output path unavailable\n");
        return false;
    }
    // Every PRINT is numbered; existing images are never opened for writing.
    char destination[MAX_FILENAME_LEN + 32] = {0};
    char temporary[MAX_FILENAME_LEN + 40];
    for (; nextNumber < 100000; ++nextNumber) {
        const int n = snprintf(destination, sizeof(destination), "%s-%d.bmp",
                               outputBase, nextNumber);
        if (n < 0 || static_cast<size_t>(n) >= sizeof(destination)) break;
        struct stat info;
        if (stat(destination, &info) == 0) continue;
        if (errno == ENOENT) break;
        printLog("Printer output path check failed (%d)\n", errno);
        return false;
    }
    if (nextNumber == 100000 || !destination[0] ||
            strlen(destination) >= MAX_FILENAME_LEN) {
        printLog("Printer output path too long or no free number\n");
        return false;
    }
    const int tn = snprintf(temporary, sizeof(temporary), "%s.tmp", destination);
    if (tn < 0 || static_cast<size_t>(tn) >= sizeof(temporary) ||
            strlen(temporary) >= MAX_FILENAME_LEN) {
        printLog("Printer temporary path too long\n");
        return false;
    }
    struct stat info;
    if (stat(temporary, &info) == 0 || errno != ENOENT) {
        printLog("Printer temporary output exists or cannot be checked\n");
        return false;
    }
    const unsigned rows = (imageSize + 39) / 40;
    const unsigned height = (rows + 15) / 16 * 16;
    const unsigned pixelSize = WIDTH * height / 2;
    if (!height || height > 208 || pixelSize > 16640) {
        printLog("Printer image dimensions invalid\n");
        return false;
    }
    FILE* f = fopen(temporary, "wb");
    if (!f) {
        printLog("Printer output open failed (%d)\n", errno);
        return false;
    }
    u8 header[70] = {0};
    header[0] = 'B'; header[1] = 'M';
    put32(header + 2, sizeof(header) + pixelSize);
    put32(header + 10, sizeof(header));
    put32(header + 14, 40);
    put32(header + 18, WIDTH);
    put32(header + 22, static_cast<unsigned>(-static_cast<int>(height)));
    header[26] = 1; header[28] = 4;
    put32(header + 34, pixelSize);
    put32(header + 38, 2834); put32(header + 42, 2834);
    put32(header + 46, 4);
    for (unsigned i = 0; i < 4; ++i) {
        const u8 shade = static_cast<u8>(255 - 85 * ((palette >> (i * 2)) & 3));
        header[54 + i * 4] = header[55 + i * 4] = header[56 + i * 4] = shade;
    }
    bool ok = checkedWrite(header, sizeof(header), f) == sizeof(header);
    u8 row[80];
    for (unsigned y = 0; ok && y < height; ++y) {
        memset(row, 0, sizeof(row));
        for (unsigned x = 0; x < WIDTH; ++x) {
            const unsigned tile = (y / 8) * 20 + x / 8;
            const unsigned offset = tile * 16 + (y & 7) * 2;
            u8 color = 0;
            if (offset + 1 < imageSize) {
                const unsigned bit = 7 - (x & 7);
                color = ((image[offset] >> bit) & 1) |
                        (((image[offset + 1] >> bit) & 1) << 1);
            }
            if (x & 1) row[x / 2] |= color;
            else row[x / 2] = color << 4;
        }
        ok = checkedWrite(row, sizeof(row), f) == sizeof(row);
    }
    if (ok) ok = checkedFlush(f) == 0;
    if (fclose(f) != 0) ok = false;
    // A second collision check avoids replacing an image created while this
    // staged file was being written. No existing destination is opened.
    if (ok && (stat(destination, &info) == 0 || errno != ENOENT))
        ok = false;
    if (ok) ok = checkedRename(temporary, destination) == 0;
    if (!ok) {
        printLog("Printer output write/flush/close/rename failed (%d)\n", errno);
        remove(temporary);
        return false;
    }
    ++nextNumber;
    printLog("Printer image saved: %s\n", destination);
    return true;
}
void updateGbPrinter() {
    if (phase && ++packetIdleFrames >= 6) {
        clearPacket(); // Abandoned/incomplete packet, measured in guest VBlanks.
        printLog("Printer serial packet timeout\n");
    }
    if (!printCounter || --printCounter) return;
    if (status & PRINTING) {
        status &= ~PRINTING;
        displayIcon(ICON_NULL);
        return;
    }
    const unsigned printedHeight = ((imageSize + 39) / 40 + 15) / 16 * 16;
    if (!saveImage()) return; // Data retained for an explicit PRINT retry.
    imageSize = 0;
    emptyDataReceived = false;
    status = (status & ~(READY | PACKET_ERROR)) | REQUESTED | PRINTING;
    displayIcon(ICON_PRINTER);
    // Retain the legacy height-based busy interval in guest VBlanks. This is
    // an emulator approximation, not a measured physical print duration.
    printCounter = printedHeight;
}
