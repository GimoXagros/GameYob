#ifdef DS
#include "common.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "menu.h"
#include "gbprinter.h"
#include "console.h"
#include "inputhelper.h"
#include "filechooser.h"
#include "soundengine.h"
#include "main.h"
#include "gameboy.h"
#include "nifi.h"
#include "cheats.h"
#include "gbgfx.h"
#include "gbs.h"
#include "gbmanager.h"
#include "config.h"
#include "localization.h"
#include "text.h"

const int MENU_DS   = 1;
const int MENU_3DS  = 2;
const int MENU_SDL  = 4;

const int MENU_ALL = MENU_DS | MENU_3DS | MENU_SDL;

#if defined(DS)
const int MENU_BITMASK = MENU_DS;
#elif defined(_3DS)
const int MENU_BITMASK = MENU_3DS;
#elif defined(SDL)
const int MENU_BITMASK = MENU_SDL;
#endif

void printVersionInfo(); // Defined in version.cpp

void subMenuGenericUpdateFunc(); // Private function here

void printSpaces(int count) {
    while (count-- > 0)
        printf(" ");
}


bool consoleDebugOutput = false;
bool menuOn = false;
bool consoleInitialized = false;
int menu=0;
int option = -1;
char printMessage[256];
int gameScreen=0;
int singleScreenMode=0;
int stateNum=0;

bool windowDisabled = false;
bool hblankDisabled = false;

PrintConsole* menuConsole;


int gbcModeOption = 0;
bool gbaModeOption = 0;
int sgbModeOption = 0;

bool soundDisabled = false;
bool hyperSound = false;

bool customBordersEnabled = false;
bool sgbBordersEnabled = false;
bool autoSavingEnabled = false;

bool printerEnabled = false;

// how/when the bios should be used
int biosEnabled = false;

void (*subMenuUpdateFunc)();

bool fpsOutput = false;
bool timeOutput = false;

int rumbleStrength = 0;

extern int interruptWaitMode;
extern int halt;

extern int rumbleInserted;


// Private function used for simple submenus
void subMenuGenericUpdateFunc() {
    if (keyJustPressed(mapMenuKey(MENU_KEY_A)) || keyJustPressed(mapMenuKey(MENU_KEY_B)))
        closeSubMenu();
}

// Functions corresponding to menu options

void suspendFunc(int value) {
    muteSND();
    if (!autoSavingEnabled && gameboy->getNumSramBanks()) {
        printMenuMessage("Saving SRAM...");
        mgr_save();
    }
    printMenuMessage("Saving state...");
    gameboy->saveState(-1);
    printMessage[0] = '\0';
    closeMenu();
    mgr_selectRom();
}
void exitFunc(int value) {
    muteSND();
    if (!autoSavingEnabled && gameboy->getNumSramBanks()) {
        printMenuMessage("Saving SRAM...");
        mgr_save();
    }
    printMessage[0] = '\0';
    closeMenu();
    mgr_selectRom();
}
void exitNoSaveFunc(int value) {
    muteSND();
    closeMenu();
    mgr_selectRom();
}
void consoleOutputFunc(int value) {
    if (value == 0) {
        fpsOutput = false;
        timeOutput = false;
        consoleDebugOutput = false;
    }
    else if (value == 1) {
        fpsOutput = false;
        timeOutput = true;
        consoleDebugOutput = false;
    }
    else if (value == 2) {
        fpsOutput = true;
        timeOutput = true;
        consoleDebugOutput = false;
    }
    else if (value == 3) {
        fpsOutput = false;
        timeOutput = false;
        consoleDebugOutput = true;
    }
}
void returnToLauncherFunc(int value) {
    system_cleanup();
    exit(0);
}

void printerEnableFunc(int value) {
    if (value) {
        initGbPrinter();
    }
    printerEnabled = value;
}

void cheatFunc(int value) {
    if (!startCheatMenu())
        printMenuMessage("No cheats found!");
}
void keyConfigFunc(int value) {
    startKeyConfigChooser();
}

