#pragma once

#include <stdint.h>

// Foreground-only diagnostic controller. It never changes the FF state or
// guest timing; a menu-forced L release is not a gameplay release.
class HBlankFfCapture {
public:
    enum Action { NONE, CLEAR, RELEASE };

    HBlankFfCapture() : rom_(0), lastGuest_(0), lastFast_(false),
                        armed_(false), pending_(false) {}

    Action observe(uintptr_t rom, bool usable, uint32_t guest, bool fast) {
        if (!rom) {
            const bool hadCapture = rom_ || armed_ || pending_;
            rom_ = 0;
            lastGuest_ = 0;
            lastFast_ = false;
            armed_ = false;
            pending_ = false;
            return hadCapture ? CLEAR : NONE;
        }
        if (rom != rom_ || guest < lastGuest_) {
            rom_ = rom;
            lastGuest_ = guest;
            lastFast_ = fast;
            armed_ = usable && fast;
            pending_ = false;
            return CLEAR;
        }
        const bool hadFast = lastFast_;
        const uint32_t elapsed = guest - lastGuest_;
        lastGuest_ = guest;
        lastFast_ = fast;
        if (pending_)
            return NONE;
        if (!usable) {
            armed_ = false;
            return NONE;
        }
        if (fast && !hadFast) {
            armed_ = true;
            return CLEAR;
        }
        if (armed_ && hadFast && !fast && elapsed) {
            armed_ = false;
            pending_ = true;
            return RELEASE;
        }
        return NONE;
    }

    bool pending() const { return pending_; }
    void exported() { pending_ = false; }

private:
    uintptr_t rom_;
    uint32_t lastGuest_;
    bool lastFast_;
    bool armed_;
    bool pending_;
};
