#include <string.h>
#include "sgbborder.h"

void sgbBorderReset(SgbBorderData* border) {
    memset(border, 0, sizeof(*border));
}

void sgbBorderDecodeTiles(SgbBorderData* border, const uint8_t* source, bool highBank) {
    const int firstTile = highBank ? 128 : 0;

    for (int tile = 0; tile < 128; tile++) {
        const int sourceOffset = tile * 32;
        uint8_t* destination = border->tiles[firstTile + tile];

        for (int y = 0; y < 8; y++) {
            const uint8_t plane0 = source[sourceOffset + y * 2];
            const uint8_t plane1 = source[sourceOffset + y * 2 + 1];
            const uint8_t plane2 = source[sourceOffset + 16 + y * 2];
            const uint8_t plane3 = source[sourceOffset + 16 + y * 2 + 1];

            for (int x = 0; x < 8; x++) {
                const int bit = 7 - x;
                destination[y * 8 + x] =
                    ((plane0 >> bit) & 1) |
                    (((plane1 >> bit) & 1) << 1) |
                    (((plane2 >> bit) & 1) << 2) |
                    (((plane3 >> bit) & 1) << 3);
            }
        }
    }

    border->tileBankLoaded[highBank ? 1 : 0] = true;
}

void sgbBorderDecodeMap(SgbBorderData* border, const uint8_t* source) {
    for (int i = 0; i < SGB_BORDER_MAP_WIDTH * SGB_BORDER_MAP_HEIGHT; i++) {
        border->map[i] = source[i * 2] | (source[i * 2 + 1] << 8);
    }

    const uint8_t* paletteSource = source + 0x800;
    for (int palette = 0; palette < SGB_BORDER_PALETTE_COUNT; palette++) {
        for (int color = 0; color < SGB_BORDER_PALETTE_COLORS; color++) {
            const int offset = (palette * SGB_BORDER_PALETTE_COLORS + color) * 2;
            border->palettes[palette][color] =
                paletteSource[offset] | (paletteSource[offset + 1] << 8);
        }
    }

    border->mapLoaded = true;
}

uint8_t sgbBorderGetPixel(const SgbBorderData* border, int x, int y, uint8_t* palette) {
    if (x < 0 || x >= SGB_BORDER_WIDTH || y < 0 || y >= SGB_BORDER_HEIGHT) {
        *palette = 0;
        return 0;
    }

    const uint16_t mapEntry =
        border->map[(y / 8) * SGB_BORDER_MAP_WIDTH + (x / 8)];
    const int tile = mapEntry & 0xff;
    const bool flipX = (mapEntry & 0x4000) != 0;
    const bool flipY = (mapEntry & 0x8000) != 0;
    const int tileX = flipX ? 7 - (x & 7) : (x & 7);
    const int tileY = flipY ? 7 - (y & 7) : (y & 7);

    *palette = (mapEntry >> 10) & 3;
    return border->tiles[tile][tileY * 8 + tileX];
}

uint32_t sgbBorderColorToRgb24(uint16_t color) {
    const int red5 = color & 0x1f;
    const int green5 = (color >> 5) & 0x1f;
    const int blue5 = (color >> 10) & 0x1f;
    const int red8 = (red5 << 3) | (red5 >> 2);
    const int green8 = (green5 << 3) | (green5 >> 2);
    const int blue8 = (blue5 << 3) | (blue5 >> 2);

    return (red8 << 16) | (green8 << 8) | blue8;
}
