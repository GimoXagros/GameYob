#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include "gbmanager.h"
#include "inputhelper.h"
#include "gameboy.h"
#include "cheats.h"
#include "nifi.h"
#include "gbs.h"
#include "menu.h"
#include "romfile.h"
#include "filechooser.h"
#include "soundengine.h"
#include "error.h"
#include "timer.h"
#include "io.h"

Gameboy* gameboy = NULL;
Gameboy* gb2 = NULL;

// Ordering for purposes of link emulation
Gameboy* gbUno = NULL;
Gameboy* gbDuo = NULL;

Gameboy* hostGb = NULL;

int mgr_frameCounter;

int autoFireCounterA=0, autoFireCounterB=0;


int fps = 0;

time_t rawTime;
time_t lastRawTime;

bool emulationPaused;

void mgr_init() {
    if (gameboy != NULL)
        delete gameboy;
    if (gb2 != NULL)
        delete gb2;

    gameboy = new Gameboy();
    hostGb = gameboy;
    gbUno = gameboy;
    gbDuo = NULL;

    mgr_frameCounter = 0;

    rawTime = 0;
    lastRawTime = rawTime;

    emulationPaused = false;

#ifdef CPU_DEBUG
    startDebugger();
#endif
}

void mgr_reset() {
    if (gameboy)
        gameboy->init();
    if (gb2)
        gb2->init();
    mgr_frameCounter = 0;
}

void mgr_runFrame() {
    if (!gbUno || emulationPaused)
        return;

    int ret1=0;

    if (gbDuo) {
//         printLog("Begin\n");
        while (!((ret1 & RET_VBLANK))) {
            bool swap = false;
            if ((gbUno->ioRam[0x02]&0x81) == 0x80)
                swap = true;

            if (swap) {
                Gameboy* tmp = gbUno;
                gbUno = gbDuo;
                gbDuo = tmp;
            }

            if (gbUno->cycleCount <= gbDuo->cycleCount) {
                if (gbUno == hostGb)
                    ret1 |= gbUno->runEmul();
                else
                    gbUno->runEmul();
            }
            else {
                if (gbDuo == hostGb)
                    ret1 |= gbDuo->runEmul();
                else
                    gbDuo->runEmul();
            }
//             printLog("%d, %d\n", gbUno->cycleCount, gbDuo->cycleCount);
        }
//         printLog("End\n");

        int subtractor;
        if (gbDuo->cycleCount <= gbUno->cycleCount)
            subtractor = gbDuo->cycleCount;
        else
            subtractor = gbUno->cycleCount;
        gbUno->cycleCount -= subtractor;
        gbDuo->cycleCount -= subtractor;
        if (gbUno->cycleToSerialTransfer != -1)
            gbUno->cycleToSerialTransfer -= subtractor;
        if (gbDuo->cycleToSerialTransfer != -1)
            gbDuo->cycleToSerialTransfer -= subtractor;
    }
    else {
        while (!(ret1 & RET_VBLANK)) {
            ret1 |= gbUno->runEmul();
        }

        gbUno->cycleCount = 0;
    }

    if (mgr_areBothUsingExternalClock())
        printLog("Both waiting\n");

    mgr_frameCounter++;
}

bool mgr_startGb2(int saveId) {
    if (!gameboy || !gameboy->getRomFile())
        return false;
    if (gb2 == NULL) {
        gb2 = new (std::nothrow) Gameboy();
        if (!gb2)
            return false;
    }

    // A previous wireless or local-link session may have left the secondary
    // instance attached to another ROM/save file. Detach it before reusing the
    // object so the new local session cannot inherit stale link state.
    RomFile* oldRomFile = gb2->getRomFile();
    if (oldRomFile != NULL) {
        gb2->unloadRom();
        if (oldRomFile != gameboy->getRomFile())
            delete oldRomFile;
    }
    gb2->setRomFile(gameboy->getRomFile());
    if (gb2->loadSave(saveId) != 0) {
        gb2->unloadRom();
        return false;
    }
    gb2->init();
    gb2->cycleCount = gameboy->cycleCount;
    gb2->getSoundEngine()->mute();

    gameboy->linkedGameboy = gb2;
    gb2->linkedGameboy = gameboy;

    gbDuo = gb2;
    return true;
}

void mgr_swapFocus() {
    if (gb2) {
        Gameboy* tmp = gameboy;
        gameboy = gb2;
        gb2 = tmp;

        gb2->getSoundEngine()->mute();
        gameboy->getSoundEngine()->refresh();

        refreshGFX();
    }
}

void mgr_setInternalClockGb(Gameboy* g) {
    gbUno = g;
    hostGb = g;
    if (g == gameboy)
        gbDuo = gb2;
    else
        gbDuo = gameboy;
}

