#pragma once
#include <3ds/gfx.h>

u32 getPixel(u8* framebuffer, int x, int y);
void drawPixel(u8* framebuffer, int x, int y, u32 color);
void drawRgb24Frame(u8* framebuffer, int destX, int destY,
        const u32* pixels, int width, int height);
void drawRgb24FrameScaled(u8* framebuffer, int destX, int destY,
        int destWidth, int destHeight, const u32* pixels,
        int sourceWidth, int sourceHeight, bool filter);

u8* gfxGetActiveFramebuffer(gfxScreen_t screen, gfx3dSide_t side);
u8* gfxGetInactiveFramebuffer(gfxScreen_t screen, gfx3dSide_t side);
void gfxInitFramebufferTracking();
void gfxMySwapBuffers();

inline u32 RGB24(int red, int green, int blue) {
    return red<<16 | green<<8 | blue;
}

const int TOP_SCREEN_WIDTH = 400;
const int TOP_SCREEN_HEIGHT = 240;

const int BOTTOM_SCREEN_WIDTH = 320;
const int BOTTOM_SCREEN_HEIGHT = 240;

const int framebufferSizes[] = {288000, 230400};
