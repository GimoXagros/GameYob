#include <3ds.h>
#include <string.h>
#include <math.h>
#include "gbgfx.h"
#include "gameboy.h"
#include "3dsgfx.h"
#include "menu.h"
#include "inputhelper.h"
#include "filechooser.h"
#include "soundengine.h"
#include "console.h"
#include "localization.h"
#include "image_loader.h"
#include "sgbborder.h"
#include "gb_render_rules.h"

// public variables

bool probingForBorder;

int interruptWaitMode;
int scaleMode;
int scaleFilter;
u8 gfxMask;
volatile int loadedBorderType;
bool customBorderExists;
bool sgbBorderLoaded;



// private variables

int lastGameScreen = -1;
SgbBorderData sgbBorderData;

u32 gbColors[4];
u32 pixels[32*32*64];

int scale = 3;

u32 bgPalettes[8][4];
u32* bgPalettesRef[8][4];
u32 sprPalettes[8][4];
u32* sprPalettesRef[8][4];

int dmaLine;
bool lineModified;

static DecodedImage customBorderImage = {0, 0, NULL};
    

// For drawScanline / drawSprite

u8 spritePixels[256];
u32 spritePixelsTrue[256]; // Holds the palettized colors
bool spriteBehindBg[256];

u8 bgPixels[256];
u32 bgPixelsTrue[256];
u8 bgPixelsLow[256];
u32 bgPixelsTrueLow[256];

// The native 3DS backend renders a complete Game Boy frame here first. This
// avoids exposing a framebuffer while its scanlines still belong to different
// emulated frames and mirrors the completed-frame submission used by the DS
// backend.
u32 gameFrame[160*144];


bool bgPalettesModified[8];
bool sprPalettesModified[8];


// Private functions
void drawSprite(int scanline, int spriteNum);

void updateBgPalette(int paletteid);
void updateBgPaletteDMG();
void updateSprPalette(int paletteid);
void updateSprPaletteDMG(int paletteid);
void resetPaletteReferences();
void drawSgbBorder(u8* framebuffer, int screenWidth);
void drawCustomBorder(u8* framebuffer, int screenWidth);
void drawMaskedScanline(int scanline, u32 color);
void clearGameArea(u32 color);


// Function definitions

void doAtVBlank(void (*func)(void)) {
    func();
}

void resetPaletteReferences() {
    // DMG/SGB palette registers are implemented by redirecting these
    // pointers. A dual-mode cartridge is first run as SGB to capture its
    // border and then reset as CGB; without restoring the identity mapping,
    // that CGB run keeps the last SGB shade permutation and displays valid
    // sprite/background pixels as black or with the wrong colour.
    for (int palette=0; palette<8; ++palette) {
        for (int color=0; color<4; ++color) {
            sprPalettesRef[palette][color] =
                &sprPalettes[palette][color];
            bgPalettesRef[palette][color] =
                &bgPalettes[palette][color];
        }
    }
}

void initGFX()
{
    sgbBorderReset(&sgbBorderData);
    loadedBorderType = BORDER_NONE;
    sgbBorderLoaded = false;
    gfxMask = 0;

	bgPalettes[0][0] = RGB24(255, 255, 255);
	bgPalettes[0][1] = RGB24(192, 192, 192);
	bgPalettes[0][2] = RGB24(94, 94, 94);
	bgPalettes[0][3] = RGB24(0, 0, 0);
	sprPalettes[0][0] = RGB24(255, 255, 255);
	sprPalettes[0][1] = RGB24(192, 192, 192);
	sprPalettes[0][2] = RGB24(94, 94, 94);
	sprPalettes[0][3] = RGB24(0, 0, 0);
	sprPalettes[1][0] = RGB24(255, 255, 255);
	sprPalettes[1][1] = RGB24(192, 192, 192);
	sprPalettes[1][2] = RGB24(94, 94, 94);
	sprPalettes[1][3] = RGB24(0, 0, 0);

    resetPaletteReferences();

    memset(bgPalettesModified, 0, sizeof(bgPalettesModified));
    memset(sprPalettesModified, 0, sizeof(sprPalettesModified));
}

