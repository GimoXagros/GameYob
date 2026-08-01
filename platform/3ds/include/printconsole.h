#pragma once
#include <3ds/gfx.h>

struct PrintConsole {
    int cursorX, cursorY;
    int prevCursorX, prevCursorY;
    int consoleWidth, consoleHeight;
    gfxScreen_t screen;
    u8* framebuffer;
    bool redraw;
};

void consoleInitBottom();
void consoleSetScreen(gfxScreen_t screen);
void consoleCheckFramebuffers();
void consolePrintChar(char c);
void consolePrintGlyph(const u8* bitmap);

const int CHAR_WIDTH = 8;
const int CHAR_HEIGHT = 8;

