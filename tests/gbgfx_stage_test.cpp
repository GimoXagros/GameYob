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
    struct MinimalLine { bool modified; int scroll; };
    MinimalLine latest[3][2] = {};
    GbFrameSlots latestSlots;
    latest[1][0].modified = true;
    latest[1][0].scroll = 0;
    latest[1][1].modified = true;
    latest[1][1].scroll = 8;
    gbStageCompleteLatestFrame(latestSlots, latest); // Ready A.
    assert(latestSlots.ready() == 1 && latestSlots.producer() == 2);
    latest[2][0].modified = true;  // B starts with a full-frame line-0 state.
    latest[2][0].scroll = 0;
    latest[2][1].modified = false; // No line-1 change in B.
    gbStageCompleteLatestFrame(latestSlots, latest); // B supersedes A.
    assert(latestSlots.ready() == 2);
    assert(!latest[2][1].modified); // Old A's line flag must not replay.
    assert(!latest[latestSlots.producer()][1].modified);

    GbFrameSlots legacySlots;
    assert(legacySlots.displayed() == 0 && legacySlots.producer() == 1);
    legacySlots.publishImmediately();
    assert(legacySlots.displayed() == 1 && legacySlots.producer() == 0);
    legacySlots.publishImmediately();
    assert(legacySlots.displayed() == 0 && legacySlots.producer() == 1);

    GbFrameSlots stagedSlots;
    assert(stagedSlots.stageCompleted() == 3);
    assert(stagedSlots.displayed() == 0 && stagedSlots.ready() == 1 &&
           stagedSlots.producer() == 2);
    assert(stagedSlots.stageCompleted() == 1);
    assert(stagedSlots.displayed() == 0 && stagedSlots.ready() == 2 &&
           stagedSlots.producer() == 1);
    stagedSlots.commitReady();
    assert(stagedSlots.displayed() == 2 && !stagedSlots.hasReady() &&
           stagedSlots.producer() == 1);
    assert(stagedSlots.stageCompleted() == 3);
    assert(stagedSlots.displayed() == 2 && stagedSlots.ready() == 1 &&
           stagedSlots.producer() == 0);
    stagedSlots.commitReady();
    assert(stagedSlots.displayed() == 1 && stagedSlots.producer() == 0);

    assert(!mayCommitGbStagedFrame(167, 0));
    assert(mayCommitGbStagedFrame(168, 64));
    assert(!mayCommitGbStagedFrame(170, 64));
    assert(mayCommitGbStagedFrame(170, 62));
    assert(!mayCommitGbStagedFrame(170, 63));
    assert(!mayCommitGbStagedFrame(192, 40));
    assert(!mayCommitGbStagedFrame(168, 67));
    assert(gbStageMayPublish(232));
    assert(!gbStageMayPublish(233));
    assert(!gbStageMayPublish(20));
    assert(gbStageElapsedLines(3, 192, 3, 240) == 48);
    assert(gbStageElapsedLines(3, 168, 4, 216) == 48);
    assert(gbStageElapsedLines(3, 240, 3, 20) == 43);
    assert(!gbStageReadyStale(119, 0));
    assert(gbStageReadyStale(120, 0));
    assert(gbStageReadyStale(119, 0xfffffffeu));
    assert(gbStageNeedsForegroundWait(true, true, true, true));
    assert(!gbStageNeedsForegroundWait(true, false, true, true));
    assert(!gbStageNeedsForegroundWait(true, true, false, true));
    assert(!gbStageNeedsForegroundWait(true, true, true, false));
    assert(!gbStageNeedsForegroundWait(false, true, true, true));
    assert(gbStageDisplaySuspended(true, false, false, false, false));
    assert(gbStageDisplaySuspended(false, true, false, false, false));
    assert(gbStageDisplaySuspended(false, false, true, false, false));
    assert(!gbStageDisplaySuspended(false, false, false, false, false));
    uint32_t readySince = 3;
    gbStageRebaseReadyAge(150, true, true, &readySince);
    assert(readySince == 150 && !gbStageReadyStale(151, readySince));
    gbStageRebaseReadyAge(151, false, true, &readySince);
    assert(readySince == 150);
    gbStageRebaseReadyAge(200, true, false, &readySince);
    assert(readySince == 150);
    GbFrameSlots suspendedSlots;
    assert(suspendedSlots.stageCompleted() == 3);
    readySince = 0;
    for (uint32_t hostFrame = 1; hostFrame <= 180; hostFrame++)
        gbStageRebaseReadyAge(hostFrame, true,
                              suspendedSlots.hasReady(), &readySince);
    assert(suspendedSlots.hasReady());
    assert(!gbStageReadyStale(181, readySince));
    suspendedSlots.commitReady(); // First eligible service after resume.
    assert(!suspendedSlots.hasReady() && suspendedSlots.displayed() == 1);
    assert(gbStageAwaitSafePoll(7, 7, true));
    assert(!gbStageAwaitSafePoll(7, 8, true));
    assert(!gbStageAwaitSafePoll(7, 7, false));
    assert(gbStageAwaitHostVBlank(12, 12)); // Service before line 192.
    assert(!gbStageAwaitHostVBlank(12, 13)); // Copy crossed line 192.
    assert(gbStageAwaitHostVBlank(13, 13)); // Entered after line 168.
    assert(gbStageTileDirtyMask(0x7f) ==
           (STAGE_UNSIGNED | STAGE_UNSIGNED_FILLED | STAGE_OBJ));
    assert(gbStageTileDirtyMask(0x80) ==
           (STAGE_UNSIGNED | STAGE_UNSIGNED_FILLED | STAGE_OBJ |
            STAGE_SIGNED | STAGE_SIGNED_FILLED));
    assert(gbStageTileDirtyMask(0x100) ==
           (STAGE_SIGNED | STAGE_SIGNED_FILLED));
    assert(gbStageMapDirtyMask(0) ==
           (STAGE_NORMAL_0 | STAGE_COLOR0_0 | STAGE_OVERLAY_0));
    assert(gbStageMapDirtyMask(1) ==
           (STAGE_NORMAL_1 | STAGE_COLOR0_1 | STAGE_OVERLAY_1));
    assert(gbStageSgbDirtyMask() == (STAGE_NORMAL_0 | STAGE_NORMAL_1));
    unsigned fullDirty = gbStageMapDirtyMask(0) | gbStageMapDirtyMask(1);
    for (unsigned tile = 0; tile < 0x180; tile++)
        fullDirty |= gbStageTileDirtyMask(tile);
    assert(fullDirty == STAGE_ALL);
    assert(gbStageDirtyBytes(STAGE_ALL) == GB_GFX_STAGE_BYTES);
    assert(gbStageDirtyBytes(gbStageSgbDirtyMask()) == 4096);

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
    const uint16_t beforeRemap = legacy.assets.normalMaps[1][1023];
    remapSgbMapPalette(converted.assets.normalMaps[1], 1023, 3);
    legacy.assets.normalMaps[1][1023] =
        (beforeRemap & ~(uint16_t)(7 << 12)) | (3 << 12);
    assert((converted.assets.normalMaps[1][1023] & ~(uint16_t)(7 << 12)) ==
           (beforeRemap & ~(uint16_t)(7 << 12)));
    assert(memcmp(&converted.assets, &legacy.assets,
                  sizeof(converted.assets)) == 0);
    assert(converted.before == 0x76543210);
    assert(converted.after == 0xabcdef01);
    return 0;
}