void languageFunc(int value) {
    static const char* codes[] = {"en", "ja", "ko"};
    if (value >= 0 && value < 3) {
        languagePath[0] = '\0';
        languageLoadCode(codes[value]);
    }
    else if (value == 3 && languagePath[0]) {
        languageLoadFile(languagePath);
    }
}

void selectLanguageFileFunc(int value) {
    static FileChooserState languageChooserState = {0, "/"};
    const char* extensions[] = {"ini", "json", "xml", "yaml", "yml"};
    loadFileChooserState(&languageChooserState);
    char* filename = startFileChooser(extensions, 5, false, true);
    if (filename) {
        char cwd[MAX_FILENAME_LEN];
        fs_getcwd(cwd, sizeof(cwd));
        const bool needsSlash = cwd[0] && cwd[strlen(cwd) - 1] != '/';
        snprintf(languagePath, sizeof(languagePath), "%s%s%s", cwd,
                 needsSlash ? "/" : "", filename);
        free(filename);
        if (languageLoadFile(languagePath)) {
            setMenuOption("Language", 3);
            printMenuMessage("Language loaded.");
        }
        else {
            printMenuMessage("Invalid language file.");
        }
    }
    saveFileChooserState(&languageChooserState);
    loadFileChooserState(&romChooserState);
}

void saveSettingsFunc(int value) {
    printMenuMessage("Saving settings...");
    muteSND();
    writeConfigFile();
    const bool languageTemplateReady = createCustomLanguageTemplate();

    // Also save cheats
    if (gameboy != NULL) {
        char nameBuf[MAX_FILENAME_LEN];
        snprintf(nameBuf, sizeof(nameBuf), "%s.cht",
                 gameboy->getRomFile()->getBasename());
        gameboy->getCheatEngine()->saveCheats(nameBuf);
    }

    if (!mgr_isPaused())
        unmuteSND();
    printMenuMessage(languageTemplateReady ? "Settings saved." :
                     "Language template could not be created.");
}

void stateSelectFunc(int value) {
    stateNum = value;
    if (gameboy->checkStateExists(stateNum)) {
        enableMenuOption("Load State");
        enableMenuOption("Delete State");
    }
    else {
        disableMenuOption("Load State");
        disableMenuOption("Delete State");
    }
}
void stateSaveFunc(int value) {
    printMenuMessage("Saving state...");
    muteSND();
    gameboy->saveState(stateNum);
    if (!mgr_isPaused())
        unmuteSND();
    printMenuMessage("State saved.");
    // Will activate the other state options
    stateSelectFunc(stateNum);
}
void stateLoadFunc(int value) {
    printMenuMessage("Loading state...");
    muteSND();
    if (gameboy->loadState(stateNum) == 0) {
        closeMenu();
        updateScreens();
        printMessage[0] = '\0';
    }
}
void stateDeleteFunc(int value) {
    muteSND();
    gameboy->deleteState(stateNum);
    // Will grey out the other state options
    stateSelectFunc(stateNum);
    if (!mgr_isPaused())
        unmuteSND();
}
void resetFunc(int value) {
    closeMenu();
    updateScreens();
    gameboy->init();
}
void returnFunc(int value) {
    closeMenu();
    updateScreens();
}

void gameboyModeFunc(int value) {
    gbcModeOption = value;
}

void gbaModeFunc(int value) {
    gbaModeOption = value;
}

void sgbModeFunc(int value) {
    sgbModeOption = value;
}

void biosEnableFunc(int value) {
    biosEnabled = value;
}

