#pragma once

#include <stdint.h>

enum HBlankAnomalyFlags {
    HBLANK_RETRY_PREVIOUS = 1 << 0,
    HBLANK_LATE_ENTRY = 1 << 1,
    HBLANK_CROSSED_LINE = 1 << 2
};

struct HBlankAnomalyEvent {
    uint32_t hostFrame;
    uint32_t guestFrame;
    uint32_t publishedGuestFrame;
    uint16_t entryVcount;
    uint16_t exitVcount;
    uint16_t requestedPhysicalLine;
    int16_t guestLine;
    uint16_t dispstat;
    uint16_t bg0cnt;
    uint16_t winIn;
    uint16_t bgPalette0;
    uint16_t spritePalette0;
    uint16_t firstGuestObjAttr0;
    uint8_t flags;
    uint8_t drawingBuffer;
    uint8_t modified;
    uint8_t mapsModified;
    uint8_t bgPalettesModified;
    uint8_t sprPalettesModified;
    uint8_t spritesModified;
    uint8_t screenDisabled;
    uint8_t fastForward;
};

static inline uint8_t hblankAnomalyFlags(uint16_t entryVcount,
                                         uint16_t exitVcount,
                                         uint16_t dispstat,
                                         bool retriedPrevious) {
    return (retriedPrevious ? HBLANK_RETRY_PREVIOUS : 0) |
           (entryVcount < 192 && !(dispstat & 2) ?
                HBLANK_LATE_ENTRY : 0) |
           (entryVcount < 192 && entryVcount != exitVcount ?
                HBLANK_CROSSED_LINE : 0);
}

template <unsigned Capacity>
class HBlankAnomalyRing {
public:
    HBlankAnomalyRing() : written_(0), read_(0), overwritten_(0),
                          frozen_(false) {}

    void record(const HBlankAnomalyEvent& event) {
        if (frozen_)
            return;
        if (written_ - read_ == Capacity) {
            ++read_;
            ++overwritten_;
        }
        events_[written_ % Capacity] = event;
        ++written_;
    }

    bool pop(HBlankAnomalyEvent* output) {
        if (!output || read_ == written_)
            return false;
        *output = events_[read_ % Capacity];
        ++read_;
        return true;
    }

    uint32_t overwritten() const { return overwritten_; }
    void freeze() { frozen_ = true; }
    void resume() { frozen_ = false; }
    void clear() {
        written_ = 0;
        read_ = 0;
        overwritten_ = 0;
        frozen_ = false;
    }

private:
    HBlankAnomalyEvent events_[Capacity];
    uint32_t written_;
    uint32_t read_;
    uint32_t overwritten_;
    bool frozen_;
};

#ifdef GAMEYOB_HBLANK_ANOMALY_TRACE
// Foreground only: each pop/freeze uses one short interrupt exclusion.
unsigned readHBlankAnomalyTrace(HBlankAnomalyEvent* output,
                                unsigned capacity);
uint32_t hblankAnomalyTraceOverwritten();
void freezeHBlankAnomalyTrace();
void resumeHBlankAnomalyTrace();
void clearHBlankAnomalyTrace();
#endif
