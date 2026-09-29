#define BORDER_INPUT_TEST
#define main lifecycle_harness_main
#include "romfile_bios_lifecycle_test.cpp"
#undef main

int main() {
    probingForBorder = true;
    fastForwardKey = false;
    fastForwardMode = false;
    syntheticPressedKeys = 1 << FUNC_KEY_FAST_FORWARD;
    mgr_updateVBlank();
    if (!fastForwardKey || fastForwardMode || buttonsPressed != 0xff)
        return 1;
    syntheticPressedKeys = 0;
    syntheticJustPressedKeys = 1 << FUNC_KEY_FAST_FORWARD_TOGGLE;
    mgr_updateVBlank();
    if (fastForwardKey || !fastForwardMode || buttonsPressed != 0xff)
        return 2;
    syntheticJustPressedKeys = 1 << FUNC_KEY_MENU;
    mgr_updateVBlank();
    if (syntheticMenuOpens != 1 || fastForwardKey || fastForwardMode ||
            buttonsPressed != 0xff)
        return 3;
    probingForBorder = false;
    return 0;
}
