#include <3ds.h>
#include <string.h>
#include "3dsgfx.h"

static u8* presentedFramebuffers[2] = {NULL, NULL};
static u8* renderingFramebuffers[2] = {NULL, NULL};

u32 getPixel(u8* framebuffer, int x, int y) {
    u8* ptr = framebuffer + (x*TOP_SCREEN_HEIGHT + y)*3;
    return *ptr | *(ptr+1)<<8 | *(ptr+2)<<16;
}
void drawPixel(u8* framebuffer, int x, int y, u32 color) {
    y = TOP_SCREEN_HEIGHT - y - 1;

    u8* ptr = framebuffer + (x*TOP_SCREEN_HEIGHT + y)*3;

    ptr[0] = color;
    ptr[1] = color >> 8;
    ptr[2] = color >> 16;
}

void drawRgb24Frame(u8* framebuffer, int destX, int destY,
        const u32* pixels, int width, int height) {
    drawRgb24FrameScaled(framebuffer, destX, destY, width, height,
        pixels, width, height, false);
}

static u32 blendRgb24(u32 topLeft, u32 topRight, u32 bottomLeft,
        u32 bottomRight, int xWeight, int yWeight) {
    u32 result = 0;
    for (int shift=0; shift<=16; shift+=8) {
        const int a = (topLeft >> shift) & 0xff;
        const int b = (topRight >> shift) & 0xff;
        const int c = (bottomLeft >> shift) & 0xff;
        const int d = (bottomRight >> shift) & 0xff;
        const int upper = a * (256-xWeight) + b * xWeight;
        const int lower = c * (256-xWeight) + d * xWeight;
        const int value = (upper * (256-yWeight) +
            lower * yWeight + 32768) >> 16;
        result |= (u32)value << shift;
    }
    return result;
}

void drawRgb24FrameScaled(u8* framebuffer, int destX, int destY,
        int destWidth, int destHeight, const u32* pixels,
        int sourceWidth, int sourceHeight, bool filter) {
    if (!framebuffer || !pixels || destWidth <= 0 || destHeight <= 0 ||
            sourceWidth <= 0 || sourceHeight <= 0 ||
            destWidth > TOP_SCREEN_WIDTH || destHeight > TOP_SCREEN_HEIGHT)
        return;

    static int sourceX0[TOP_SCREEN_WIDTH];
    static int sourceX1[TOP_SCREEN_WIDTH];
    static int sourceXWeight[TOP_SCREEN_WIDTH];
    static int sourceY0[TOP_SCREEN_HEIGHT];
    static int sourceY1[TOP_SCREEN_HEIGHT];
    static int sourceYWeight[TOP_SCREEN_HEIGHT];

    for (int x=0; x<destWidth; ++x) {
        if (!filter) {
            sourceX0[x] = x * sourceWidth / destWidth;
            sourceX1[x] = sourceX0[x];
            sourceXWeight[x] = 0;
        }
        else {
            int sourceX = ((2*x+1) * sourceWidth * 256) /
                (2*destWidth) - 128;
            if (sourceX < 0)
                sourceX = 0;
            const int maxX = (sourceWidth-1) * 256;
            if (sourceX > maxX)
                sourceX = maxX;
            sourceX0[x] = sourceX >> 8;
            sourceX1[x] = sourceX0[x]+1 < sourceWidth ?
                sourceX0[x]+1 : sourceX0[x];
            sourceXWeight[x] = sourceX & 0xff;
        }
    }
    for (int y=0; y<destHeight; ++y) {
        if (!filter) {
            sourceY0[y] = y * sourceHeight / destHeight;
            sourceY1[y] = sourceY0[y];
            sourceYWeight[y] = 0;
        }
        else {
            int sourceY = ((2*y+1) * sourceHeight * 256) /
                (2*destHeight) - 128;
            if (sourceY < 0)
                sourceY = 0;
            const int maxY = (sourceHeight-1) * 256;
            if (sourceY > maxY)
                sourceY = maxY;
            sourceY0[y] = sourceY >> 8;
            sourceY1[y] = sourceY0[y]+1 < sourceHeight ?
                sourceY0[y]+1 : sourceY0[y];
            sourceYWeight[y] = sourceY & 0xff;
        }
    }

    // Build the complete Game Boy picture away from the scanout buffers and
    // submit it in one pass. The 3DS framebuffer is column-major BGR8.
    for (int x=0; x<destWidth; ++x) {
        u8* dest = framebuffer +
            ((destX + x) * TOP_SCREEN_HEIGHT +
             (TOP_SCREEN_HEIGHT - destY - destHeight)) * 3;
        for (int y=destHeight-1; y>=0; --y) {
            u32 color;
            if (!filter) {
                color = pixels[sourceY0[y]*sourceWidth+sourceX0[x]];
            }
            else {
                color = blendRgb24(
                    pixels[sourceY0[y]*sourceWidth+sourceX0[x]],
                    pixels[sourceY0[y]*sourceWidth+sourceX1[x]],
                    pixels[sourceY1[y]*sourceWidth+sourceX0[x]],
                    pixels[sourceY1[y]*sourceWidth+sourceX1[x]],
                    sourceXWeight[x], sourceYWeight[y]);
            }
            dest[0] = color;
            dest[1] = color >> 8;
            dest[2] = color >> 16;
            dest += 3;
        }
    }
}

u8* gfxGetActiveFramebuffer(gfxScreen_t screen, gfx3dSide_t side) {
    (void)side;
    if (presentedFramebuffers[screen])
        return presentedFramebuffers[screen];
    return gfxGetInactiveFramebuffer(screen, side);
}

u8* gfxGetInactiveFramebuffer(gfxScreen_t screen, gfx3dSide_t side) {
    renderingFramebuffers[screen] =
        gfxGetFramebuffer(screen, side, NULL, NULL);
    return renderingFramebuffers[screen];
}

void gfxInitFramebufferTracking() {
    // libctru intentionally keeps its framebuffer arrays private. Discover
    // both buffers through the supported render-target API and start them in
    // a known state so border/background drawing can update both safely.
    for (int screen=GFX_TOP; screen<=GFX_BOTTOM; ++screen) {
        u16 width, height;
        renderingFramebuffers[screen] = gfxGetFramebuffer(
            (gfxScreen_t)screen, GFX_LEFT, &width, &height);
        memset(renderingFramebuffers[screen], 0, width * height * 3);
    }
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    for (int screen=GFX_TOP; screen<=GFX_BOTTOM; ++screen) {
        presentedFramebuffers[screen] = renderingFramebuffers[screen];
        u16 width, height;
        renderingFramebuffers[screen] = gfxGetFramebuffer(
            (gfxScreen_t)screen, GFX_LEFT, &width, &height);
        memset(renderingFramebuffers[screen], 0, width * height * 3);
    }
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    for (int screen=GFX_TOP; screen<=GFX_BOTTOM; ++screen) {
        presentedFramebuffers[screen] = renderingFramebuffers[screen];
        renderingFramebuffers[screen] = gfxGetFramebuffer(
            (gfxScreen_t)screen, GFX_LEFT, NULL, NULL);
    }
}

void gfxMySwapBuffers() {
    u8* nextPresented[2] = {
        gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL),
        gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL)
    };
    gfxSwapBuffers();
    for (int screen=GFX_TOP; screen<=GFX_BOTTOM; ++screen) {
        presentedFramebuffers[screen] = nextPresented[screen];
        renderingFramebuffers[screen] = gfxGetFramebuffer(
            (gfxScreen_t)screen, GFX_LEFT, NULL, NULL);
    }
}
