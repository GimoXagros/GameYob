#pragma once

#include <stdint.h>

// The six guest BG maps and four guest character blocks occupy exactly 92 KiB
// with the 512 guest OBJ tiles. Border graphics and the printer icon are not
// part of this staging area.
enum {
    GB_GFX_CHAR_HALFWORDS = 0x2000,
    GB_GFX_OBJ_HALFWORDS = 0x2000,
    GB_GFX_MAP_HALFWORDS = 0x400,
    GB_GFX_STAGE_BYTES = (4 * GB_GFX_CHAR_HALFWORDS +
                          GB_GFX_OBJ_HALFWORDS +
                          6 * GB_GFX_MAP_HALFWORDS) * sizeof(uint16_t)
};

struct GbTileTargets {
    uint16_t* unsignedTiles;
    uint16_t* signedTiles;
    uint16_t* unsignedFilledTiles;
    uint16_t* signedFilledTiles;
    uint16_t* objTiles;
};

struct GbMapTargets {
    uint16_t* normal[2];
    uint16_t* color0[2];
    uint16_t* overlay[2];
};

struct GbStagedAssets {
    uint16_t unsignedTiles[GB_GFX_CHAR_HALFWORDS];
    uint16_t signedTiles[GB_GFX_CHAR_HALFWORDS];
    uint16_t unsignedFilledTiles[GB_GFX_CHAR_HALFWORDS];
    uint16_t signedFilledTiles[GB_GFX_CHAR_HALFWORDS];
    uint16_t objTiles[GB_GFX_OBJ_HALFWORDS];
    uint16_t normalMaps[2][GB_GFX_MAP_HALFWORDS];
    uint16_t color0Maps[2][GB_GFX_MAP_HALFWORDS];
    uint16_t overlayMaps[2][GB_GFX_MAP_HALFWORDS];

    GbTileTargets tileTargets() {
        GbTileTargets result = { unsignedTiles, signedTiles,
                                 unsignedFilledTiles, signedFilledTiles,
                                 objTiles };
        return result;
    }
    GbMapTargets mapTargets() {
        GbMapTargets result = { { normalMaps[0], normalMaps[1] },
                                { color0Maps[0], color0Maps[1] },
                                { overlayMaps[0], overlayMaps[1] } };
        return result;
    }
};

static_assert(sizeof(GbStagedAssets) == GB_GFX_STAGE_BYTES,
              "Guest graphics staging footprint changed");

// Display, ready, and producer are distinct. A guest frame may replace the
// previous ready frame without taking the displayed buffer from HBlank.
class GbFrameSlots {
public:
    GbFrameSlots() : displayed_(0), producer_(1), ready_(3), free_(2) {}

    unsigned displayed() const { return displayed_; }
    unsigned producer() const { return producer_; }
    unsigned ready() const { return ready_; }
    bool hasReady() const { return ready_ != 3; }

    // Existing non-staged path: retain the historical two-buffer swap.
    void publishImmediately() {
        const unsigned oldDisplayed = displayed_;
        displayed_ = producer_;
        producer_ = oldDisplayed;
    }

    // Called only after the complete producer frame has been staged. Returns
    // the superseded ready slot, or 3 if this is the first pending frame.
    unsigned stageCompleted() {
        const unsigned dropped = ready_;
        const unsigned nextProducer = hasReady() ? ready_ : free_;
        ready_ = producer_;
        producer_ = nextProducer;
        free_ = 3;
        return dropped;
    }

    // The caller must first finish all backing-asset transfers at a host-safe
    // boundary. This only changes RAM ownership; no DMA/VRAM work belongs here.
    void commitReady() {
        if (!hasReady())
            return;
        free_ = displayed_;
        displayed_ = ready_;
        ready_ = 3;
    }

private:
    unsigned displayed_;
    unsigned producer_;
    unsigned ready_;
    unsigned free_;
};

// The bound includes the calibration's additional IRQ/contention margin.
// One complete copy must finish before line-0 pre-render at VCOUNT 235.
static inline bool mayCommitGbStagedFrame(unsigned physicalLine,
                                           unsigned maxCopyLines) {
    return physicalLine >= 168 && physicalLine <= 170 &&
           maxCopyLines <= 66 && physicalLine + maxCopyLines < 235;
}

// Only a liveness alarm: a stuck ready generation is reported to foreground
// integration, never silently discarded or used to justify a late copy.
static inline bool gbStageReadyStale(uint32_t nowHostFrame,
                                     uint32_t readySinceHostFrame) {
    return (uint32_t)(nowHostFrame - readySinceHostFrame) >= 120;
}

