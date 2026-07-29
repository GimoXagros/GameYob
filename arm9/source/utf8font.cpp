#include <nds.h>
#include <string.h>

#include "console.h"
#include "utf8font.h"
#include "hangul_font_bin.h"
#include "cp949_table_bin.h"

namespace {

const unsigned int FIRST_DYNAMIC_TILE = 128;
const unsigned int DYNAMIC_TILE_COUNT = 896;
const unsigned int INVALID_CODEPOINT = 0xFFFD;

u16 cachedCodepoints[DYNAMIC_TILE_COUNT];
unsigned int nextDynamicTile = 0;

bool decodeUtf8Character(const unsigned char* text, unsigned int remaining,
                         unsigned int* codepoint, unsigned int* byteCount) {
    const unsigned char first = text[0];
    if (first < 0x80) {
        *codepoint = first;
        *byteCount = 1;
        return true;
    }
    if (first >= 0xC2 && first <= 0xDF && remaining >= 2 &&
            (text[1] & 0xC0) == 0x80) {
        *codepoint = ((first & 0x1F) << 6) | (text[1] & 0x3F);
        *byteCount = 2;
        return true;
    }
    if (first >= 0xE0 && first <= 0xEF && remaining >= 3 &&
            (text[1] & 0xC0) == 0x80 && (text[2] & 0xC0) == 0x80 &&
            !(first == 0xE0 && text[1] < 0xA0) &&
            !(first == 0xED && text[1] >= 0xA0)) {
        *codepoint = ((first & 0x0F) << 12) |
                     ((text[1] & 0x3F) << 6) | (text[2] & 0x3F);
        *byteCount = 3;
        return true;
    }
    if (first >= 0xF0 && first <= 0xF4 && remaining >= 4 &&
            (text[1] & 0xC0) == 0x80 && (text[2] & 0xC0) == 0x80 &&
            (text[3] & 0xC0) == 0x80 &&
            !(first == 0xF0 && text[1] < 0x90) &&
            !(first == 0xF4 && text[1] >= 0x90)) {
        *codepoint = ((first & 0x07) << 18) |
                     ((text[1] & 0x3F) << 12) |
                     ((text[2] & 0x3F) << 6) | (text[3] & 0x3F);
        *byteCount = 4;
        return true;
    }
    return false;
}

bool isValidUtf8(const char* text) {
    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text);
    unsigned int remaining = strlen(text);
    while (remaining) {
        unsigned int codepoint;
        unsigned int byteCount;
        if (!decodeUtf8Character(cursor, remaining, &codepoint, &byteCount))
            return false;
        cursor += byteCount;
        remaining -= byteCount;
    }
    return true;
}

unsigned int decodeCp949Character(const unsigned char* text,
                                  unsigned int remaining,
                                  unsigned int* byteCount) {
    if (text[0] < 0x80) {
        *byteCount = 1;
        return text[0];
    }
    if (remaining >= 2 && text[0] >= 0x81 && text[0] <= 0xFE) {
        const unsigned int index = (text[0] - 0x81) * 256 + text[1];
        const unsigned int codepoint =
            cp949_table_bin[index * 2] | (cp949_table_bin[index * 2 + 1] << 8);
        if (codepoint) {
            *byteCount = 2;
            return codepoint;
        }
    }
    *byteCount = 1;
    return INVALID_CODEPOINT;
}

unsigned int decodeCharacter(const unsigned char* text, unsigned int remaining,
                             bool utf8, unsigned int* byteCount) {
    unsigned int codepoint;
    if (utf8 && decodeUtf8Character(text, remaining, &codepoint, byteCount))
        return codepoint;
    return decodeCp949Character(text, remaining, byteCount);
}

const unsigned char* hangulBitmap(unsigned int codepoint) {
    unsigned int index;
    if (codepoint >= 0x1100 && codepoint <= 0x11FF) {
        index = codepoint - 0x1100;
    } else if (codepoint >= 0x3130 && codepoint <= 0x318F) {
        index = 0x100 + codepoint - 0x3130;
    } else if (codepoint >= 0xAC00 && codepoint <= 0xD7A3) {
        index = 0x160 + codepoint - 0xAC00;
    } else {
        return 0;
    }
    return hangul_font_bin + index * 8;
}

