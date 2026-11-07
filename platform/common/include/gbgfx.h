#pragma once

#ifdef DS
#include <nds.h>
#endif

enum Icons {
    ICON_NULL=0,
    ICON_PRINTER
};

#ifdef DS
extern volatile int dsFrameCounter;
#endif

extern bool probingForBorder;

extern int interruptWaitMode;
extern int scaleMode;
extern int scaleFilter;
extern int scalePan; // 0 (top) .. 4 (bottom): visible part for x1.4..x1.6

// Values of scaleMode, in the order of the "Scaling" menu option.
enum {
    SCALE_OFF = 0,
    SCALE_1_1,
    SCALE_1_2,
    SCALE_1_3,
    SCALE_ASPECT, // fills the screen height, about x1.33
    SCALE_1_4,
    SCALE_1_5,
    SCALE_1_6,    // fills the screen width
    SCALE_FULL,   // fills the whole screen (stretched)
    NUM_SCALE_MODES
};
#define SCALE_PAN_STEPS 5
extern u8 gfxMask;
extern volatile int loadedBorderType;
extern bool customBorderExists;
extern bool sgbBorderLoaded;


void doAtVBlank(void (*func)(void));

void initGFX();
void refreshGFX();
void clearGFX();
void resetSgbBorder();

void drawScanline(int scanline);
void drawScanline_P2(int scanline);
void drawScreen();

void displayIcon(int iconid);

void selectBorder(); // Starts the file chooser for a border
int loadBorder(const char* filename); // Loads the border to vram
void checkBorder(); // Decides what kind of border to use, invokes loadBorder if necessary

void refreshScaleMode();

void refreshSgbPalette();
void setSgbMask(int mask);
void setSgbTiles(u8* src, u8 flags);
void setSgbMap(u8* src);

void writeVram(u16 addr, u8 val);
void writeVram16(u16 addr, u16 src);
void writeHram(u16 addr, u8 val);
void handleVideoRegister(u8 ioReg, u8 val);

enum {
    BORDER_NONE=0,
    BORDER_SGB,
    BORDER_CUSTOM
};  // Use with loadedBorderType
