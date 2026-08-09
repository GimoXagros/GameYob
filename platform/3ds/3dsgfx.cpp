#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include "3dsgfx.h"

static u8* presentedFramebuffers[2] = {NULL, NULL};
static u8* renderingFramebuffers[2] = {NULL, NULL};
static u8* knownFramebuffers[2][2] = {{NULL, NULL}, {NULL, NULL}};

static bool acceleratedRendererInitialized = false;
static bool acceleratedFramePending = false;
static bool acceleratedFrameInFlight = false;
static C3D_RenderTarget* acceleratedTargets[2] = {NULL, NULL};
static C3D_Tex gameTexture;
static Tex3DS_SubTexture gameSubTexture;
static C2D_Image gameImage;
static int lastTextureFilter = -1;

static inline unsigned int morton8(unsigned int x, unsigned int y) {
    return (x & 1) | ((y & 1) << 1) |
        ((x & 2) << 1) | ((y & 2) << 2) |
        ((x & 4) << 2) | ((y & 4) << 3);
}

static inline u16 rgb24To565(u32 color) {
    return RGB8_to_565(color >> 16, color >> 8, color);
}

static inline void setTexturePixel(u16* texture, int x, int y, u16 color) {
    const unsigned int tile = (y >> 3) * (256 / 8) + (x >> 3);
    texture[tile * 64 + morton8(x & 7, y & 7)] = color;
}

static void updateGameTexture(const u32* pixels) {
    u16* texture = static_cast<u16*>(gameTexture.data);
    for (int sourceY=0; sourceY<144; ++sourceY) {
        const int textureY = 143-sourceY;
        const u32* source = pixels + sourceY*160;
        for (int x=0; x<160; ++x)
            setTexturePixel(texture, x, textureY, rgb24To565(source[x]));

        // Duplicate the rightmost texel so linear filtering cannot sample the
        // unused part of the 256x256 allocation at the image boundary.
        setTexturePixel(texture, 160, textureY,
            rgb24To565(source[159]));
    }

    // The same one-texel guard is needed above the top row.
    for (int x=0; x<=160; ++x)
        setTexturePixel(texture, x, 144,
            rgb24To565(pixels[x < 160 ? x : 159]));
    C3D_TexFlush(&gameTexture);
}

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
        knownFramebuffers[screen][0] = renderingFramebuffers[screen];
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
        knownFramebuffers[screen][1] = renderingFramebuffers[screen];
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

bool gfxInitAcceleratedGameRenderer() {
    if (acceleratedRendererInitialized)
        return true;
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))
        return false;
    if (!C2D_Init(8)) {
        C3D_Fini();
        return false;
    }

    C2D_Prepare();
    acceleratedTargets[GFX_TOP] =
        C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    acceleratedTargets[GFX_BOTTOM] =
        C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!acceleratedTargets[GFX_TOP] ||
            !acceleratedTargets[GFX_BOTTOM] ||
            !C3D_TexInit(&gameTexture, 256, 256, GPU_RGB565)) {
        if (acceleratedTargets[GFX_TOP])
            C3D_RenderTargetDelete(acceleratedTargets[GFX_TOP]);
        if (acceleratedTargets[GFX_BOTTOM])
            C3D_RenderTargetDelete(acceleratedTargets[GFX_BOTTOM]);
        acceleratedTargets[GFX_TOP] = NULL;
        acceleratedTargets[GFX_BOTTOM] = NULL;
        C2D_Fini();
        C3D_Fini();
        return false;
    }

    memset(gameTexture.data, 0, gameTexture.size);
    C3D_TexSetWrap(&gameTexture, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    C3D_TexSetFilter(&gameTexture, GPU_NEAREST, GPU_NEAREST);
    C3D_TexFlush(&gameTexture);

    gameSubTexture.width = 160;
    gameSubTexture.height = 144;
    gameSubTexture.left = 0.0f;
    gameSubTexture.top = 144.0f/256.0f;
    gameSubTexture.right = 160.0f/256.0f;
    gameSubTexture.bottom = 0.0f;
    gameImage.tex = &gameTexture;
    gameImage.subtex = &gameSubTexture;

    lastTextureFilter = 0;
    acceleratedFramePending = false;
    acceleratedFrameInFlight = false;
    acceleratedRendererInitialized = true;
    return true;
}

