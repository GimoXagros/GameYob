#pragma once
#include <stdint.h>

// Diagnostic-only, foreground controller. It never changes fast-forward or
// guest timing; it captures the last FF frames plus eight post-release frames.
class VideoFfReleaseCapture {
public:
    enum Action { NONE, NEW_ROM, RELEASE, READY, CANCEL };
    VideoFfReleaseCapture() : rom_(0), lastGuest_(0), releaseGuest_(0),
        tailFrames_(0), lastFast_(false), state_(WAIT) {}

    Action observe(uintptr_t rom, bool usable, uint32_t guest, bool fast) {
        if (!rom) {
            const bool wasTail = state_ == TAIL;
            state_ = WAIT;
            rom_ = 0;
            return wasTail ? CANCEL : NONE;
        }
        if (rom != rom_ || guest < lastGuest_) {
            const bool wasTail = state_ == TAIL;
            rom_ = rom;
            lastGuest_ = guest;
            lastFast_ = fast;
            tailFrames_ = 0;
            state_ = ARMED;
            return wasTail ? CANCEL : NEW_ROM;
        }
        const uint32_t elapsed = guest - lastGuest_;
        lastGuest_ = guest;
        if (state_ == DONE)
            return NONE;
        if (!usable) {
            const bool wasTail = state_ == TAIL;
            state_ = ARMED;
            lastFast_ = fast;
            tailFrames_ = 0;
            return wasTail ? CANCEL : NONE;
        }
        if (state_ == ARMED) {
            const bool released = lastFast_ && !fast && elapsed != 0;
            lastFast_ = fast;
            if (released) {
                releaseGuest_ = guest;
                tailFrames_ = 0;
                state_ = TAIL;
                return RELEASE;
            }
            return NONE;
        }
        if (fast) {
            state_ = ARMED;
            lastFast_ = true;
            tailFrames_ = 0;
            return CANCEL;
        }
        tailFrames_ += elapsed;
        if (tailFrames_ >= 8) {
            state_ = DONE;
            return READY;
        }
        return NONE;
    }
    uint32_t releaseGuest() const { return releaseGuest_; }
private:
    enum State { WAIT, ARMED, TAIL, DONE };
    uintptr_t rom_;
    uint32_t lastGuest_;
    uint32_t releaseGuest_;
    uint32_t tailFrames_;
    bool lastFast_;
    State state_;
};

#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_FF_RELEASE)
bool exportVideoFfReleaseTrace(uint32_t releaseGuest, unsigned releaseVcount);
#endif
