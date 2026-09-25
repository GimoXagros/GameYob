#pragma once

#include <stdint.h>

// Diagnostics only. The release renderer never allocates this ring.
enum VideoFrameEventType {
    VIDEO_GUEST_COMPLETE = 1,
    VIDEO_PUBLISH = 2,
    VIDEO_HOST_VBLANK = 3,
    VIDEO_HOST_LINE = 4,
    VIDEO_VRAM_ARM7 = 5,
    VIDEO_VRAM_DISPLAY = 6
};

struct VideoFrameEvent {
    uint32_t hostFrame;
    uint32_t guestFrame;
    uint32_t publishedFrame;
    uint32_t captureControl;
    uint32_t displayControlMain;
    uint32_t displayControlSub;
    uint16_t physicalLine;
    uint8_t type;
    uint8_t drawingBuffer;
    uint8_t renderingBuffer;
    uint8_t vramC;
    uint8_t vramD;
    uint8_t transferReady;
    uint8_t scalingMode;
    uint8_t filterMode;
};

template <unsigned Capacity>
class VideoFrameTraceRing {
public:
    VideoFrameTraceRing() : written_(0), read_(0), overwritten_(0) {}

    void record(const VideoFrameEvent& event) {
        if (written_ - read_ == Capacity) {
            ++read_;
            ++overwritten_;
        }
        events_[written_ % Capacity] = event;
        ++written_;
    }

    bool pop(VideoFrameEvent* output) {
        if (!output || read_ == written_)
            return false;
        *output = events_[read_ % Capacity];
        ++read_;
        return true;
    }

    uint32_t overwritten() const { return overwritten_; }

private:
    VideoFrameEvent events_[Capacity];
    uint32_t written_;
    uint32_t read_;
    uint32_t overwritten_;
};

// A sampled host frame showing two published guest generations is a definite
// mixed-frame publication. A false result is inconclusive between samples.
static inline bool videoTraceHasMixedFrame(const VideoFrameEvent* events,
                                            unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        if (events[i].type != VIDEO_HOST_LINE)
            continue;
        for (unsigned j = i + 1; j < count; ++j) {
            if (events[j].type == VIDEO_HOST_LINE &&
                    events[j].hostFrame == events[i].hostFrame &&
                    events[j].publishedFrame != events[i].publishedFrame)
                return true;
        }
    }
    return false;
}

#ifdef GAMEYOB_VIDEO_TRACE
// Foreground only. Copies at most one event per short IRQ-off section;
// callers can format or write the copied data after interrupts are restored.
unsigned readVideoFrameTrace(VideoFrameEvent* output, unsigned capacity);
uint32_t videoFrameTraceOverwritten();
#endif
