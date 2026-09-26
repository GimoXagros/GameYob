#pragma once

#include <stdint.h>

// Diagnostic-only frame schedule. The caller changes host fast-forward only
// between completed guest frames, and performs all trace I/O in foreground.
class VideoTraceProfile {
public:
    enum Phase { WARMUP, NORMAL, FAST, RECOVERY, DONE };
    enum Action { NONE, START_WINDOW, END_WINDOW };

    VideoTraceProfile(uint32_t firstGuestFrame, bool* fastForwardMode)
        : phase_(WARMUP), lastGuestFrame_(firstGuestFrame), frames_(0),
          fastForwardMode_(fastForwardMode),
          savedFastForwardMode_(fastForwardMode ? *fastForwardMode : false) {}

    Action observe(bool ready, uint32_t guestFrame) {
        if (phase_ == DONE || phaseEnded_)
            return NONE;
        if (!ready) {
            lastGuestFrame_ = guestFrame;
            if (phase_ == WARMUP)
                frames_ = 0;
            return NONE;
        }
        // An SGB border probe can reinitialize the cartridge and reset this
        // counter to zero. It is not a huge unsigned frame jump.
        if (guestFrame < lastGuestFrame_) {
            lastGuestFrame_ = guestFrame;
            if (phase_ == WARMUP)
                frames_ = 0;
            return NONE;
        }
        const uint32_t elapsed = guestFrame - lastGuestFrame_;
        lastGuestFrame_ = guestFrame;
        if (!elapsed)
            return NONE;
        frames_ += elapsed;
        if (phase_ == WARMUP && frames_ >= 180) {
            phase_ = NORMAL;
            frames_ = 0;
            setFastForward(false);
            return START_WINDOW;
        }
        if (phase_ != WARMUP && frames_ >= 20) {
            phaseEnded_ = true;
            // Restore before the caller touches the filesystem, including on
            // every error path. The next window opts back in explicitly.
            setFastForward(savedFastForwardMode_);
            return END_WINDOW;
        }
        return NONE;
    }

    bool nextWindow() {
        if (!phaseEnded_)
            return false;
        phaseEnded_ = false;
        frames_ = 0;
        phase_ = phase_ == NORMAL ? FAST :
                 phase_ == FAST ? RECOVERY : DONE;
        if (phase_ != DONE)
            setFastForward(phase_ == FAST);
        return phase_ != DONE;
    }

    Phase phase() const { return phase_; }
    bool wantsFastForward() const { return phase_ == FAST; }
    static const char* phaseName(Phase phase) {
        return phase == NORMAL ? "normal_before" :
               phase == FAST ? "fast_forward" :
               phase == RECOVERY ? "normal_after" : "none";
    }

private:
    void setFastForward(bool enabled) {
        if (fastForwardMode_)
            *fastForwardMode_ = enabled;
    }

    Phase phase_;
    uint32_t lastGuestFrame_;
    uint32_t frames_;
    bool* fastForwardMode_;
    bool savedFastForwardMode_;
    bool phaseEnded_ = false;
};

#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_PUBLICATION_ONLY)
// Foreground only. A null directory writes into GameYob's current FAT folder.
bool exportVideoTraceProfileCsv(VideoTraceProfile::Phase phase,
                                uint32_t firstGuestFrame,
                                uint32_t lastGuestFrame,
                                uint32_t overwrittenBefore,
                                const char* directory = 0);
#endif