unsigned int tileForCodepoint(unsigned int codepoint, PrintConsole* console) {
    if (codepoint < 128)
        return codepoint;

    const unsigned char* bitmap = hangulBitmap(codepoint);
    if (!bitmap)
        return '?';

    for (unsigned int i = 0; i < nextDynamicTile; i++) {
        if (cachedCodepoints[i] == codepoint)
            return FIRST_DYNAMIC_TILE + i;
    }
    if (nextDynamicTile >= DYNAMIC_TILE_COUNT)
        return '?';

    const unsigned int cacheIndex = nextDynamicTile++;
    cachedCodepoints[cacheIndex] = codepoint;
    const unsigned int tile = FIRST_DYNAMIC_TILE + cacheIndex;
    u8* destination = reinterpret_cast<u8*>(console->fontBgGfx) + tile * 32;
    for (unsigned int y = 0; y < 8; y++) {
        for (unsigned int pair = 0; pair < 4; pair++) {
            const unsigned int left = pair * 2;
            u8 value = (bitmap[y] & (1 << (7 - left))) ? 0x0F : 0;
            if (bitmap[y] & (1 << (6 - left)))
                value |= 0xF0;
            destination[y * 4 + pair] = value;
        }
    }
    return tile;
}

} // namespace

void utf8FontResetCache() {
    memset(cachedCodepoints, 0, sizeof(cachedCodepoints));
    nextDynamicTile = 0;
}

unsigned int utf8TextColumns(const char* text) {
    if (!text)
        return 0;
    const bool utf8 = isValidUtf8(text);
    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text);
    unsigned int remaining = strlen(text);
    unsigned int columns = 0;
    while (remaining) {
        unsigned int byteCount;
        decodeCharacter(cursor, remaining, utf8, &byteCount);
        cursor += byteCount;
        remaining -= byteCount;
        columns++;
    }
    return columns;
}

void utf8CopyText(char* destination, size_t destinationSize,
                  const char* source, unsigned int maxColumns) {
    if (!destination || destinationSize == 0)
        return;
    destination[0] = '\0';
    if (!source)
        return;

    const bool utf8 = isValidUtf8(source);
    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(source);
    unsigned int remaining = strlen(source);
    size_t used = 0;
    unsigned int columns = 0;
    while (remaining && columns < maxColumns) {
        unsigned int byteCount;
        decodeCharacter(cursor, remaining, utf8, &byteCount);
        if (used + byteCount >= destinationSize)
            break;
        memcpy(destination + used, cursor, byteCount);
        used += byteCount;
        cursor += byteCount;
        remaining -= byteCount;
        columns++;
    }
    destination[used] = '\0';
}

void utf8PrintColored(int palette, const char* text, int maxColumns) {
    if (!text)
        return;
    PrintConsole* console = getPrintConsole();
    if (!console || !console->fontBgMap || !console->fontBgGfx)
        return;

    const bool utf8 = isValidUtf8(text);
    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text);
    unsigned int remaining = strlen(text);
    int printedColumns = 0;
    int x = console->cursorX;
    int y = console->cursorY;

    while (remaining && (maxColumns < 0 || printedColumns < maxColumns)) {
        unsigned int byteCount;
        const unsigned int codepoint =
            decodeCharacter(cursor, remaining, utf8, &byteCount);
        cursor += byteCount;
        remaining -= byteCount;

        if (codepoint == '\r')
            continue;
        if (codepoint == '\n') {
            x = 0;
            y++;
            continue;
        }
        if (x >= 32) {
            x = 0;
            y++;
        }
        if (y >= 32)
            break;

        const unsigned int tile = tileForCodepoint(codepoint, console);
        console->fontBgMap[y * 32 + x] = tile | TILE_PALETTE(palette);
        x++;
        printedColumns++;
        if (x >= 32) {
            x = 0;
            y++;
        }
    }
    console->cursorX = x;
    console->cursorY = y;
}
