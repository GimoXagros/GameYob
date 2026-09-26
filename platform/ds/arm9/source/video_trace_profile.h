#pragma once

#include <stdint.h>

// Diagnostic-only frame schedule. The caller changes host fast-forward only
// between completed guest frames, and performs all trace I/O in foreground.
class VideoTraceProfile {
public:
    enum Phase { WARMUP, STABLE, NORMAL, FAST, RECOVERY, DONE };
    enum Action { NONE, START_WINDOW, END_WINDOW, ABORT_WINDOW };

    VideoTraceProfile(uint32_t firstGuestFrame, bool* fastForwardMode)
        : phase_(WARMUP), lastGuestFrame_(firstGuestFrame), frames_(0),
          fastForwardMode_(fastForwardMode),
          savedFastForwardMode_(fastForwardMode ? *fastForwardMode : false),
          attempt_(1) {}

    Action observe(bool ready, bool unmasked, uint32_t guestFrame) {
        if (phase_ == DONE || phaseEnded_)
            return NONE;
        if (!ready) {
            lastGuestFrame_ = guestFrame;
            if (phase_ == WARMUP || phase_ == STABLE)
                frames_ = 0;
            return phase_ == NORMAL || phase_ == FAST ||
                   phase_ == RECOVERY ? retry(guestFrame) : NONE;
        }
        // An SGB border probe can reinitialize the cartridge and reset this
        // counter to zero. It is not a huge unsigned frame jump.
        if (guestFrame < lastGuestFrame_) {
            lastGuestFrame_ = guestFrame;
            if (phase_ == WARMUP || phase_ == STABLE)
                frames_ = 0;
            return phase_ == NORMAL || phase_ == FAST ||
                   phase_ == RECOVERY ? retry(guestFrame) : NONE;
        }
        const uint32_t elapsed = guestFrame - lastGuestFrame_;
        lastGuestFrame_ = guestFrame;
        if (!elapsed)
            return NONE;
        frames_ += elapsed;
        if (phase_ == WARMUP && frames_ >= 180) {
            phase_ = STABLE;
            frames_ = 0;
            return NONE;
        }
        if (phase_ == STABLE) {
            if (!unmasked) {
                frames_ = 0;
                return NONE;
            }
            if (frames_ < 60)
                return NONE;
            phase_ = NORMAL;
            frames_ = 0;
            setFastForward(false);
            return START_WINDOW;
        }
        if (!unmasked)
            return retry(guestFrame);
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
    unsigned attempt() const { return attempt_; }
    bool retryInvalidEvidence(uint32_t guestFrame) {
        retry(guestFrame);
        return phase_ != DONE;
    }
    void stop() {
        setFastForward(savedFastForwardMode_);
        phase_ = DONE;
        phaseEnded_ = false;
    }
    bool wantsFastForward() const { return phase_ == FAST; }
    static const char* phaseName(Phase phase) {
        return phase == NORMAL ? "normal_before" :
               phase == FAST ? "fast_forward" :
               phase == RECOVERY ? "normal_after" : "none";
    }

private:
    Action retry(uint32_t guestFrame) {
        setFastForward(savedFastForwardMode_);
        lastGuestFrame_ = guestFrame;
        frames_ = 0;
        phaseEnded_ = false;
        phase_ = ++attempt_ <= 3 ? WARMUP : DONE;
        return ABORT_WINDOW;
    }
    void setFastForward(bool enabled) {
        if (fastForwardMode_)
            *fastForwardMode_ = enabled;
    }

    Phase phase_;
    uint32_t lastGuestFrame_;
    uint32_t frames_;
    bool* fastForwardMode_;
    bool savedFastForwardMode_;
    unsigned attempt_;
    bool phaseEnded_ = false;
};

#if defined(GAMEYOB_VIDEO_TRACE) && defined(GAMEYOB_VIDEO_PUBLICATION_ONLY)
// Foreground only. A null directory writes into GameYob's current FAT folder.
enum VideoTraceProfileExportResult {
    VIDEO_PROFILE_WRITTEN,
    VIDEO_PROFILE_INVALID,
    VIDEO_PROFILE_IO_ERROR
};
VideoTraceProfileExportResult exportVideoTraceProfileCsv(
    VideoTraceProfile::Phase phase, unsigned attempt,
    uint32_t firstGuestFrame, uint32_t lastGuestFrame,
    uint32_t overwrittenBefore, const char* directory = 0);
#endif