void selectBiosFileFunc(int value) {
    (void)value;
    static FileChooserState biosChooserState = {0, "/"};
    static bool chooserInitialized = false;

    if (!chooserInitialized && biosPath[0]) {
        char directory[MAX_FILENAME_LEN];
        snprintf(directory, sizeof(directory), "%s", biosPath);
        char* slash = strrchr(directory, '/');
        if (slash) {
            setFileChooserMatchFile(slash + 1);
            if (slash == directory)
                slash[1] = '\0';
            else
                *slash = '\0';
            biosChooserState.directory = directory;
        }
        chooserInitialized = true;
    }

    loadFileChooserState(&biosChooserState);
    const char* extensions[] = {"bin"};
    char* filename = startFileChooser(extensions, 1, false, true);
    if (filename) {
        char cwd[MAX_FILENAME_LEN];
        char selectedPath[MAX_FILENAME_LEN];
        fs_getcwd(cwd, sizeof(cwd));
        const bool needsSlash = cwd[0] && cwd[strlen(cwd)-1] != '/';
        const int written = snprintf(selectedPath, sizeof(selectedPath),
            "%s%s%s", cwd, needsSlash ? "/" : "", filename);
        free(filename);

        RomFile* romFile = gameboy ? gameboy->getRomFile() : NULL;
        if (written >= 0 && written < (int)sizeof(selectedPath) &&
                romFile && romFile->loadBios(selectedPath)) {
            snprintf(biosPath, sizeof(biosPath), "%s", selectedPath);
            enableMenuOption("GBC Bios");
            printMenuMessage("GBC BIOS selected. Reset the game to apply it.");
        }
        else {
            printMenuMessage("Invalid GBC BIOS file.");
        }
    }

    saveFileChooserState(&biosChooserState);
    loadFileChooserState(&romChooserState);
}

void setScreenFunc(int value) {
    gameScreen = value;
    updateScreens();
}

void setSingleScreenFunc(int value) {
    if (value != singleScreenMode) {
        singleScreenMode = value;
        if (singleScreenMode)
            mgr_pause();

        if (isMenuOn()) {
            // Swap game screen
            // This will invoke updateScreens, incidentally.
            setMenuOption("Game Screen", !gameScreen);
        }
    }
}

void setScaleModeFunc(int value) {
    scaleMode = value;
    if (!isMenuOn()) {
        updateScreens();
    }
#ifdef _3DS
    // The native backend submits a completed, scaled frame. Rebuild the
    // surrounding framebuffer immediately so an old border or old-size game
    // image cannot remain visible outside the new destination rectangle.
    doAtVBlank(refreshScaleMode);
#endif
    if (value == 0) {
        doAtVBlank(checkBorder);
        enableMenuOption("Console Output");
    }
    else {
        disableMenuOption("Console Output");
    }
}
void setScaleFilterFunc(int value) {
    scaleFilter = value;
}

void customBorderEnableFunc(int value) {
    customBordersEnabled = value;
    checkBorder();
}

void sgbBorderEnableFunc(int value) {
    sgbBordersEnabled = value;
    checkBorder();
}

void vblankWaitFunc(int value) {
    interruptWaitMode = value;
}
void hblankEnableFunc(int value) {
    hblankDisabled = !value;
}
void windowEnableFunc(int value) {
    windowDisabled = !value;
#ifdef DS
    if (windowDisabled)
        REG_DISPCNT &= ~DISPLAY_WIN0_ON;
    else
        REG_DISPCNT |= DISPLAY_WIN0_ON;
#endif
}
void soundEnableFunc(int value) {
    soundDisabled = !value;
#ifdef DS
    sharedData->fifosSent = sharedData->fifosSent + 1;
    fifoSendValue32(FIFO_USER_01, GBSND_MUTE_COMMAND<<20);
#endif
}
void romInfoFunc(int value) {
    displaySubMenu(subMenuGenericUpdateFunc);
    gameboy->printRomInfo();
}
void versionInfoFunc(int value) {
    displaySubMenu(subMenuGenericUpdateFunc);
    printVersionInfo();
}

void setChanEnabled(int chan, int value) {
    if (value == 0)
        disableChannel(chan);
    else
        enableChannel(chan);
}
void chan1Func(int value) {
    setChanEnabled(0, value);
}
void chan2Func(int value) {
    setChanEnabled(1, value);
}
void chan3Func(int value) {
    setChanEnabled(2, value);
}
void chan4Func(int value) {
    setChanEnabled(3, value);
}

