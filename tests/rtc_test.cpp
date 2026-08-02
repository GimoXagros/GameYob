#include "rtc.h"

#include <assert.h>

static void testSecondAndDayRollover() {
    int seconds = 59;
    int minutes = 59;
    int hours = 23;
    int days = 0;
    int control = 0;
    rtc::advanceMbc3(seconds, minutes, hours, days, control, 1);
    assert(seconds == 0);
    assert(minutes == 0);
    assert(hours == 0);
    assert(days == 1);
}

static void testDayCarry() {
    int seconds = 59;
    int minutes = 59;
    int hours = 23;
    int days = 0x1ff;
    int control = 1;
    rtc::advanceMbc3(seconds, minutes, hours, days, control, 1);
    assert(days == 0);
    assert((control & 0x80) != 0);
    assert((control & 1) == 0);
}

static void testHalt() {
    int seconds = 12;
    int minutes = 34;
    int hours = 5;
    int days = 0x101;
    int control = 0x41;
    rtc::advanceMbc3(seconds, minutes, hours, days, control, 86400);
    assert(seconds == 12);
    assert(minutes == 34);
    assert(hours == 5);
    assert(days == 0x101);
    assert((control & 0x41) == 0x41);
}

static void testLargeElapsedInterval() {
    int seconds = 0;
    int minutes = 0;
    int hours = 0;
    int days = 0;
    int control = 0;
    rtc::advanceMbc3(seconds, minutes, hours, days, control,
            513ULL * 86400 + 3661);
    assert(seconds == 1);
    assert(minutes == 1);
    assert(hours == 1);
    assert(days == 1);
    assert((control & 0x80) != 0);
}

static void testControlRegisterIsSanitized() {
    int seconds = 1;
    int minutes = 2;
    int hours = 3;
    int days = 0x101;
    int control = 0xff;
    rtc::advanceMbc3(seconds, minutes, hours, days, control, 0);
    assert(control == 0xc1);
}

static void testHuc3Rollover() {
    int minutes = 1439;
    int days = 364;
    int years = 7;
    rtc::advanceHuc3(minutes, days, years, 60);
    assert(minutes == 0);
    assert(days == 0);
    assert(years == 8);
}

int main() {
    testSecondAndDayRollover();
    testDayCarry();
    testHalt();
    testLargeElapsedInterval();
    testControlRegisterIsSanitized();
    testHuc3Rollover();
    return 0;
}
