#include <assert.h>
#include <string.h>
#include "sgbborder.h"

static void testTileBanksAndBitplanes() {
    SgbBorderData border;
    uint8_t transfer[0x1000];
    sgbBorderReset(&border);
    memset(transfer, 0, sizeof(transfer));

    // Tile 0, row 0: pixels 1, 2, 4, 8 followed by four transparent pixels.
    transfer[0] = 0x80;
    transfer[1] = 0x40;
    transfer[16] = 0x20;
    transfer[17] = 0x10;
    sgbBorderDecodeTiles(&border, transfer, false);

    assert(border.tiles[0][0] == 1);
    assert(border.tiles[0][1] == 2);
    assert(border.tiles[0][2] == 4);
    assert(border.tiles[0][3] == 8);
    assert(border.tileBankLoaded[0]);
    assert(!border.tileBankLoaded[1]);

    memset(transfer, 0, sizeof(transfer));
    transfer[0] = 0xff;
    sgbBorderDecodeTiles(&border, transfer, true);
    assert(border.tiles[128][0] == 1);
    assert(border.tiles[128][7] == 1);
    assert(border.tileBankLoaded[1]);
}

static void testMapPaletteAndFlipping() {
    SgbBorderData border;
    uint8_t transfer[0x1000];
    sgbBorderReset(&border);
    memset(transfer, 0, sizeof(transfer));

    // Tile 5, palette 2, horizontally and vertically flipped.
    const uint16_t entry = 5 | (2 << 10) | 0x4000 | 0x8000;
    transfer[0] = entry & 0xff;
    transfer[1] = entry >> 8;
    transfer[0x800 + (2 * 16 + 3) * 2] = 0x1f;
    transfer[0x800 + (2 * 16 + 3) * 2 + 1] = 0x00;
    border.tiles[5][7 * 8 + 7] = 3;

    sgbBorderDecodeMap(&border, transfer);

    uint8_t palette = 0;
    assert(sgbBorderGetPixel(&border, 0, 0, &palette) == 3);
    assert(palette == 2);
    assert(border.palettes[2][3] == 0x001f);
    assert(border.mapLoaded);
}

static void testColorConversion() {
    assert(sgbBorderColorToRgb24(0x001f) == 0xff0000);
    assert(sgbBorderColorToRgb24(0x03e0) == 0x00ff00);
    assert(sgbBorderColorToRgb24(0x7c00) == 0x0000ff);
    assert(sgbBorderColorToRgb24(0x7fff) == 0xffffff);
}

int main() {
    testTileBanksAndBitplanes();
    testMapPaletteAndFlipping();
    testColorConversion();
    return 0;
}