void setRumbleFunc(int value) {
    rumbleStrength = value;

    rumbleInserted = checkRumble();
}

void setCamera(int value) {
    if (value > 0) {
        system_enableCamera(value);
    } else {
        system_disableCamera();
    }
}

void hyperSoundFunc(int value) {
    hyperSound = value;
#ifdef DS
    sharedData->hyperSound = value;
    sharedData->fifosSent = sharedData->fifosSent + 1;
    fifoSendValue32(FIFO_USER_01, GBSND_HYPERSOUND_ENABLE_COMMAND<<20 | hyperSound);
#endif
}

void setAutoSaveFunc(int value) {
    muteSND();
    if (autoSavingEnabled)
        gameboy->gameboySyncAutosave();
    else
        gameboy->saveGame(); // Synchronizes save file with filesystem
    autoSavingEnabled = value;
    if (gameboy->isRomLoaded() && gameboy->getNumSramBanks() && !gbsMode && !autoSavingEnabled)
        enableMenuOption("Exit without saving");
    else
        disableMenuOption("Exit without saving");
    if (!mgr_isPaused())
        unmuteSND();
}

void localLinkFunc(int value) {
#ifdef NIFI
    // Local and wireless links are mutually exclusive. A completed wireless
    // session otherwise keeps routing input through NiFi and makes a
    // subsequently selected local link appear unresponsive.
    nifiStop();
#endif
    // Save slot 2 is dedicated to the second local Game Boy. Loading it once
    // avoids replacing a freshly allocated SRAM buffer and losing its handle.
    if (!mgr_startGb2(2)) {
        printMenuMessage("Link failed.");
        return;
    }
    mgr_setInternalClockGb(gameboy);
    printMenuMessage("Local link started.");
}
#ifdef _3DS
void audioInfoFunc(int value) {
    displaySubMenu(subMenuGenericUpdateFunc);
    printAudioInfo();
}
#endif

struct MenuOption {
    const char* name;
    void (*function)(int);
    int numValues;
    const char* values[10];
    int defaultSelection;
    int platforms;

    bool enabled;
    int selection;
};
struct SubMenu {
    const char *name;
    int numOptions;
    MenuOption options[10];

    int selection;
};


