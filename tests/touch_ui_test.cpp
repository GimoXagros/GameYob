#include <cassert>
#include <iostream>

#include "touch_ui.h"

static void expect(TouchUiDecision value, TouchUiAction action, int row = -1) {
    assert(value.action == action);
    assert(value.visibleRow == row);
}

int main() {
    expect(touchMenuHitTest(0, 0, 7, -1, false),
           TOUCH_UI_HEADER_PREVIOUS);
    expect(touchMenuHitTest(63, 7, 7, -1, false),
           TOUCH_UI_HEADER_PREVIOUS);
    expect(touchMenuHitTest(64, 7, 7, -1, false), TOUCH_UI_HEADER_CLOSE);
    expect(touchMenuHitTest(191, 7, 7, -1, false), TOUCH_UI_HEADER_CLOSE);
    expect(touchMenuHitTest(192, 7, 7, -1, false), TOUCH_UI_HEADER_NEXT);
    expect(touchMenuHitTest(255, 7, 7, -1, false), TOUCH_UI_HEADER_NEXT);

    expect(touchMenuHitTest(10, 8, 7, -1, false), TOUCH_UI_SELECT_ROW, 0);
    expect(touchMenuHitTest(10, 23, 7, -1, false), TOUCH_UI_SELECT_ROW, 0);
    expect(touchMenuHitTest(10, 24, 7, -1, false), TOUCH_UI_SELECT_ROW, 1);
    expect(touchMenuHitTest(10, 119, 7, -1, false), TOUCH_UI_SELECT_ROW, 6);
    expect(touchMenuHitTest(10, 120, 7, -1, false), TOUCH_UI_NONE);
    expect(touchMenuHitTest(-1, 8, 7, -1, false), TOUCH_UI_NONE);
    expect(touchMenuHitTest(256, 8, 7, -1, false), TOUCH_UI_NONE);
    expect(touchMenuHitTest(10, 192, 7, -1, false), TOUCH_UI_NONE);

    expect(touchMenuHitTest(10, 8, 7, 0, false),
           TOUCH_UI_ACTIVATE_ROW, 0);
    expect(touchMenuHitTest(127, 24, 7, 1, true),
           TOUCH_UI_VALUE_LEFT, 1);
    expect(touchMenuHitTest(128, 24, 7, 1, true),
           TOUCH_UI_VALUE_RIGHT, 1);
    assert(touchWrappedValue(0, 3, -1) == 2);
    assert(touchWrappedValue(2, 3, 1) == 0);
    assert(touchWrappedValue(1, 3, -1) == 0);
    assert(touchWrappedValue(1, 3, 1) == 2);

    const bool visible[] = {true, false, true, false, true};
    assert(touchVisibleRowToSource(visible, 5, 0) == 0);
    assert(touchVisibleRowToSource(visible, 5, 1) == 2);
    assert(touchVisibleRowToSource(visible, 5, 2) == 4);
    assert(touchVisibleRowToSource(visible, 5, 3) == -1);

    TouchUiDebounce debounce;
    touchUiBegin(&debounce);
    assert(!touchUiPressEdge(&debounce, true));
    assert(!touchUiPressEdge(&debounce, true));
    assert(!touchUiPressEdge(&debounce, false));
    assert(touchUiPressEdge(&debounce, true));
    assert(!touchUiPressEdge(&debounce, true));
    assert(!touchUiPressEdge(&debounce, false));
    assert(touchUiPressEdge(&debounce, true));

    expect(touchFileChooserHitTest(20, 8, 5, 22, 2, true, false, false),
           TOUCH_UI_SELECT_ROW, 0);
    expect(touchFileChooserHitTest(20, 24, 5, 22, 2, true, false, false),
           TOUCH_UI_ACTIVATE_ROW, 2);
    expect(touchFileChooserHitTest(0, 8, 5, 22, 0, true, true, true),
           TOUCH_UI_SCROLL_UP, 0);
    expect(touchFileChooserHitTest(0, 40, 5, 22, 4, true, true, true),
           TOUCH_UI_SCROLL_DOWN, 4);
    expect(touchFileChooserHitTest(20, 184, 5, 22, 0, true, false, false),
           TOUCH_UI_EXIT);
    expect(touchFileChooserHitTest(20, 184, 5, 22, 0, false, false, false),
           TOUCH_UI_NONE);
    expect(touchFileChooserHitTest(20, 48, 5, 22, 0, true, false, false),
           TOUCH_UI_NONE);

    // A scrolled list still reports visible rows; the caller adds scrollY.
    expect(touchFileChooserHitTest(20, 16, 6, 22, 1, true, true, true),
           TOUCH_UI_ACTIVATE_ROW, 1);

    std::cout << "touch UI tests passed\n";
    return 0;
}
