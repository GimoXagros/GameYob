#include <assert.h>
#include <string.h>
#include "../platform/ds/arm9/source/gbgfx_stage.h"

// Frozen reference from the former DS drawTile path. This compares the new
// production converter against the old address and pixel mapping for every
// tile in both VRAM banks, including signed/unsigned overlap.
static void legacyTile(const GbTileTargets& target, int tileNum, int bank,
                       const uint8_t* src) {
    int index = (tileNum << 4) + (bank * 0x100 * 16);
    int signedIndex = index;
    if (tileNum >= 0x100)
        signedIndex -= 0x100 << 4;
    const bool unsign = tileNum < 0x100;
    const bool sign = tileNum >= 0x80;
    for (int y = 0; y < 8; y++) {
        int b1 = *(src++);
        int b2 = *(src++) << 1;
        int bb0 = 0, bb1 = 0, fb0 = 0, fb1 = 0, sb0 = 0, sb1 = 0;
        int shift = 12;
        for (int x = 0; x < 4; x++) {
            int colorid = b1 & 1;
            b1 >>= 1;
            colorid |= b2 & 2;
            b2 >>= 1;
            fb1 |= (colorid + 1) << shift;
            if (colorid)
                bb1 |= (colorid + 1) << shift;
            if (unsign)
                sb1 |= colorid << shift;
            shift -= 4;
        }
        shift = 12;
        for (int x = 0; x < 4; x++) {
            int colorid = b1 & 1;
            b1 >>= 1;
            colorid |= b2 & 2;
            b2 >>= 1;
            fb0 |= (colorid + 1) << shift;
            if (colorid)
                bb0 |= (colorid + 1) << shift;
            if (unsign)
                sb0 |= colorid << shift;
            shift -= 4;
        }
        if (unsign) {
            target.unsignedTiles[index] = bb0;
            target.unsignedTiles[index + 1] = bb1;
            target.unsignedFilledTiles[index] = fb0;
            target.unsignedFilledTiles[index + 1] = fb1;
            target.objTiles[index++] = sb0;
            target.objTiles[index++] = sb1;
        }
        if (sign) {
            target.signedTiles[signedIndex] = bb0;
            target.signedTiles[signedIndex + 1] = bb1;
            target.signedFilledTiles[signedIndex++] = fb0;
            target.signedFilledTiles[signedIndex++] = fb1;
        }
    }
}

struct GuardedAssets {
    uint32_t before;
    GbStagedAssets assets;
    uint32_t after;
};

static GuardedAssets converted = {};
static GuardedAssets legacy = {};

int main() {
    assert(GB_GFX_STAGE_BYTES == 92 * 1024);
    assert(sizeof(GbStagedAssets) == 92 * 1024);
    converted.before = legacy.before = 0x76543210;
    converted.after = legacy.after = 0xabcdef01;
    memset(&converted.assets, 0xa5, sizeof(converted.assets));
    memset(&legacy.assets, 0xa5, sizeof(legacy.assets));

    for (int bank = 0; bank < 2; bank++) {
        for (int tile = 0; tile < 0x180; tile++) {
            uint8_t data[16];
            for (int byte = 0; byte < 16; byte++)
                data[byte] = (tile * 37 + bank * 91 + byte * 13) & 255;
            convertGbTile(converted.assets.tileTargets(), tile, bank, data);
            legacyTile(legacy.assets.tileTargets(), tile, bank, data);
        }
    }
    assert(memcmp(&converted.assets, &legacy.assets,
                  sizeof(converted.assets)) == 0);
    assert(converted.before == 0x76543210);
    assert(converted.after == 0xabcdef01);

    for (int mode = 0; mode < 2; mode++) {
        for (int map = 0; map < 2; map++) {
            for (int index = 0; index < GB_GFX_MAP_HALFWORDS; index++) {
                const uint8_t tile = (index * 19 + map) & 255;
                const uint8_t attr = (index * 31 + map * 17) & 255;
                convertGbMapEntry(converted.assets.mapTargets(), map,
                                  index, tile, attr, mode != 0);
                const int bank = mode && (attr & 8) ? 1 : 0;
                const int palette = mode ? attr & 7 : 0;
                const int flipX = mode && (attr & 0x20) ? 1 : 0;
                const int flipY = mode && (attr & 0x40) ? 1 : 0;
                const uint16_t value = tile + bank * 0x100 +
                    (palette << 12) + (flipX << 10) + (flipY << 11);
                legacy.assets.normalMaps[map][index] = value;
                legacy.assets.color0Maps[map][index] = palette << 12;
                legacy.assets.overlayMaps[map][index] =
                    mode && (attr & 0x80) ? value : 0x300;
            }
        }
        assert(memcmp(&converted.assets, &legacy.assets,
                      sizeof(converted.assets)) == 0);
    }
    remapSgbMapPalette(converted.assets.normalMaps[1], 1023, 3);
    remapSgbMapPalette(legacy.assets.normalMaps[1], 1023, 3);
    assert(memcmp(&converted.assets, &legacy.assets,
                  sizeof(converted.assets)) == 0);
    assert(converted.before == 0x76543210);
    assert(converted.after == 0xabcdef01);
    return 0;
}