SubMenu menuList[] = {
    {
        "ROM",
        9,
        {
            {"Exit", exitFunc, 0, {}, 0, MENU_ALL},
            {"Reset", resetFunc, 0, {}, 0, MENU_ALL},
            {"State Slot", stateSelectFunc, 10, {"0","1","2","3","4","5","6","7","8","9"}, 0, MENU_ALL},
            {"Save State", stateSaveFunc, 0, {}, 0, MENU_ALL},
            {"Load State", stateLoadFunc, 0, {}, 0, MENU_ALL},
            {"Delete State", stateDeleteFunc, 0, {}, 0, MENU_ALL},
            {"Quit to Launcher", returnToLauncherFunc, 0, {}, 0, MENU_DS | MENU_3DS},
            {"Exit without saving", exitNoSaveFunc, 0, {}, 0, MENU_ALL},
            {"Suspend", suspendFunc, 0, {}, 0, MENU_ALL}
        }
    },
    {
        "Settings",
        10,
        {
            {"Button Mapping", keyConfigFunc, 0, {}, 0, MENU_ALL},
            {"Manage Cheats", cheatFunc, 0, {}, 0, MENU_ALL},
            {"Language", languageFunc, 4, {"English","日本語","한국어","Custom"}, 0, MENU_ALL},
            {"Select Language File", selectLanguageFileFunc, 0, {}, 0, MENU_ALL},
            {"Rumble Pak", setRumbleFunc, 4, {"Off","Low","Mid","High"}, 2, MENU_DS},
            {"GB Camera", setCamera, 3, {"Off", "Inner","Outer"}, 0, MENU_DS},
            {"Console Output", consoleOutputFunc, 4, {"Off","Time","FPS+Time","Debug"}, 0, MENU_ALL},
            {"GB Printer", printerEnableFunc, 2, {"Off","On"}, 1, MENU_ALL},
            {"Autosaving", setAutoSaveFunc, 2, {"Off","On"}, 1, MENU_DS},
            {"Save Settings", saveSettingsFunc, 0, {}, 0, MENU_ALL}
        }
    },
    {
        "Display",
        7,
        {
            {"Game Screen", setScreenFunc, 2, {"Top","Bottom"}, 0, MENU_ALL},
            {"Single Screen", setSingleScreenFunc, 2, {"Off","On"}, 0, MENU_ALL},
            {"Scaling", setScaleModeFunc, 3, {"Off","Aspect","Full"}, 0, MENU_DS},
            {"Scale Filter", setScaleFilterFunc, 2, {"Off","On"}, 1, MENU_DS},
            {"SGB Borders", sgbBorderEnableFunc, 2, {"Off","On"}, 1, MENU_ALL},
            {"Custom Border", customBorderEnableFunc, 2, {"Off","On"}, 1, MENU_ALL},
            {"Select Border", (void (*)(int))selectBorder, 0, {}, 0, MENU_ALL},
        }
    },
    {
        "GB Modes",
        5,
        {
            {"Select GBC BIOS", selectBiosFileFunc, 0, {}, 0, MENU_ALL},
            {"GBC Bios", biosEnableFunc, 3, {"Off","GB Only","On"}, 1, MENU_ALL},
            {"Detect GBA", gbaModeFunc, 2, {"Off","On"}, 0, MENU_ALL},
            {"GBC Mode", gameboyModeFunc, 3, {"Off","If Needed","On"}, 2, MENU_ALL},
            {"SGB Mode", sgbModeFunc, 3, {"Off","Prefer GBC","Prefer SGB"}, 1, MENU_ALL}
        }
    },
    {
        "Debug",
        8,
        {
            {"Wait for Vblank", vblankWaitFunc, 2, {"Off","On"}, 0, MENU_DS},
            {"Hblank", hblankEnableFunc, 2, {"Off","On"}, 1, MENU_DS},
            {"Window", windowEnableFunc, 2, {"Off","On"}, 1, MENU_DS},
            {"Sound", soundEnableFunc, 2, {"Off","On"}, 1, MENU_ALL},
            {"Sound Timing Fix", hyperSoundFunc, 2, {"Off","On"}, 1, MENU_DS},
#ifdef _3DS
            {"Audio Status", audioInfoFunc, 0, {}, 0, MENU_3DS},
#else
            {"Audio Status", NULL, 0, {}, 0, 0},
#endif
            {"ROM Info", romInfoFunc, 0, {}, 0, MENU_ALL},
            {"Version Info", versionInfoFunc, 0, {}, 0, MENU_ALL}
        }
    },
    {
        "Sound Channels",
        4,
        {
            {"Channel 1", chan1Func, 2, {"Off","On"}, 1, MENU_ALL},
            {"Channel 2", chan2Func, 2, {"Off","On"}, 1, MENU_ALL},
            {"Channel 3", chan3Func, 2, {"Off","On"}, 1, MENU_ALL},
            {"Channel 4", chan4Func, 2, {"Off","On"}, 1, MENU_ALL}
        }
    },
    {
        "Linking",
        3,
        {
#if defined(DS) || defined(_3DS)
            {"Wireless Link", (void (*)(int))nifiInterLinkMenu, 0, {}, 0, MENU_DS | MENU_3DS},
#else
            {"Stub", NULL, 0, {}, 0, 0},
#endif
            {"Local Link", localLinkFunc, 0, {}, 0, MENU_ALL},
            {"Swap Focus", (void (*)(int))mgr_swapFocus, 0, {}, 0, MENU_ALL},
        }
    }
};
const int numMenus = sizeof(menuList)/sizeof(SubMenu);