void refreshGFX() {
    // SGB border capture can leave MASK_EN active while the cartridge is
    // reset into its requested GB/GBC mode. The DS renderer clears this mask
    // during refresh; do the same here or the captured border surrounds a
    // permanently white/black game area.
    gfxMask = 0;
    resetPaletteReferences();
    for (int i=0; i<8; i++) {
        bgPalettesModified[i] = true;
        sprPalettesModified[i] = true;
    }
}

void clearGFX() {

}

void resetSgbBorder() {
    sgbBorderLoaded = false;
    gfxMask = 0;
    sgbBorderReset(&sgbBorderData);

    // checkBorder() clears both active and inactive framebuffers before it
    // applies an optional user-selected custom border.
    checkBorder();
}

void drawScanline(int scanline)
{
}

void drawScanline_P2(int scanline) {
    int tileSigned;
    int BGMapAddr;
    int winMapAddr;

    for (int i=0; i<8; i++) {
        if (bgPalettesModified[i]) {
            if (gameboy->gbMode == GB)
                updateBgPaletteDMG();
            else
                updateBgPalette(i);
            bgPalettesModified[i] = false;
        }
        if (sprPalettesModified[i]) {
            if (gameboy->gbMode == GB)
                updateSprPaletteDMG(i);
            else
                updateSprPalette(i);
            sprPalettesModified[i] = false;
        }
    }

    if (gameboy->sgbMode && scanline == 0)
        refreshSgbPalette();

    if (gfxMask == 1)
        return;
    if (gfxMask == 2) {
        drawMaskedScanline(scanline, RGB24(0, 0, 0));
        return;
    }
    if (gfxMask == 3) {
        drawMaskedScanline(scanline, *bgPalettesRef[0][0]);
        return;
    }

	if (gameboy->ioRam[0x40] & 0x10) {	// Tile Data location
		tileSigned = 0;
	}
	else {
		tileSigned = 1;
	}

	if (gameboy->ioRam[0x40] & 0x8) {		// Tile Map location
		BGMapAddr = 0x1C00;
	}
	else {
		BGMapAddr = 0x1800;
	}
	if (gameboy->ioRam[0x40] & 0x40)
		winMapAddr = 0x1C00;
	else
		winMapAddr = 0x1800;

    /*
	for (int i=0; i<256; i++) {
		//bgPixels[i] = 5;
		//bgPixelsLow[i] = 5;
	}
    */
    memset(bgPixels, 0, sizeof(bgPixels));
    memset(bgPixelsLow, 0, sizeof(bgPixelsLow));
    memset(spritePixels, 0, sizeof(spritePixels));
    memset(spriteBehindBg, 0, sizeof(spriteBehindBg));
    for (int x=0; x<160; ++x) {
        bgPixelsTrueLow[x] = *bgPalettesRef[0][0];
    }

	if (gameboy->ioRam[0x40] & 0x2) { // Sprites enabled
        gb_render::SpriteRef sprites[10];
        int spriteCount = 0;
        const int height = (gameboy->ioRam[0x40] & 0x4) ? 16 : 8;
        for (int i=0; i<40 && spriteCount<10; ++i) {
            const int y = gameboy->hram[i*4] - 16;
            if (scanline >= y && scanline < y + height) {
                sprites[spriteCount].index = i;
                sprites[spriteCount].x = gameboy->hram[i*4+1] - 8;
                ++spriteCount;
            }
        }
        gb_render::sortSpritesForDrawing(sprites, spriteCount,
                gameboy->gbMode == GB);
        for (int i=0; i<spriteCount; ++i)
            drawSprite(scanline, sprites[i].index);
	}

	int winY = gameboy->ioRam[0x4A];
    const gb_render::WindowSpan window = gb_render::windowSpan(
            gameboy->ioRam[0x4B], winY, scanline,
            (gameboy->ioRam[0x40] & 0x20) != 0);
    bool drawingWindow = window.visible;

    int BGOn = 1;
    if (!(gameboy->gbMode == CGB) && (gameboy->ioRam[0x40] & 1) == 0) {
        BGOn = 0;
    }

	if (BGOn) {
		u8 scrollX = gameboy->ioRam[0x43];
		int scrollY = gameboy->ioRam[0x42];
		// The y position (measured in tiles)
		int tileY = ((scanline+scrollY)&0xFF)/8;

        int numTilesX = 20;
        int startTile = scrollX/8;
        int endTile = (startTile+numTilesX+1)&31;

		for (int i=startTile; i!=endTile; i=(i+1)&31)
		{
			int mapAddr = BGMapAddr+i+(tileY*32);		// The address (from beginning of gameboy->vram) of the tile's mapping
			// This is the tile id.
			int tileNum = gameboy->vram[0][mapAddr];
			if (tileSigned)
				tileNum = ((s8)tileNum)+256;

			int flipX = 0, flipY = 0;
			int bank = 0;
			int paletteid = 0;
			int priority = 0;

			if (gameboy->gbMode == CGB)
			{
				flipX = gameboy->vram[1][mapAddr] & 0x20;
				flipY = gameboy->vram[1][mapAddr] & 0x40;
				bank = !!(gameboy->vram[1][mapAddr] & 0x8);
				paletteid = gameboy->vram[1][mapAddr] & 0x7;
				priority = gameboy->vram[1][mapAddr] & 0x80;
			}

			// This is the tile's Y position to be read (0-7)
			int pixelY = (scanline+scrollY)%8;
			if (flipY)
				pixelY = 7-pixelY;

			for (int x=0; x<8; x++)
			{
				int colorid;
				u32 color;

				if (flipX)
				{
					colorid = !!(gameboy->vram[bank][(tileNum<<4)+(pixelY<<1)] & (0x80>>(7-x)));
					colorid |= !!(gameboy->vram[bank][(tileNum<<4)+(pixelY<<1)+1] & (0x80>>(7-x)))<<1;
				}
				else
				{
					colorid = !!(gameboy->vram[bank][(tileNum<<4)+(pixelY<<1)] & (0x80>>x));
					colorid |= !!(gameboy->vram[bank][(tileNum<<4)+(pixelY<<1)+1] & (0x80>>x))<<1;
				}
				// The x position to write to pixels[].
				u32 writeX = ((i*8)+x-scrollX)&0xFF;
                if (gameboy->sgbMode && writeX < 160)
                    paletteid = gameboy->sgbMap[(scanline / 8) * 20 + (writeX / 8)] & 3;

				color = *bgPalettesRef[paletteid][colorid];
				if (priority) {
					bgPixels[writeX] = colorid;
					bgPixelsTrue[writeX] = color;
					bgPixelsLow[writeX] = colorid;
					bgPixelsTrueLow[writeX] = color;
				}
				else {
					bgPixelsLow[writeX] = colorid;
					bgPixelsTrueLow[writeX] = color;
				}
				//spritePixels[writeX] = 0;
			}
		}
	}

	// Draw window

	if (drawingWindow)
	{
		int tileY = (scanline-winY)/8;
        int pixelY = (scanline-winY)%8;
        for (int writeX=window.screenStart; writeX<160; ++writeX) {
            const int windowX = window.sourceStart +
                (writeX - window.screenStart);
            const int tileX = windowX / 8;
            const int sourceX = windowX & 7;
            const int mapAddr = winMapAddr + tileX + tileY*32;
            int tileNum = gameboy->vram[0][mapAddr];
            if (tileSigned)
                tileNum = ((s8)tileNum)+256;

            int flipX = 0, flipY = 0;
            int bank = 0;
            int paletteid = 0;
            int priority = 0;
            if (gameboy->gbMode == CGB) {
                flipX = gameboy->vram[1][mapAddr] & 0x20;
                flipY = gameboy->vram[1][mapAddr] & 0x40;
                bank = !!(gameboy->vram[1][mapAddr]&0x8);
                paletteid = gameboy->vram[1][mapAddr]&0x7;
                priority = gameboy->vram[1][mapAddr] & 0x80;
            }

            int sourceY = flipY ? 7-pixelY : pixelY;
            int bit = flipX ? sourceX : 7-sourceX;
            int colorid =
                !!(gameboy->vram[bank][(tileNum<<4)+(sourceY<<1)] & (1<<bit));
            colorid |=
                !!(gameboy->vram[bank][(tileNum<<4)+(sourceY<<1)+1] & (1<<bit))<<1;
            if (gameboy->sgbMode)
                paletteid = gameboy->sgbMap[(scanline / 8) * 20 +
                    (writeX / 8)] & 3;

            const u32 color = *bgPalettesRef[paletteid][colorid];
            bgPixelsLow[writeX] = colorid;
            bgPixelsTrueLow[writeX] = color;
            if (priority) {
                bgPixels[writeX] = colorid;
                bgPixelsTrue[writeX] = color;
            }
            else
                bgPixels[writeX] = 0;
        }
	}

    for (int i=0; i<160; i++)
    {
        bool spriteWins = false;
        if (spritePixels[i] != 0) {
            if (gameboy->gbMode == CGB) {
                spriteWins = gb_render::cgbSpriteWins(
                        (gameboy->ioRam[0x40] & 1) != 0,
                        bgPixels[i] != 0, bgPixelsLow[i],
                        spriteBehindBg[i]);
            }
            else {
                spriteWins = !BGOn || bgPixelsLow[i] == 0 ||
                    !spriteBehindBg[i];
            }
        }

        if (spriteWins)
            gameFrame[scanline*160+i] = spritePixelsTrue[i];
        else if (bgPixels[i] != 0)
            gameFrame[scanline*160+i] = bgPixelsTrue[i];
        else
            gameFrame[scanline*160+i] = bgPixelsTrueLow[i];
    }
}