bool mgr_isInternalClockGb(Gameboy* g) {
    return gbDuo && gbUno == g;
}
bool mgr_isExternalClockGb(Gameboy* g) {
    return gbUno && gbDuo == g;
}
bool mgr_areBothUsingExternalClock() {
    return (gbUno->ioRam[0x02] & 0x81) == 0x80 && gbDuo &&
        (gbDuo->ioRam[0x02] & 0x81) == 0x80;
}

void mgr_pause() {
    emulationPaused = true;
}
void mgr_unpause() {
    emulationPaused = false;
}
bool mgr_isPaused() {
    return emulationPaused;
}

void mgr_loadRom(const char* filename) {
    mgr_unloadRom();

    // A previous SGB probe may have been interrupted by choosing another ROM.
    // Never let that transient boot state leak into the next cartridge.
    probingForBorder = false;

#ifdef NIFI
    nifiStop();
#endif

    // A cartridge-owned SGB border must never survive into the next ROM.
    // Reset the platform renderer before opening the new cartridge so even a
    // slow SD read cannot leave the previous border visible on screen.
    resetSgbBorder();

    RomFile* romFile = new RomFile(filename);
    if (romFile == 0)
        fatalerr("Not enough RAM to load rom");
    gameboy->setRomFile(romFile);
    if (gameboy->loadSave(1) != 0)
        printLog("Unable to open the ROM save file; continuing without persistence.\n");

    hostGb = gameboy;

    if (sgbBordersEnabled)
        // Enhanced dual-mode cartridges are briefly booted as SGB so their
        // border can be captured, then reset into the user's preferred mode.
        probingForBorder = true;

    gameboy->init();

    char cheatFilename[MAX_FILENAME_LEN];
    if (gbsMode) {
        gameboy->getCheatEngine()->loadCheats("");
    }
    else {
        const int written = snprintf(cheatFilename, sizeof(cheatFilename),
            "%s.cht", romFile->getBasename());
        if (written >= 0 && written < (int)sizeof(cheatFilename))
            gameboy->getCheatEngine()->loadCheats(cheatFilename);
        if (gameboy->getCheatEngine()->getNumCheats() == 0 &&
                strcmp(romFile->getBasename(),
                       romFile->getStorageBasename()) != 0) {
            const int fallbackWritten = snprintf(cheatFilename,
                sizeof(cheatFilename), "%s.cht",
                romFile->getStorageBasename());
            if (fallbackWritten >= 0 &&
                    fallbackWritten < (int)sizeof(cheatFilename))
                gameboy->getCheatEngine()->loadCheats(cheatFilename);
        }
    }

    if (gbsMode) {
        disableMenuOption("State Slot");
        disableMenuOption("Save State");
        disableMenuOption("Load State");
        disableMenuOption("Delete State");
        disableMenuOption("Suspend");
        disableMenuOption("Exit without saving");
    }
    else {
        enableMenuOption("State Slot");
        enableMenuOption("Save State");
        enableMenuOption("Suspend");
        if (gameboy->checkStateExists(stateNum)) {
            enableMenuOption("Load State");
            enableMenuOption("Delete State");
        }
        else {
            disableMenuOption("Load State");
            disableMenuOption("Delete State");
        }

        if (gameboy->getNumSramBanks() && !autoSavingEnabled)
            enableMenuOption("Exit without saving");
        else
            disableMenuOption("Exit without saving");
    }

    if (biosExists)
        enableMenuOption("GBC Bios");
    else
        disableMenuOption("GBC Bios");
}

void mgr_unloadRom() {
#ifdef CPU_DEBUG
    stopDebugger();
#endif

    RomFile* mainRom = gameboy ? gameboy->getRomFile() : NULL;
    if (gb2) {
        RomFile* secondaryRom = gb2->getRomFile();
        gb2->unloadRom();
        if (secondaryRom != NULL && secondaryRom != mainRom)
            delete secondaryRom;
        delete gb2;
    }
    if (gameboy && mainRom != NULL) {
        gameboy->unloadRom();
        delete mainRom;
    }

    if (gameboy)
        gameboy->linkedGameboy = NULL;
    gbUno = gameboy;
    gb2 = NULL;
    gbDuo = NULL;
}

void mgr_selectRom() {
    mgr_unloadRom();

    loadFileChooserState(&romChooserState);
    const char* extraExtensions[] = {"gbs"};
    char* filename = startFileChooser(extraExtensions, 1, true);
    saveFileChooserState(&romChooserState);

    if (filename == NULL) {
        fatalerr("Filechooser error");
    }

    // Keep the v0.5.4 cartridge launch path: the chooser leaves its selected
    // directory active and the exact d_name is opened relative to that cwd.
    mgr_loadRom(filename);
    free(filename);

    // These things shouldn't be here?
    if (!biosExists) {
        gameboy->getRomFile()->loadBios("gbc_bios.bin");
    }
    updateScreens();
    if (!mgr_isPaused())
        unmuteSND();
}