// dsFrameCounter advances at VBlank line 192, not physical line zero.
static inline unsigned gbStageElapsedLines(uint32_t startHostFrame,
                                            unsigned startLine,
                                            uint32_t endHostFrame,
                                            unsigned endLine) {
    const int startPhase = (startLine + 263 - 192) % 263;
    const int endPhase = (endLine + 263 - 192) % 263;
    return (uint32_t)(endHostFrame - startHostFrame) * 263 +
           endPhase - startPhase;
}

enum GbStageDirtyBlock {
    STAGE_UNSIGNED = 1 << 0,
    STAGE_SIGNED = 1 << 1,
    STAGE_UNSIGNED_FILLED = 1 << 2,
    STAGE_SIGNED_FILLED = 1 << 3,
    STAGE_OBJ = 1 << 4,
    STAGE_NORMAL_0 = 1 << 5,
    STAGE_COLOR0_0 = 1 << 6,
    STAGE_OVERLAY_0 = 1 << 7,
    STAGE_NORMAL_1 = 1 << 8,
    STAGE_COLOR0_1 = 1 << 9,
    STAGE_OVERLAY_1 = 1 << 10,
    STAGE_ALL = (1 << 11) - 1
};

static inline unsigned gbStageTileDirtyMask(unsigned tile) {
    return (tile < 0x100 ?
            STAGE_UNSIGNED | STAGE_UNSIGNED_FILLED | STAGE_OBJ : 0) |
           (tile >= 0x80 ? STAGE_SIGNED | STAGE_SIGNED_FILLED : 0);
}

static inline unsigned gbStageMapDirtyMask(unsigned map) {
    return 7 << (5 + map * 3);
}

static inline unsigned gbStageSgbDirtyMask() {
    return STAGE_NORMAL_0 | STAGE_NORMAL_1;
}

static inline unsigned gbStageDirtyBytes(unsigned mask) {
    unsigned bytes = 0;
    for (unsigned block = 0; block < 5; block++)
        if (mask & (1 << block)) bytes += 0x4000;
    for (unsigned block = 5; block < 11; block++)
        if (mask & (1 << block)) bytes += 0x800;
    return bytes;
}

static inline void convertGbTile(const GbTileTargets& target,
                                  int tileNum, int bank, const uint8_t* src) {
    int index = (tileNum << 4) + (bank * 0x100 * 16);
    int signedIndex = index;
    if (tileNum >= 0x100)
        signedIndex -= (0x100 << 4);

    const bool unsign = tileNum < 0x100;
    const bool sign = tileNum >= 0x80;
    for (int y = 0; y < 8; y++) {
        int b1 = *(src++);
        int b2 = *(src++) << 1;
        int bb0 = 0, bb1 = 0;
        int fb0 = 0, fb1 = 0;
        int sb0 = 0, sb1 = 0;
        int shift = 12;
        for (int x = 0; x < 4; x++) {
            int colorid = b1 & 1;
            b1 >>= 1;
            colorid |= b2 & 2;
            b2 >>= 1;
            fb1 |= (colorid + 1) << shift;
            if (colorid != 0)
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
            if (colorid != 0)
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
            target.objTiles[index] = sb0;
            target.objTiles[index + 1] = sb1;
        }
        if (sign) {
            target.signedTiles[signedIndex] = bb0;
            target.signedTiles[signedIndex + 1] = bb1;
            target.signedFilledTiles[signedIndex] = fb0;
            target.signedFilledTiles[signedIndex + 1] = fb1;
        }
        if (unsign)
            index += 2;
        if (sign)
            signedIndex += 2;
    }
}

static inline void convertGbMapEntry(const GbMapTargets& target, int map,
                                      int index, uint8_t tileNum,
                                      uint8_t attributes, bool cgb) {
    const int flipX = cgb && (attributes & 0x20);
    const int flipY = cgb && (attributes & 0x40);
    const int bank = cgb && (attributes & 0x08);
    const int palette = cgb ? attributes & 7 : 0;
    const int priority = cgb && (attributes & 0x80);
    const uint16_t tile = tileNum + (bank ? 0x100 : 0);
    const uint16_t attrs = (palette << 12) | (flipX ? 1 << 10 : 0) |
                           (flipY ? 1 << 11 : 0);
    target.overlay[map][index] = priority ? tile | attrs : 0x300;
    target.normal[map][index] = tile | attrs;
    target.color0[map][index] = palette << 12;
}

static inline void remapSgbMapPalette(uint16_t* map, int index,
                                      int palette) {
    map[index] = (map[index] & ~(7 << 12)) | (palette << 12);
}