void drawSprite(int scanline, int spriteNum)
{
    // The sprite's number, times 4 (each uses 4 bytes)
    spriteNum *= 4;

    int y = (gameboy->hram[spriteNum]-16);
    int height;
    if (gameboy->ioRam[0x40] & 0x4)
        height = 16;
	else
		height = 8;

    if (scanline < y || scanline >= y+height) {
        return;
    }

	int x = (gameboy->hram[spriteNum+1]-8);
	int tileNum = gameboy->hram[spriteNum+2];
	int bank = 0;
	int flipX = (gameboy->hram[spriteNum+3] & 0x20);
	int flipY = (gameboy->hram[spriteNum+3] & 0x40);
	const bool behindBg = (gameboy->hram[spriteNum+3] & 0x80) != 0;
	int paletteid;
    int dmgPalette = 0;

	if (gameboy->gbMode == CGB)
	{
		bank = (gameboy->hram[spriteNum+3]&0x8)>>3;
		paletteid = gameboy->hram[spriteNum+3] & 0x7;
	}
	else
	{
        dmgPalette = (gameboy->hram[spriteNum+3] & 0x10) ? 4 : 0;
		paletteid = dmgPalette;
	}

    if (height == 16) {
        tileNum &= ~1;

        if (scanline-y >= 8)
            tileNum++;
    }

    //u8* tile = &memory[0]+((tileNum)*16)+0x8000;//tileAddr;
    int pixelY = (scanline-y)%8;

    if (flipY)
    {
        pixelY = 7-pixelY;
        if (height == 16)
            tileNum = tileNum^1;
    }
    for (int j=0; j<8; j++)
    {
        int color;
        int trueColor;

        color = !!(gameboy->vram[bank][(tileNum<<4)+(pixelY<<1)] & (0x80>>j));
        color |= !!(gameboy->vram[bank][(tileNum<<4)+(pixelY<<1)+1] & (0x80>>j))<<1;
        if (color != 0)
        {
            const int writeX = flipX ? x + (7-j) : x + j;
            if (writeX < 0 || writeX >= 160)
                continue;
            if (gameboy->sgbMode && writeX >= 0 && writeX < 160)
                paletteid = dmgPalette +
                    (gameboy->sgbMap[(scanline / 8) * 20 + (writeX / 8)] & 3);
            trueColor = *sprPalettesRef[paletteid][color];
            spritePixels[writeX] = color;
            spritePixelsTrue[writeX] = trueColor;
            spriteBehindBg[writeX] = behindBg;
        }
    }
}

