#pragma once

enum TouchUiAction {
    TOUCH_UI_NONE,
    TOUCH_UI_HEADER_PREVIOUS,
    TOUCH_UI_HEADER_CLOSE,
    TOUCH_UI_HEADER_NEXT,
    TOUCH_UI_SELECT_ROW,
    TOUCH_UI_ACTIVATE_ROW,
    TOUCH_UI_VALUE_LEFT,
    TOUCH_UI_VALUE_RIGHT,
    TOUCH_UI_SCROLL_UP,
    TOUCH_UI_SCROLL_DOWN,
    TOUCH_UI_EXIT
};

struct TouchUiDecision {
    TouchUiAction action;
    int visibleRow;
};

struct TouchUiDebounce {
    bool armed;
    bool wasDown;
};

void touchUiBegin(TouchUiDebounce* state);
bool touchUiPressEdge(TouchUiDebounce* state, bool down);

TouchUiDecision touchMenuHitTest(int x, int y, int visibleRows,
                                 int selectedVisibleRow,
                                 bool selectedHasValues);
TouchUiDecision touchFileChooserHitTest(int x, int y, int visibleRows,
                                        int pageRows,
                                        int selectedVisibleRow,
                                        bool canQuit,
                                        bool canScrollUp,
                                        bool canScrollDown);

int touchVisibleRowToSource(const bool* visible, int sourceRows,
                            int visibleRow);
int touchWrappedValue(int current, int count, int direction);
