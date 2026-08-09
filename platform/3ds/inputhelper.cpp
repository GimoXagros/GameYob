#include <3ds.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <string>

#include "inputhelper.h"
#include "gameboy.h"
#include "main.h"
#include "console.h"
#include "menu.h"
#include "nifi.h"
#include "gbgfx.h"
#include "soundengine.h"
#include "cheats.h"
#include "gbs.h"
#include "filechooser.h"
#include "romfile.h"
#include "io.h"
#include "gbmanager.h"
#include "3dsgfx.h"

void audioExit();

u32 lastKeysPressed = 0;
u32 keysPressed = 0;
u32 keysJustPressed = 0;

u32 keysForceReleased=0;
u32 repeatStartTimer=0;
int repeatTimer=0;

u8 buttonsPressed;

bool fastForwardMode = false; // controlled by the toggle hotkey
bool fastForwardKey = false;  // only while its hotkey is pressed

bool biosExists = false;
int rumbleInserted = 0;


void initInput()
{
}

void flushFatCache() {
}


bool keyPressed(int key) {
    return keysPressed & key;
}
bool keyPressedAutoRepeat(int key) {
    if (keyJustPressed(key)) {
        repeatStartTimer = 14;
        return true;
    }
    if (keyPressed(key) && repeatStartTimer == 0 && repeatTimer == 0) {
        repeatTimer = 2;
        return true;
    }
    return false;
}
bool keyJustPressed(int key) {
    return keysJustPressed & key;
}

void forceReleaseKey(int key) {
    keysForceReleased |= key;
    keysPressed &= ~key;
}

void inputUpdateVBlank() {
    hidScanInput();
    lastKeysPressed = keysPressed;
    keysPressed = hidKeysHeld();

    for (int i=0; i<32; i++) {
        if (keysForceReleased & (1<<i)) {
            if (!(keysPressed & (1<<i)))
                keysForceReleased &= ~(1<<i);
        }
    }
    keysPressed &= ~keysForceReleased;

    keysJustPressed = (lastKeysPressed ^ keysPressed) & keysPressed;

    if (repeatTimer > 0)
        repeatTimer--;
    if (repeatStartTimer > 0)
        repeatStartTimer--;
}

void system_doRumble(bool rumbleVal)
{
}

int system_getMotionSensorX() {
    return 0;
}
int system_getMotionSensorY() {
    return 0;
}

void system_enableCamera(int index)
{
}

void system_disableCamera(void)
{
}

void system_getCamera(u8* memory, const u8* camRegisters)
{
}

void system_checkPolls() {
    if (!aptMainLoop()) {
        system_cleanup();
        exit(0);
    }

    if (gfxConsumeAcceleratedGameFrame()) {
        // Citro3D presents the GPU-rendered game screen itself. Preserve the
        // software-rendered menu/debug screen with one independent swap.
        const gfxScreen_t consoleScreen =
            gameScreen == 0 ? GFX_BOTTOM : GFX_TOP;
        u8* consoleFramebuffer = gfxGetInactiveFramebuffer(
            consoleScreen, GFX_LEFT);
        GSPGPU_FlushDataCache(consoleFramebuffer,
            framebufferSizes[consoleScreen]);
        gfxMySwapBuffer(consoleScreen);
    }
    else {
        gfxFlushBuffers();
        gfxMySwapBuffers();
    }
    consoleCheckFramebuffers();
}

void system_waitForVBlank() {
    gspWaitForVBlank();
}

void system_cleanup() {
    nifiStop();
    mgr_save();
    mgr_exit();

    audioExit();
    gfxExitAcceleratedGameRenderer();
    gfxExit();
}