void drawScreen()
{
    u8* framebuffer;
    int screenWidth;
    if (gameScreen == 0) {
        framebuffer = gfxGetInactiveFramebuffer(GFX_TOP, GFX_LEFT);
        screenWidth = TOP_SCREEN_WIDTH;
    }
    else {
        framebuffer = gfxGetInactiveFramebuffer(GFX_BOTTOM, GFX_LEFT);
        screenWidth = BOTTOM_SCREEN_WIDTH;
    }

    // Scaling is intentionally unavailable in the v0.5.5-ko native 3DS
    // build. Real-hardware testing found both the CPU and PICA200 prototypes
    // unsuitable for release, so keep the verified centered direct path.
    const int offsetX = (screenWidth-160)/2;
    const int offsetY = (TOP_SCREEN_HEIGHT-144)/2;
    drawRgb24Frame(framebuffer, offsetX, offsetY, gameFrame, 160, 144);

    if (!(fastForwardMode || fastForwardKey))
        system_waitForVBlank();
}


void displayIcon(int iconid) {

}


void selectBorder() {
    muteSND();

    if (borderChooserState.directory == "/" && borderPath[0] != '\0') {
        char directory[MAX_FILENAME_LEN];
        strncpy(directory, borderPath, sizeof(directory));
        directory[sizeof(directory)-1] = '\0';
        char* slash = strrchr(directory, '/');
        if (slash) {
            setFileChooserMatchFile(slash + 1);
            if (slash == directory)
                slash[1] = '\0';
            else
                *slash = '\0';
            borderChooserState.directory = directory;
        }
    }

    loadFileChooserState(&borderChooserState);
    const char* extensions[] = {"png", "bmp"};
    char* filename = startFileChooser(extensions, 2, false, true);
    if (filename) {
        char cwd[MAX_FILENAME_LEN];
        fs_getcwd(cwd, sizeof(cwd));
        const size_t cwdLength = strlen(cwd);
        const int written = snprintf(borderPath, sizeof(borderPath), "%s%s%s",
            cwd, cwdLength > 0 && cwd[cwdLength-1] == '/' ? "" : "/",
            filename);
        free(filename);

        borderPathExists = written >= 0 &&
            static_cast<size_t>(written) < sizeof(borderPath);
        if (borderPathExists && loadBorder(borderPath) == 0) {
            customBordersEnabled = true;
            setMenuOption("Custom Border", 1);
        }
        checkBorder();
    }

    saveFileChooserState(&borderChooserState);
    loadFileChooserState(&romChooserState);
    unmuteSND();
}