void setMenuDefaults() {
    for (int i=0; i<numMenus; i++) {
        menuList[i].selection = -1;
        for (int j=0; j<menuList[i].numOptions; j++) {
            menuList[i].options[j].selection = menuList[i].options[j].defaultSelection;
            menuList[i].options[j].enabled = true;
            if (menuList[i].options[j].numValues != 0 &&
                    menuList[i].options[j].platforms & MENU_BITMASK) {
                int selection = menuList[i].options[j].defaultSelection;
                menuList[i].options[j].function(selection);
            }
        }
    }

#ifdef DS
    menuConsole = (PrintConsole*)malloc(sizeof(PrintConsole));
    memcpy(menuConsole, getDefaultConsole(), sizeof(PrintConsole));
#endif
}

void displayMenu() {
    menuOn = true;
    if (checkRumble())
        enableMenuOption("Rumble Pak");
    else
        disableMenuOption("Rumble Pak");

    if (checkCamera())
        enableMenuOption("GB Camera");
    else
        disableMenuOption("GB Camera");

    updateScreens();
    doAtVBlank(redrawMenu);
}
void closeMenu() {
    menuOn = false;
    setPrintConsole(menuConsole);
    clearConsole();
    mgr_unpause();
}

bool isMenuOn() {
    return menuOn;
}

// Some helper functions
void menuCursorUp() {
    option--;
    if (option == -1)
        return;
    if (option < -1)
        option = menuList[menu].numOptions - 1;

    if (!(menuList[menu].options[option].platforms & MENU_BITMASK))
        menuCursorUp();
}
void menuCursorDown() {
    option++;
    if (option >= menuList[menu].numOptions)
        option = -1;
    else {
        if (!(menuList[menu].options[option].platforms & MENU_BITMASK))
            menuCursorDown();
    }
}

// Get the number of rows down the selected option is
// Necessary because of leaving out certain options in certain platforms
int menuGetOptionRow() {
    if (option == -1)
        return option;
    int row = 0;
    for (int i=0; i<option; i++) {
        if (menuList[menu].options[i].platforms & MENU_BITMASK)
            row++;
    }
    return row;
}
void menuSetOptionRow(int row) {
    if (row == -1) {
        option = -1;
        return;
    }
    row++;
    int lastValidRow = -1;
    for (int i=0; i<menuList[menu].numOptions; i++) {
        if (menuList[menu].options[i].platforms & MENU_BITMASK) {
            row--;
            lastValidRow = i;
        }
        if (row == 0) {
            option = i;
            return;
        }
    }
    // Too high
    option = lastValidRow;
}
// Get the number of VISIBLE rows for this platform
int menuGetNumRows() {
    int count = 0;
    for (int i=0; i<menuList[menu].numOptions; i++) {
        if (menuList[menu].options[i].platforms & MENU_BITMASK)
            count++;
    }
    return count;
}

