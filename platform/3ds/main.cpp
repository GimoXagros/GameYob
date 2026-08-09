#include <3ds.h>
#include <stdio.h>
#include <cwchar>
#include "gbgfx.h"
#include "soundengine.h"
#include "inputhelper.h"
#include "gameboy.h"
#include "console.h"
#include "gbs.h"
#include "romfile.h"
#include "menu.h"
#include "gbmanager.h"
#include "printconsole.h"
#include "3dsgfx.h"

void audioInit();

int main(int argc, char* argv[])
{
    // libctru's default application bootstrap already initializes srv, APT,
    // HID, FS, and mounts the SD card. Initializing those services a second
    // time causes mismatched reference counts during shutdown on current
    // toolchains.
	gfxInitDefault();
    gfxInitFramebufferTracking();

    consoleInitBottom();

    audioInit();
    fs_init();
    mgr_init();

	initInput();
    setMenuDefaults();
    readConfigFile();

    printf("GameYob 3DS\n\n");

    mgr_selectRom();

    for (;;) {
        mgr_runFrame();
        mgr_updateVBlank();
    }

	return 0;
}
