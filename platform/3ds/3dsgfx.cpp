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
    if (!framebuffer || !pixels || width <= 0 || height <= 0 ||
            destX < 0 || destY < 0 ||
            destX + width > TOP_SCREEN_WIDTH ||
            destY + height > TOP_SCREEN_HEIGHT)
        return;

    // The native 3DS release uses a fixed 160x144 picture. Convert the
    // completed frame directly into the column-major BGR8 framebuffer without
    // building scale maps or touching pixels outside the Game Boy viewport.
    for (int x=0; x<width; ++x) {
        u8* dest = framebuffer +
            ((destX + x) * TOP_SCREEN_HEIGHT +
             (TOP_SCREEN_HEIGHT - destY - height)) * 3;
        for (int y=height-1; y>=0; --y) {
            const u32 color = pixels[y*width+x];
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