void redrawMenu() {
    PrintConsole* oldConsole = getPrintConsole();
    setPrintConsole(menuConsole);
    clearConsole();

    int width = consoleGetWidth();
    int height = consoleGetHeight();

    // Top line: submenu
    int pos=0;
    const char* translatedMenuName = tr(menuList[menu].name);
    int menuNameColumns = textColumns(translatedMenuName);
    int nameStart = (width-menuNameColumns-2)/2;
    if (option == -1) {
        nameStart-=2;
        iprintfColored(CONSOLE_COLOR_LIGHT_GREEN, "<");
    }
    else
        printf("<");
    pos++;
    for (; pos<nameStart; pos++)
        printf(" ");
    if (option == -1) {
        iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, "* ");
        pos += 2;
    }
    {
        int color = (option == -1 ? CONSOLE_COLOR_LIGHT_YELLOW : CONSOLE_COLOR_WHITE);
        iprintfColored(color, "[%s]", translatedMenuName);
    }
    pos += 2 + menuNameColumns;
    if (option == -1) {
        iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, " *");
        pos += 2;
    }
    for (; pos < width-1; pos++)
        printf(" ");
    if (option == -1)
        iprintfColored(CONSOLE_COLOR_LIGHT_GREEN, ">");
    else
        printf(">");
    printf("\n");

    // Rest of the lines: options
    for (int i=0; i<menuList[menu].numOptions; i++) {
        if (!(menuList[menu].options[i].platforms & MENU_BITMASK))
            continue;

        const char* translatedOption = tr(menuList[menu].options[i].name);
        const int optionColumns = textColumns(translatedOption);

        int option_color;
        if (!menuList[menu].options[i].enabled)
            option_color = CONSOLE_COLOR_GREY;
        else if (option == i)
            option_color = CONSOLE_COLOR_LIGHT_YELLOW;
        else
            option_color = CONSOLE_COLOR_WHITE;

        if (menuList[menu].options[i].numValues == 0) {
            printSpaces((width-optionColumns)/2-2);
            if (i == option) {
                iprintfColored(option_color, "* %s *\n\n", translatedOption);
            }
            else
                iprintfColored(option_color, "  %s  \n\n", translatedOption);
        }
        else {
            printSpaces(width/2-optionColumns);
            const char* translatedValue = tr(menuList[menu].options[i].values[
                menuList[menu].options[i].selection]);
            if (i == option) {
                iprintfColored(option_color, "* ");
                iprintfColored(option_color, "%s  ", translatedOption);
                iprintfColored(menuList[menu].options[i].enabled ? CONSOLE_COLOR_LIGHT_GREEN : option_color,
                        "%s", translatedValue);
                iprintfColored(option_color, " *");
            }
            else {
                printf("  ");
                iprintfColored(option_color, "%s  ", translatedOption);
                iprintfColored(option_color, "%s", translatedValue);
            }
            printf("\n\n");
        }
    }

    // Message at the bottom
    if (printMessage[0] != '\0') {
        int rows = menuGetNumRows();
        int newlines = height-1-(rows*2+2)-1;
        for (int i=0; i<newlines; i++)
            printf("\n");
        printSpaces(width-1-textColumns(printMessage));
        iprintfColored(CONSOLE_COLOR_WHITE, "%s\n", printMessage);

        printMessage[0] = '\0';
    }

    setPrintConsole(oldConsole);
}

// Called each vblank while the menu is on
void updateMenu() {
    if (!isMenuOn())
        return;

    if (subMenuUpdateFunc != 0) {
        subMenuUpdateFunc();
        return;
    }

    bool redraw = false;
    // Get input
    if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_UP))) {
        menuCursorUp();
        redraw = true;
    }
    else if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_DOWN))) {
        menuCursorDown();
        redraw = true;
    }
    else if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_LEFT))) {
        if (option == -1) {
            menu--;
            if (menu < 0)
                menu = numMenus-1;
        }
        else if (menuList[menu].options[option].numValues != 0 && menuList[menu].options[option].enabled) {
            int selection = menuList[menu].options[option].selection-1;
            if (selection < 0)
                selection = menuList[menu].options[option].numValues-1;
            menuList[menu].options[option].selection = selection;
            menuList[menu].options[option].function(selection);
        }
        redraw = true;
    }
    else if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_RIGHT))) {
        if (option == -1) {
            menu++;
            if (menu >= numMenus)
                menu = 0;
        }
        else if (menuList[menu].options[option].numValues != 0 && menuList[menu].options[option].enabled) {
            int selection = menuList[menu].options[option].selection+1;
            if (selection >= menuList[menu].options[option].numValues)
                selection = 0;
            menuList[menu].options[option].selection = selection;
            menuList[menu].options[option].function(selection);
        }
        redraw = true;
    }
    else if (keyJustPressed(mapMenuKey(MENU_KEY_A))) {
        forceReleaseKey(mapMenuKey(MENU_KEY_A));
        if (option >= 0 && menuList[menu].options[option].numValues == 0 && menuList[menu].options[option].enabled) {
            menuList[menu].options[option].function(menuList[menu].options[option].selection);
        }
        redraw = true;
    }
    else if (keyJustPressed(mapMenuKey(MENU_KEY_B))) {
        forceReleaseKey(mapMenuKey(MENU_KEY_B));
        closeMenu();
        updateScreens();
    }
    else if (keyJustPressed(mapMenuKey(MENU_KEY_L))) {
        int row = menuGetOptionRow();
        menu--;
        if (menu < 0)
            menu = numMenus-1;
        menuSetOptionRow(row);
        redraw = true;
    }
    else if (keyJustPressed(mapMenuKey(MENU_KEY_R))) {
        int row = menuGetOptionRow();
        menu++;
        if (menu >= numMenus)
            menu = 0;
        menuSetOptionRow(row);
        redraw = true;
    }
    if (redraw && subMenuUpdateFunc == 0 &&
            isMenuOn()) // The menu may have been closed by an option
        doAtVBlank(redrawMenu);
}

