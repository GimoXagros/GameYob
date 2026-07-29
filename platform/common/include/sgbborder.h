#pragma once

#include <stdint.h>

enum {
    SGB_BORDER_WIDTH = 256,
    SGB_BORDER_HEIGHT = 224,
    SGB_BORDER_MAP_WIDTH = 32,
    SGB_BORDER_MAP_HEIGHT = 28,
    SGB_BORDER_TILE_COUNT = 256,
    SGB_BORDER_TILE_PIXELS = 64,
    SGB_BORDER_PALETTE_COUNT = 4,
    SGB_BORDER_PALETTE_COLORS = 16
};

struct SgbBorderData {
    uint8_t tiles[SGB_BORDER_TILE_COUNT][SGB_BORDER_TILE_PIXELS];
    uint16_t map[SGB_BORDER_MAP_WIDTH * SGB_BORDER_MAP_HEIGHT];
    uint16_t palettes[SGB_BORDER_PALETTE_COUNT][SGB_BORDER_PALETTE_COLORS];
    bool tileBankLoaded[2];
    bool mapLoaded;
};

void sgbBorderReset(SgbBorderData* border);
void sgbBorderDecodeTiles(SgbBorderData* border, const uint8_t* source, bool highBank);
void sgbBorderDecodeMap(SgbBorderData* border, const uint8_t* source);
uint8_t sgbBorderGetPixel(const SgbBorderData* border, int x, int y, uint8_t* palette);
uint32_t sgbBorderColorToRgb24(uint16_t color);