int loadBorder(const char* filename) {
    if (!filename || !filename[0] || !borderPathExists)
        return 1;

    DecodedImage image = {0, 0, NULL};
    if (!decodeImageFile(filename, &image)) {
        printLog("%s: %s\n", tr("Error opening border."), filename);
        borderPathExists = false;
        customBorderExists = false;
        return 1;
    }

    freeDecodedImage(&customBorderImage);
    customBorderImage = image;
    customBorderExists = true;
    borderPathExists = true;
    return 0;
}

void checkBorder() {
    lastGameScreen = gameScreen;

    u8* buffers[2];
    int screenWidth;
    if (gameScreen == 0) {
        buffers[0] = gfxGetActiveFramebuffer(GFX_TOP, GFX_LEFT);
        buffers[1] = gfxGetInactiveFramebuffer(GFX_TOP, GFX_LEFT);
        screenWidth = TOP_SCREEN_WIDTH;
    }
    else {
        buffers[0] = gfxGetActiveFramebuffer(GFX_BOTTOM, GFX_LEFT);
        buffers[1] = gfxGetInactiveFramebuffer(GFX_BOTTOM, GFX_LEFT);
        screenWidth = BOTTOM_SCREEN_WIDTH;
    }

    loadedBorderType = BORDER_NONE;
    if (scaleMode == 0 && sgbBordersEnabled && sgbBorderLoaded) {
        loadedBorderType = BORDER_SGB;
    }
    else if (scaleMode == 0 && customBordersEnabled) {
        if (!customBorderExists)
            loadBorder(borderPath);
        if (customBorderExists)
            loadedBorderType = BORDER_CUSTOM;
    }

    for (int fb=0; fb<2; fb++) {
        memset(buffers[fb], 0, framebufferSizes[gameScreen]);
        if (loadedBorderType == BORDER_SGB)
            drawSgbBorder(buffers[fb], screenWidth);
        else if (loadedBorderType == BORDER_CUSTOM)
            drawCustomBorder(buffers[fb], screenWidth);
    }
}

void refreshScaleMode() {
    checkBorder();
}