void mgr_save() {
    if (gameboy)
        gameboy->saveGame();
    if (gb2)
        gb2->saveGame();
}

void mgr_updateVBlank() {
    drawScreen();

    system_checkPolls();

    inputUpdateVBlank();

    buttonsPressed = 0xff;
    if (isMenuOn())
        updateMenu();
    else {
        // Check some buttons
        buttonsPressed = 0xff;

        if (probingForBorder)
            return;

        if (keyPressed(mapFuncKey(FUNC_KEY_UP))) {
            buttonsPressed &= (0xFF ^ GB_UP);
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_DOWN))) {
            buttonsPressed &= (0xFF ^ GB_DOWN);
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_LEFT))) {
            buttonsPressed &= (0xFF ^ GB_LEFT);
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_RIGHT))) {
            buttonsPressed &= (0xFF ^ GB_RIGHT);
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_A))) {
            buttonsPressed &= (0xFF ^ GB_A);
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_B))) {
            buttonsPressed &= (0xFF ^ GB_B);
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_START))) {
            buttonsPressed &= (0xFF ^ GB_START);
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_SELECT))) {
            buttonsPressed &= (0xFF ^ GB_SELECT);
        }

        if (keyPressed(mapFuncKey(FUNC_KEY_AUTO_A))) {
            if (autoFireCounterA <= 0) {
                buttonsPressed &= (0xFF ^ GB_A);
                autoFireCounterA = 2;
            }
            autoFireCounterA--;
        }
        if (keyPressed(mapFuncKey(FUNC_KEY_AUTO_B))) {
            if (autoFireCounterB <= 0) {
                buttonsPressed &= (0xFF ^ GB_B);
                autoFireCounterB = 2;
            }
            autoFireCounterB--;
        }

#ifndef NIFI
        gameboy->controllers[0] = buttonsPressed;
#endif

        if (keyJustPressed(mapFuncKey(FUNC_KEY_SAVE))) {
            if (!autoSavingEnabled) {
                gameboy->saveGame();
            }
        }

        fastForwardKey = keyPressed(mapFuncKey(FUNC_KEY_FAST_FORWARD));
        if (keyJustPressed(mapFuncKey(FUNC_KEY_FAST_FORWARD_TOGGLE)))
            fastForwardMode = !fastForwardMode;

        if (keyJustPressed(mapFuncKey(FUNC_KEY_MENU) | mapFuncKey(FUNC_KEY_MENU_PAUSE)
#if defined(DS) || defined(_3DS)
                    | KEY_TOUCH
#endif
                    )) {
            if (singleScreenMode || keyJustPressed(mapFuncKey(FUNC_KEY_MENU_PAUSE)))
                mgr_pause();

            forceReleaseKey(0xffffffff);
            fastForwardKey = false;
            fastForwardMode = false;
            displayMenu();
        }

        if (keyJustPressed(mapFuncKey(FUNC_KEY_SCALE))) {
            setMenuOption("Scaling", !getMenuOption("Scaling"));
        }

#ifdef DS
        if (fastForwardKey || fastForwardMode) {
            sharedData->hyperSound = false;
        }
        else {
            sharedData->hyperSound = hyperSound;
        }
#endif
        if (keyJustPressed(mapFuncKey(FUNC_KEY_RESET)))
            gameboy->resetGameboy();

        if (gb2 && keyJustPressed(mapFuncKey(FUNC_KEY_SWAPFOCUS)))
            mgr_swapFocus();

        gameboy->checkInput();
        if (gb2)
            gb2->checkInput();

        if (gbsMode)
            gbsCheckInput();
    } // !isMenuOn()

    if (gameboy) {
#ifdef NIFI
        nifiUpdateInput();
#endif
    }

#ifndef DS
    rawTime = getTime();
#endif

#ifndef SDL
    fps++;

    if (isConsoleOn() && !isMenuOn() && !consoleDebugOutput && (rawTime > lastRawTime))
    {
        setPrintConsole(menuConsole);
        int line=0;
        if (fpsOutput) {
            clearConsole();
            printf("FPS: %d\n", fps);
            line++;
        }
        fps = 0;
#ifdef DS
        if (timeOutput) {
            for (; line<23-1; line++)
                printf("\n");
            char *timeString = ctime(&rawTime);
            for (int i=0;; i++) {
                if (timeString[i] == ':') {
                    timeString += i-2;
                    break;
                }
            }
            char s[50];
            strncpy(s, timeString, 50);
            s[5] = '\0';
            int spaces = 31-strlen(s);
            for (int i=0; i<spaces; i++)
                printf(" ");

            printf("%s\n", s);
        }
#endif
        lastRawTime = rawTime;
    }
#endif
}

void mgr_exit() {
    mgr_unloadRom();
    if (gameboy)
        delete gameboy;
    if (gb2)
        delete gb2;

    gameboy = NULL;
    gb2 = NULL;
}
