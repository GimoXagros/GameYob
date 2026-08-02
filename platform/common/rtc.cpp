#include "rtc.h"

namespace {

int clampRegister(int value, int maximum) {
    if (value < 0)
        return 0;
    return value > maximum ? maximum : value;
}

} // namespace

namespace rtc {

void advanceMbc3(int& seconds, int& minutes, int& hours, int& days,
        int& control, uint64_t elapsedSeconds) {
    seconds = clampRegister(seconds, 59);
    minutes = clampRegister(minutes, 59);
    hours = clampRegister(hours, 23);
    days &= 0x1ff;
    control &= 0xc1;

    if (control & 0x40) {
        control = (control & 0xc0) | ((days >> 8) & 1);
        return;
    }

    uint64_t total = static_cast<uint64_t>(seconds) +
        static_cast<uint64_t>(minutes) * 60 +
        static_cast<uint64_t>(hours) * 60 * 60 +
        static_cast<uint64_t>(days) * 24 * 60 * 60 + elapsedSeconds;

    seconds = total % 60;
    total /= 60;
    minutes = total % 60;
    total /= 60;
    hours = total % 24;
    const uint64_t totalDays = total / 24;
    if (totalDays > 0x1ff)
        control |= 0x80;
    days = totalDays & 0x1ff;
    control = (control & 0xc0) | ((days >> 8) & 1);
}

void advanceHuc3(int& minutes, int& days, int& years,
        uint64_t elapsedSeconds) {
    const uint64_t elapsedMinutes = elapsedSeconds / 60;
    uint64_t totalMinutes = static_cast<uint64_t>(minutes < 0 ? 0 : minutes) +
        elapsedMinutes;
    minutes = totalMinutes % (24 * 60);
    uint64_t totalDays = static_cast<uint64_t>(days < 0 ? 0 : days) +
        totalMinutes / (24 * 60);
    days = totalDays % 365;
    years += totalDays / 365;
}

} // namespace rtc
