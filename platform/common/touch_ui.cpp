#include "touch_ui.h"

namespace {

const int TOUCH_WIDTH = 256;
const int TOUCH_HEIGHT = 192;
const int MENU_HEADER_HEIGHT = 8;
const int MENU_ROW_HEIGHT = 16;
const int MENU_HEADER_SIDE_WIDTH = 64;
const int FILE_HEADER_HEIGHT = 8;
const int FILE_ROW_HEIGHT = 8;
const int FILE_SCROLL_WIDTH = 16;

TouchUiDecision decision(TouchUiAction action, int row = -1) {
    TouchUiDecision result = {action, row};
    return result;
}

bool pointOnScreen(int x, int y) {
    return x >= 0 && x < TOUCH_WIDTH && y >= 0 && y < TOUCH_HEIGHT;
}

} // namespace

void touchUiBegin(TouchUiDebounce* state) {
    if (!state)
        return;
    state->armed = false;
    state->wasDown = true;
}

bool touchUiPressEdge(TouchUiDebounce* state, bool down) {
    if (!state)
        return false;

    if (!state->armed) {
        if (!down) {
            state->armed = true;
            state->wasDown = false;
        }
        return false;
    }

    const bool pressed = down && !state->wasDown;
    state->wasDown = down;
    return pressed;
}

TouchUiDecision touchMenuHitTest(int x, int y, int visibleRows,
                                 int selectedVisibleRow,
                                 bool selectedHasValues) {
    if (!pointOnScreen(x, y) || visibleRows < 0)
        return decision(TOUCH_UI_NONE);

    if (y < MENU_HEADER_HEIGHT) {
        if (x < MENU_HEADER_SIDE_WIDTH)
            return decision(TOUCH_UI_HEADER_PREVIOUS);
        if (x >= TOUCH_WIDTH - MENU_HEADER_SIDE_WIDTH)
            return decision(TOUCH_UI_HEADER_NEXT);
        return decision(TOUCH_UI_HEADER_CLOSE);
    }

    const int row = (y - MENU_HEADER_HEIGHT) / MENU_ROW_HEIGHT;
    if (row < 0 || row >= visibleRows)
        return decision(TOUCH_UI_NONE);
    if (row != selectedVisibleRow)
        return decision(TOUCH_UI_SELECT_ROW, row);
    if (!selectedHasValues)
        return decision(TOUCH_UI_ACTIVATE_ROW, row);
    return decision(x < TOUCH_WIDTH / 2 ? TOUCH_UI_VALUE_LEFT :
                    TOUCH_UI_VALUE_RIGHT, row);
}

TouchUiDecision touchFileChooserHitTest(int x, int y, int visibleRows,
                                        int pageRows,
                                        int selectedVisibleRow,
                                        bool canQuit,
                                        bool canScrollUp,
                                        bool canScrollDown) {
    if (!pointOnScreen(x, y) || visibleRows < 0 || pageRows < 0 ||
            visibleRows > pageRows)
        return decision(TOUCH_UI_NONE);

    const int listBottom = FILE_HEADER_HEIGHT + visibleRows * FILE_ROW_HEIGHT;
    if (y >= FILE_HEADER_HEIGHT && y < listBottom) {
        const int row = (y - FILE_HEADER_HEIGHT) / FILE_ROW_HEIGHT;
        if (x < FILE_SCROLL_WIDTH && row == 0 && canScrollUp)
            return decision(TOUCH_UI_SCROLL_UP, row);
        if (x < FILE_SCROLL_WIDTH && row == visibleRows - 1 &&
                canScrollDown)
            return decision(TOUCH_UI_SCROLL_DOWN, row);
        if (row != selectedVisibleRow)
            return decision(TOUCH_UI_SELECT_ROW, row);
        return decision(TOUCH_UI_ACTIVATE_ROW, row);
    }

    const int exitTop = FILE_HEADER_HEIGHT + pageRows * FILE_ROW_HEIGHT;
    if (canQuit && y >= exitTop && y < exitTop + FILE_ROW_HEIGHT)
        return decision(TOUCH_UI_EXIT);
    return decision(TOUCH_UI_NONE);
}

int touchVisibleRowToSource(const bool* visible, int sourceRows,
                            int visibleRow) {
    if (!visible || sourceRows < 0 || visibleRow < 0)
        return -1;
    for (int source = 0; source < sourceRows; source++) {
        if (!visible[source])
            continue;
        if (visibleRow == 0)
            return source;
        visibleRow--;
    }
    return -1;
}

int touchWrappedValue(int current, int count, int direction) {
    if (count <= 0)
        return current;
    int value = (current + direction) % count;
    if (value < 0)
        value += count;
    return value;
}