void refreshSgbPalette() {
    for (int i=0; i<4; i++)
        updateBgPalette(i);
    for (int i=0; i<4; i++)
        updateSprPalette(i);
    for (int i=0; i<4; i++) {
        for (int color=0; color<4; color++)
            sprPalettes[i+4][color] = sprPalettes[i][color];
    }

    updateBgPaletteDMG();
    for (int i=0; i<8; i++)
        updateSprPaletteDMG(i);
}
void setSgbMask(int mask) {
    gfxMask = mask & 3;
    if (gfxMask == 1) {
        const gfxScreen_t screen = gameScreen == 0 ? GFX_TOP : GFX_BOTTOM;
        memcpy(gfxGetInactiveFramebuffer(screen, GFX_LEFT),
            gfxGetActiveFramebuffer(screen, GFX_LEFT),
            framebufferSizes[gameScreen]);
    }
}
void setSgbTiles(u8* src, u8 flags) {
    if (!sgbBorderLoaded && sgbBorderData.mapLoaded)
        sgbBorderReset(&sgbBorderData);
    sgbBorderDecodeTiles(&sgbBorderData, src, (flags & 1) != 0);
}
void setSgbMap(u8* src) {
    sgbBorderDecodeMap(&sgbBorderData, src);
    sgbBorderLoaded = true;
    checkBorder();
    if (probingForBorder) {
        probingForBorder = false;
        gameboy->resetGameboy();
    }
}

void drawSgbBorder(u8* framebuffer, int screenWidth) {
    const int offsetX = (screenWidth - SGB_BORDER_WIDTH) / 2;
    const int offsetY = (TOP_SCREEN_HEIGHT - SGB_BORDER_HEIGHT) / 2;

    for (int y=0; y<SGB_BORDER_HEIGHT; y++) {
        for (int x=0; x<SGB_BORDER_WIDTH; x++) {
            // Keep the complete 160x144 opening owned by the Game Boy
            // renderer. Some cartridges leave non-zero transfer data in this
            // area; compositing it again produces 8x8 black blocks on 3DS.
            if (x >= 48 && x < 48+160 && y >= 40 && y < 40+144)
                continue;

            u8 palette;
            const u8 color = sgbBorderGetPixel(&sgbBorderData, x, y, &palette);
            drawPixel(framebuffer, offsetX+x, offsetY+y,
                sgbBorderColorToRgb24(sgbBorderData.palettes[palette][color]));
        }
    }
}

void drawCustomBorder(u8* framebuffer, int screenWidth) {
    if (!customBorderImage.pixels || customBorderImage.width <= 0 ||
            customBorderImage.height <= 0)
        return;

    int drawWidth = customBorderImage.width;
    int drawHeight = customBorderImage.height;
    if (drawWidth > screenWidth || drawHeight > TOP_SCREEN_HEIGHT) {
        const double xScale = (double)screenWidth / drawWidth;
        const double yScale = (double)TOP_SCREEN_HEIGHT / drawHeight;
        const double fitScale = xScale < yScale ? xScale : yScale;
        drawWidth = (int)(drawWidth * fitScale);
        drawHeight = (int)(drawHeight * fitScale);
    }

    const int offsetX = (screenWidth - drawWidth) / 2;
    const int offsetY = (TOP_SCREEN_HEIGHT - drawHeight) / 2;
    for (int y=0; y<drawHeight; ++y) {
        const int sourceY = y * customBorderImage.height / drawHeight;
        for (int x=0; x<drawWidth; ++x) {
            const int sourceX = x * customBorderImage.width / drawWidth;
            const unsigned char* pixel = customBorderImage.pixels +
                (sourceY * customBorderImage.width + sourceX) * 3;
            drawPixel(framebuffer, offsetX+x, offsetY+y,
                RGB24(pixel[0], pixel[1], pixel[2]));
        }
    }
}

void drawMaskedScanline(int scanline, u32 color) {
    if (scanline < 0 || scanline >= 144)
        return;
    for (int x=0; x<160; x++)
        gameFrame[scanline*160+x] = color;
}

void clearGameArea(u32 color) {
    u8* buffers[2];
    int screenWidth;
    if (gameScreen == 0) {
        buffers[0] = gfxGetActiveFramebuffer(GFX_TOP, GFX_LEFT);
        buffers[1] = gfxGetInactiveFramebuffer(GFX_TOP, GFX_LEFT);
        screenWidth = TOP_SCREEN_WIDTH;
    }
    else {
        buffers[0] = gfxGetActiveFramebuffer(GFX_BOTTOM, GFX_LEFT);
        buffers[1] = gfxGetInactiveFramebuffer(GFX_BOTTOM, GFX_LEFT);
        screenWidth = BOTTOM_SCREEN_WIDTH;
    }
    const int offsetX = screenWidth / 2 - 160/2;
    const int offsetY = TOP_SCREEN_HEIGHT / 2 - 144/2;
    for (int i=0; i<160*144; ++i)
        gameFrame[i] = color;
    for (int buffer=0; buffer<2; ++buffer) {
        drawRgb24Frame(buffers[buffer], offsetX, offsetY,
            gameFrame, 160, 144);
    }
}