bool gfxDrawAcceleratedGameFrame(gfxScreen_t screen, const u32* pixels,
        int destX, int destY, int destWidth, int destHeight,
        bool filter, bool syncToVBlank) {
    if (!acceleratedRendererInitialized || !pixels ||
            (screen != GFX_TOP && screen != GFX_BOTTOM) ||
            destWidth <= 0 || destHeight <= 0)
        return false;

    if (!C3D_FrameBegin(syncToVBlank ? C3D_FRAME_SYNCDRAW : 0))
        return false;

    // FrameBegin waits for the preceding command queue, including its screen
    // transfer and swap, before the shared dynamic texture is overwritten.
    acceleratedFrameInFlight = false;
    updateGameTexture(pixels);

    const int requestedFilter = filter ? 1 : 0;
    if (requestedFilter != lastTextureFilter) {
        C3D_TexSetFilter(&gameTexture,
            filter ? GPU_LINEAR : GPU_NEAREST,
            filter ? GPU_LINEAR : GPU_NEAREST);
        // Citro2D caches the currently bound texture. Mark its GPU state dirty
        // when the sampler changes so the new filter reaches PICA200.
        C2D_Prepare();
        lastTextureFilter = requestedFilter;
    }

    C2D_TargetClear(acceleratedTargets[screen], C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(acceleratedTargets[screen]);
    C2D_DrawImageAt(gameImage, destX, destY, 0.5f, NULL,
        destWidth/160.0f, destHeight/144.0f);
    C3D_FrameEnd(0);

    acceleratedFramePending = true;
    acceleratedFrameInFlight = true;
    return true;
}

bool gfxConsumeAcceleratedGameFrame() {
    const bool pending = acceleratedFramePending;
    acceleratedFramePending = false;
    return pending;
}

void gfxResyncFramebufferTracking() {
    for (int screen=GFX_TOP; screen<=GFX_BOTTOM; ++screen) {
        u8* back = gfxGetFramebuffer(
            (gfxScreen_t)screen, GFX_LEFT, NULL, NULL);
        renderingFramebuffers[screen] = back;
        if (knownFramebuffers[screen][0] == back)
            presentedFramebuffers[screen] = knownFramebuffers[screen][1];
        else if (knownFramebuffers[screen][1] == back)
            presentedFramebuffers[screen] = knownFramebuffers[screen][0];
    }
}

void gfxFinishAcceleratedGameFrame() {
    if (!acceleratedRendererInitialized || !acceleratedFrameInFlight)
        return;

    // Starting the next frame blocks until Citro3D's preceding render queue
    // and display transfer are complete. End the empty frame immediately;
    // this is used only when returning to direct framebuffer drawing.
    if (C3D_FrameBegin(0)) {
        acceleratedFrameInFlight = false;
        acceleratedFramePending = false;
        C3D_FrameEnd(0);
        gfxResyncFramebufferTracking();
    }
}

void gfxExitAcceleratedGameRenderer() {
    if (!acceleratedRendererInitialized)
        return;

    gfxFinishAcceleratedGameFrame();
    C3D_RenderTargetDelete(acceleratedTargets[GFX_TOP]);
    C3D_RenderTargetDelete(acceleratedTargets[GFX_BOTTOM]);
    C3D_TexDelete(&gameTexture);
    acceleratedTargets[GFX_TOP] = NULL;
    acceleratedTargets[GFX_BOTTOM] = NULL;
    C2D_Fini();
    C3D_Fini();
    acceleratedRendererInitialized = false;
}

void gfxMySwapBuffer(gfxScreen_t screen) {
    u8* nextPresented =
        gfxGetFramebuffer(screen, GFX_LEFT, NULL, NULL);
    gfxScreenSwapBuffers(screen, false);
    presentedFramebuffers[screen] = nextPresented;
    renderingFramebuffers[screen] =
        gfxGetFramebuffer(screen, GFX_LEFT, NULL, NULL);
}

void gfxMySwapBuffers() {
    gfxMySwapBuffer(GFX_TOP);
    gfxMySwapBuffer(GFX_BOTTOM);
}
