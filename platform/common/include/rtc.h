#pragma once

#include <stdint.h>
#include <time.h>

namespace rtc {

void advanceMbc3(int& seconds, int& minutes, int& hours, int& days,
        int& control, uint64_t elapsedSeconds);

void advanceHuc3(int& minutes, int& days, int& years,
        time_t& last, time_t now);

} // namespace rtc