void writeVram(u16 addr, u8 val) {
}

void writeVram16(u16 addr, u16 src) {
}

void writeHram(u16 addr, u8 val) {
}

void handleVideoRegister(u8 ioReg, u8 val) {
    switch(ioReg) {
        case 0x40:
            if ((gameboy->ioRam[0x40] & 0x80) && !(val & 0x80))
                clearGameArea(RGB24(255, 255, 255));
            break;
        case 0x47:
            if (gameboy->gbMode == GB)
                bgPalettesModified[0] = true;
            break;
        case 0x48:
            if (gameboy->gbMode == GB) {
                for (int i=0; i<4; ++i)
                    sprPalettesModified[i] = true;
            }
            break;
        case 0x49:
            if (gameboy->gbMode == GB) {
                for (int i=4; i<8; ++i)
                    sprPalettesModified[i] = true;
            }
            break;
        case 0x69:
            if (gameboy->gbMode == CGB)
                bgPalettesModified[(gameboy->ioRam[0x68]/8)&7] = true;
            break;
        case 0x6B:
            if (gameboy->gbMode == CGB)
                sprPalettesModified[(gameboy->ioRam[0x6A]/8)&7] = true;
            break;
        default:
            break;
    }
}

void updateBgPalette(int paletteid)
{
    int multiplier = 8;
    int i;
    for (i=0; i<4; i++)
    {
        int red = (gameboy->bgPaletteData[(paletteid*8)+(i*2)]&0x1F)*multiplier;
        int green = (((gameboy->bgPaletteData[(paletteid*8)+(i*2)]&0xE0) >> 5) |
                ((gameboy->bgPaletteData[(paletteid*8)+(i*2)+1]) & 0x3) << 3)*multiplier;
        int blue = ((gameboy->bgPaletteData[(paletteid*8)+(i*2)+1] >> 2) & 0x1F)*multiplier;
        bgPalettes[paletteid][i] = RGB24(red, green, blue);
    }
}

void updateBgPaletteDMG()
{
	u8 val = gameboy->ioRam[0x47];
	int palette[] = {val&3, (val>>2)&3, (val>>4)&3, (val>>6)};

    const int paletteCount = gameboy->sgbMode ? 4 : 1;
    for (int paletteid=0; paletteid<paletteCount; paletteid++) {
        for (int i=0; i<4; i++)
            bgPalettesRef[paletteid][i] = &bgPalettes[paletteid][palette[i]];
    }
}

void updateSprPalette(int paletteid)
{
    int multiplier = 8;
    int i;
    for (i=0; i<4; i++)
    {
        int red = (gameboy->sprPaletteData[(paletteid*8)+(i*2)]&0x1F)*multiplier;
        int green = (((gameboy->sprPaletteData[(paletteid*8)+(i*2)]&0xE0) >> 5) |
                ((gameboy->sprPaletteData[(paletteid*8)+(i*2)+1]) & 0x3) << 3)*multiplier;
        int blue = ((gameboy->sprPaletteData[(paletteid*8)+(i*2)+1] >> 2) & 0x1F)*multiplier;
        sprPalettes[paletteid][i] = RGB24(red, green, blue);
    }
}

void updateSprPaletteDMG(int paletteid)
{
    // Palettes 0-3 use OBP0 and palettes 4-7 use OBP1. The groups allow SGB
    // attribute regions to select a base palette while retaining the two DMG
    // object-palette registers; normal DMG rendering uses groups 0 and 4.
    const int registerIndex = paletteid >= 4 ? 1 : 0;
	int val = gameboy->ioRam[0x48+registerIndex];
	int palette[] = {val&3, (val>>2)&3, (val>>4)&3, (val>>6)};

	for (int i=0; i<4; i++)
		sprPalettesRef[paletteid][i] = &sprPalettes[paletteid][palette[i]];
}