// Message will be printed immediately, but also stored in case it's overwritten 
// right away.
void printMenuMessage(const char* s) {
    int width = consoleGetWidth();
    int height = consoleGetHeight();
    int rows = menuGetNumRows();

    bool hadPreviousMessage = printMessage[0] != '\0';
    textCopyColumns(printMessage, sizeof(printMessage), tr(s), width-1);

    if (hadPreviousMessage) {
        printf("\r");
    }
    else {
        int newlines = height-1-(rows*2+2)-1;
        for (int i=0; i<newlines; i++)
            printf("\n");
    }
    printSpaces(width-1-textColumns(printMessage));
    iprintfColored(CONSOLE_COLOR_WHITE, "%s", printMessage);

    consoleFlush();
}

void displaySubMenu(void (*updateFunc)()) {
    subMenuUpdateFunc = updateFunc;
}
void closeSubMenu() {
    subMenuUpdateFunc = NULL;
    doAtVBlank(redrawMenu);
}


int getMenuOption(const char* optionName) {
    for (int i=0; i<numMenus; i++) {
        for (int j=0; j<menuList[i].numOptions; j++) {
            if (strcasecmp(optionName, menuList[i].options[j].name) == 0) {
                return menuList[i].options[j].selection;
            }
        }
    }
    return 0;
}
void setMenuOption(const char* optionName, int value) {
    for (int i=0; i<numMenus; i++) {
        for (int j=0; j<menuList[i].numOptions; j++) {
            if (strcasecmp(optionName, menuList[i].options[j].name) == 0) {
                if (!(menuList[i].options[j].platforms & MENU_BITMASK))
                    continue;
                menuList[i].options[j].selection = value;
                menuList[i].options[j].function(value);
                return;
            }
        }
    }
}
void enableMenuOption(const char* optionName) {
    for (int i=0; i<numMenus; i++) {
        for (int j=0; j<menuList[i].numOptions; j++) {
            if (strcasecmp(optionName, menuList[i].options[j].name) == 0) {
                menuList[i].options[j].enabled = true;
                return;
            }
        }
    }
}
void disableMenuOption(const char* optionName) {
    for (int i=0; i<numMenus; i++) {
        for (int j=0; j<menuList[i].numOptions; j++) {
            if (strcasecmp(optionName, menuList[i].options[j].name) == 0) {
                menuList[i].options[j].enabled = false;
                return;
            }
        }
    }
}

void menuParseConfig(char* line) {
    char* equalsPos = (char*)strchr(line, '=');
    if (equalsPos == 0)
        return;
    *equalsPos = '\0';
    const char* option = line;
    const char* value = equalsPos+1;
    int val = atoi(value);
    setMenuOption(option, val);
}

void menuPrintConfig(FileHandle* file) {
    for (int i=0; i<numMenus; i++) {
        for (int j=0; j<menuList[i].numOptions; j++) {
            if (menuList[i].options[j].platforms & MENU_BITMASK &&
                    menuList[i].options[j].numValues != 0)
                file_printf(file, "%s=%d\n", menuList[i].options[j].name, menuList[i].options[j].selection);
        }
    }
}

