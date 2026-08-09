#include <string.h>

#include "text.h"

#if defined(DS) || defined(_3DS)
#include "console.h"
#include "unicode_font_bin.h"
#include "cp949_table_bin.h"
#endif

#ifdef DS
#include <nds.h>
#elif defined(_3DS)
#include "printconsole.h"
#else
#include <stdio.h>
#endif

namespace {

const unsigned int INVALID_CODEPOINT = 0xFFFD;

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
    const unsigned char* cursor =
        reinterpret_cast<const unsigned char*>(text);
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
#if defined(DS) || defined(_3DS)
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
#else
    (void)text;
    (void)remaining;
#endif
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

#if defined(DS) || defined(_3DS)
unsigned int read16(const unsigned char* value) {
    return value[0] | (value[1] << 8);
}

unsigned int read32(const unsigned char* value) {
    return value[0] | (value[1] << 8) | (value[2] << 16) |
        (value[3] << 24);
}

const unsigned char* bitmapForCodepoint(unsigned int codepoint) {
#ifdef _3DS
    // Menus redraw the same glyphs frequently. A small direct-mapped cache
    // avoids repeating the binary search without retaining dynamic storage.
    static u16 cacheKeys[256];
    static const unsigned char* cacheValues[256];
    static bool cacheValid[256];
    const unsigned int cacheSlot = codepoint & 0xff;
    if (cacheValid[cacheSlot] && cacheKeys[cacheSlot] == codepoint)
        return cacheValues[cacheSlot];
#endif
    if (codepoint > 0xFFFF || unicode_font_bin_size < 12 ||
            memcmp(unicode_font_bin, "GYUF", 4) != 0 ||
            read16(unicode_font_bin + 4) != 1)
        return NULL;

    const unsigned int count = read32(unicode_font_bin + 8);
    unsigned int low = 0;
    unsigned int high = count;
    while (low < high) {
        const unsigned int mid = low + (high - low) / 2;
        const unsigned char* record = unicode_font_bin + 12 + mid * 10;
        const unsigned int candidate = read16(record);
        if (candidate < codepoint)
            low = mid + 1;
        else
            high = mid;
    }
    if (low >= count)
        return NULL;
    const unsigned char* record = unicode_font_bin + 12 + low * 10;
    const unsigned char* result =
        read16(record) == codepoint ? record + 2 : NULL;
#ifdef _3DS
    cacheKeys[cacheSlot] = codepoint;
    cacheValues[cacheSlot] = result;
    cacheValid[cacheSlot] = true;
#endif
    return result;
}
#endif

#ifdef DS
const unsigned int FIRST_DYNAMIC_TILE = 128;
const unsigned int DYNAMIC_TILE_COUNT = 896;
const unsigned int GLYPH_HASH_SIZE = 2048;
u16 glyphHashKeys[GLYPH_HASH_SIZE];
u16 glyphHashTiles[GLYPH_HASH_SIZE];
unsigned int nextDynamicTile = 0;

unsigned int glyphHashSlot(unsigned int codepoint) {
    return (codepoint * 2654435761U) & (GLYPH_HASH_SIZE - 1);
}

unsigned int asciiTileForCodepoint(unsigned int codepoint,
                                   PrintConsole* console) {
    if (codepoint < console->font.asciiOffset ||
            codepoint >= console->font.asciiOffset + console->font.numChars)
        codepoint = ' ';
    return codepoint + console->fontCharOffset - console->font.asciiOffset;
}

unsigned int tileForCodepoint(unsigned int codepoint, PrintConsole* console) {
    if (codepoint < 128)
        // libnds fonts omit the control-character range. consolePrintChar()
        // maps ASCII through asciiOffset/fontCharOffset; using the raw byte as
        // a tile index displays unrelated glyph data with current BlocksDS.
        return asciiTileForCodepoint(codepoint, console);
    const unsigned int hash = glyphHashSlot(codepoint);
    for (unsigned int probe = 0; probe < GLYPH_HASH_SIZE; ++probe) {
        const unsigned int slot = (hash + probe) & (GLYPH_HASH_SIZE - 1);
        if (!glyphHashTiles[slot])
            break;
        if (glyphHashKeys[slot] == codepoint)
            return glyphHashTiles[slot] - 1;
    }
    if (nextDynamicTile >= DYNAMIC_TILE_COUNT)
        return asciiTileForCodepoint('?', console);
    const unsigned char* bitmap = bitmapForCodepoint(codepoint);
    if (!bitmap)
        return asciiTileForCodepoint('?', console);

    const unsigned int cacheIndex = nextDynamicTile++;
    const unsigned int tile = FIRST_DYNAMIC_TILE + cacheIndex;
    for (unsigned int probe = 0; probe < GLYPH_HASH_SIZE; ++probe) {
        const unsigned int slot = (hash + probe) & (GLYPH_HASH_SIZE - 1);
        if (!glyphHashTiles[slot]) {
            glyphHashKeys[slot] = codepoint;
            glyphHashTiles[slot] = tile + 1;
            break;
        }
    }
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
#endif

} // namespace

void textResetGlyphCache() {
#ifdef DS
    memset(glyphHashKeys, 0, sizeof(glyphHashKeys));
    memset(glyphHashTiles, 0, sizeof(glyphHashTiles));
    nextDynamicTile = 0;
#endif
}

bool textEquivalent(const char* first, const char* second) {
    if (!first || !second)
        return first == second;

    const bool firstUtf8 = isValidUtf8(first);
    const bool secondUtf8 = isValidUtf8(second);
    const unsigned char* firstCursor =
        reinterpret_cast<const unsigned char*>(first);
    const unsigned char* secondCursor =
        reinterpret_cast<const unsigned char*>(second);
    unsigned int firstRemaining = strlen(first);
    unsigned int secondRemaining = strlen(second);

    while (firstRemaining && secondRemaining) {
        unsigned int firstBytes;
        unsigned int secondBytes;
        const unsigned int firstCodepoint = decodeCharacter(firstCursor,
            firstRemaining, firstUtf8, &firstBytes);
        const unsigned int secondCodepoint = decodeCharacter(secondCursor,
            secondRemaining, secondUtf8, &secondBytes);
        if (firstCodepoint != secondCodepoint)
            return false;
        firstCursor += firstBytes;
        firstRemaining -= firstBytes;
        secondCursor += secondBytes;
        secondRemaining -= secondBytes;
    }
    return firstRemaining == 0 && secondRemaining == 0;
}

unsigned int textColumns(const char* text) {
    if (!text)
        return 0;
    const bool utf8 = isValidUtf8(text);
    const unsigned char* cursor =
        reinterpret_cast<const unsigned char*>(text);
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

void textCopyColumns(char* destination, size_t destinationSize,
                     const char* source, unsigned int maxColumns) {
    if (!destination || destinationSize == 0)
        return;
    destination[0] = '\0';
    if (!source)
        return;
    const bool utf8 = isValidUtf8(source);
    const unsigned char* cursor =
        reinterpret_cast<const unsigned char*>(source);
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

void textPrintColored(int palette, const char* text, int maxColumns) {
    if (!text)
        return;
#if defined(DS) || defined(_3DS)
    const bool utf8 = isValidUtf8(text);
    const unsigned char* cursor =
        reinterpret_cast<const unsigned char*>(text);
    unsigned int remaining = strlen(text);
    int columns = 0;
    while (remaining && (maxColumns < 0 || columns < maxColumns)) {
        unsigned int byteCount;
        const unsigned int codepoint =
            decodeCharacter(cursor, remaining, utf8, &byteCount);
        cursor += byteCount;
        remaining -= byteCount;
#ifdef DS
        PrintConsole* console = getPrintConsole();
        if (!console || !console->fontBgMap || !console->fontBgGfx)
            return;
        if (codepoint == '\r')
            continue;
        if (codepoint == '\n') {
            console->cursorX = 0;
            console->cursorY++;
            continue;
        }
        if (console->cursorX >= 32) {
            console->cursorX = 0;
            console->cursorY++;
        }
        if (console->cursorY >= 32)
            break;
        const unsigned int tile = tileForCodepoint(codepoint, console);
        console->fontBgMap[console->cursorY * 32 + console->cursorX] =
            tile | TILE_PALETTE(palette);
        console->cursorX++;
#else
        if (codepoint < 128)
            consolePrintChar(static_cast<char>(codepoint));
        else {
            const unsigned char* bitmap = bitmapForCodepoint(codepoint);
            if (bitmap)
                consolePrintGlyph(bitmap);
            else
                consolePrintChar('?');
        }
#endif
        columns++;
    }
#else
    (void)palette;
    (void)maxColumns;
    fputs(text, stdout);
#endif
}
