#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include "io.h"
#include "config.h"
#include "inputhelper.h"
#include "gbgfx.h"
#include "menu.h"
#include "console.h"
#include "filechooser.h"
#include "gameboy.h"
#include "cheats.h"
#include "romfile.h"
#include "localization.h"

#ifdef SDL
#define INI_PATH "gameyobds.ini"
#define LANGUAGE_TEMPLATE_PATH "gameyob_language.ini"
#else
#define INI_PATH "/gameyobds.ini"
#define LANGUAGE_TEMPLATE_PATH "/gameyob_language.ini"
#endif


char biosPath[MAX_FILENAME_LEN] = "";
char borderPath[MAX_FILENAME_LEN] = "";
char romPath[MAX_FILENAME_LEN] = "";
static char autoloadRomPath[MAX_FILENAME_LEN] = "";
bool borderPathExists = true;
char languagePath[MAX_FILENAME_LEN] = "";
static char configuredLanguage[16] = "en";

static void copyConfigText(char* destination, size_t capacity,
                           const char* source) {
    if (!destination || !capacity)
        return;
    if (!source) {
        destination[0] = '\0';
        return;
    }
    size_t length = strlen(source);
    if (length >= capacity)
        length = capacity - 1;
    memcpy(destination, source, length);
    destination[length] = '\0';
}


void controlsParseConfig(char* line);
void controlsPrintConfig(FileHandle* f);
void controlsCheckConfig();



void generalParseConfig(char* line) {
    char* equalsPos;
    if ((equalsPos = strrchr(line, '=')) != 0 && equalsPos != line+strlen(line)-1) {
        *equalsPos = '\0';
        const char* parameter = line;
        const char* value = equalsPos+1;

        if (strcasecmp(parameter, "rompath") == 0) {
            copyConfigText(romPath, sizeof(romPath), value);
            romChooserState.directory = romPath;
        }
        else if (strcasecmp(parameter, "autoloadrom") == 0) {
            copyConfigText(autoloadRomPath, sizeof(autoloadRomPath), value);
        }
        else if (strcasecmp(parameter, "biosfile") == 0) {
            copyConfigText(biosPath, sizeof(biosPath), value);
        }
        else if (strcasecmp(parameter, "borderfile") == 0) {
            copyConfigText(borderPath, sizeof(borderPath), value);
            borderPathExists = true;
        }
        else if (strcasecmp(parameter, "language") == 0) {
            copyConfigText(configuredLanguage, sizeof(configuredLanguage),
                           value);
        }
        else if (strcasecmp(parameter, "languagefile") == 0) {
            copyConfigText(languagePath, sizeof(languagePath), value);
        }
    }
    if (*borderPath == '\0') {
        strcpy(borderPath, "/border.bmp");
    }
}

void generalPrintConfig(FileHandle* file) {
        file_printf(file, "rompath=%s\n", romPath);
        if (*autoloadRomPath)
            file_printf(file, "autoloadrom=%s\n", autoloadRomPath);
        file_printf(file, "biosfile=%s\n", biosPath);
        file_printf(file, "borderfile=%s\n", borderPath);
        file_printf(file, "language=%s\n", languageGetCode());
        if (*languagePath)
            file_printf(file, "languagefile=%s\n", languagePath);
}

bool readConfigFile() {
    FileHandle* file = file_open(INI_PATH, "r");
    char line[512];
    void (*configParser)(char*) = generalParseConfig;

    if (file == NULL) {
        goto end;
    }

    while (file_tell(file) < file_getSize(file)) {
        file_gets(line, sizeof(line), file);
        size_t lineLength = strlen(line);
        while (lineLength && (line[lineLength - 1] == ' ' ||
                line[lineLength - 1] == '\n' || line[lineLength - 1] == '\r'))
            line[--lineLength] = '\0';
        if (line[0] == '[') {
            char* endBrace;
            if ((endBrace = strrchr(line, ']')) != 0) {
                *endBrace = '\0';
                const char* section = line+1;
                if (strcasecmp(section, "general") == 0) {
                    configParser = generalParseConfig;
                }
                else if (strcasecmp(section, "console") == 0) {
                    configParser = menuParseConfig;
                }
                else if (strcasecmp(section, "controls") == 0) {
                    configParser = controlsParseConfig;
                }
            }
        }
        else
            configParser(line);
    }
    file_close(file);
end:
    if (*languagePath && languageLoadFile(languagePath))
        setMenuOption("Language", 3);
    else if (strcasecmp(configuredLanguage, "ja") == 0)
        setMenuOption("Language", 1);
    else if (strcasecmp(configuredLanguage, "ko") == 0)
        setMenuOption("Language", 2);
    else
        setMenuOption("Language", 0);
    controlsCheckConfig();

    if (file == NULL)
        writeConfigFile();
    return file != NULL;
}

void writeConfigFile() {
    FileHandle* file = file_open(INI_PATH, "w");
    if (file == NULL) {
        printMenuMessage("Error opening gameyob.ini.");
        return;
    }

    file_printf(file, "[general]\n");
    generalPrintConfig(file);
    file_printf(file, "[console]\n");
    menuPrintConfig(file);
    file_printf(file, "[controls]\n");
    controlsPrintConfig(file);
    file_close(file);
}

const char* getAutoloadRomPath() {
    return autoloadRomPath;
}

bool createCustomLanguageTemplate() {
    return languageCreateEnglishTemplate(LANGUAGE_TEMPLATE_PATH);
}


// Keys: DS and 3DS specific code

#if defined(DS) || defined(_3DS)

#if defined(DS)
#include <nds.h>
#elif defined(_3DS)
#include <3ds.h>
#endif

const char* gbKeyNames[] = {"-","A","B","Left","Right","Up","Down","Start","Select",
    "Menu","Menu/Pause","Save","Autofire A","Autofire B", "Fast Forward", "FF Toggle", "Scale","Reset","Swap Focus"};
const char* dsKeyNames[] = {"A","B","Select","Start","Right","Left","Up","Down",
    "R","L","X","Y","Touch","","ZL","ZR","","","","","","","","","C-Right","C-Left","C-Up","C-Down","Pad-Right","Pad-Left","Pad-Up","Pad-Down"};

int keyMapping[NUM_FUNC_KEYS];
#if defined(DS)
#define NUM_BINDABLE_BUTTONS 13
struct KeyConfig {
    char name[32];
    int funcKeys[NUM_BINDABLE_BUTTONS];
};
#elif defined(_3DS)
#define NUM_BINDABLE_BUTTONS 32
struct KeyConfig {
    char name[32];
    int funcKeys[NUM_BINDABLE_BUTTONS];
};
#endif

#if defined(DS)
KeyConfig defaultKeyConfig = {
    "Main",
    {FUNC_KEY_A,FUNC_KEY_B,FUNC_KEY_SELECT,FUNC_KEY_START,FUNC_KEY_RIGHT,FUNC_KEY_LEFT,FUNC_KEY_UP,FUNC_KEY_DOWN,
        FUNC_KEY_MENU,FUNC_KEY_FAST_FORWARD,FUNC_KEY_START,FUNC_KEY_SELECT,FUNC_KEY_MENU}
};
#elif defined(_3DS)
KeyConfig defaultKeyConfig = {
    "Main",
    {FUNC_KEY_A,FUNC_KEY_B,FUNC_KEY_SELECT,FUNC_KEY_START,FUNC_KEY_RIGHT,FUNC_KEY_LEFT,FUNC_KEY_UP,FUNC_KEY_DOWN,
        FUNC_KEY_MENU,FUNC_KEY_FAST_FORWARD,FUNC_KEY_START,FUNC_KEY_SELECT,
        FUNC_KEY_NONE,FUNC_KEY_NONE,
        FUNC_KEY_NONE,FUNC_KEY_NONE,FUNC_KEY_NONE,FUNC_KEY_NONE,FUNC_KEY_NONE,FUNC_KEY_NONE,FUNC_KEY_NONE,FUNC_KEY_NONE,
        FUNC_KEY_RIGHT,FUNC_KEY_LEFT,FUNC_KEY_UP,FUNC_KEY_DOWN,FUNC_KEY_RIGHT,FUNC_KEY_LEFT,FUNC_KEY_UP,FUNC_KEY_DOWN}
};
#endif


std::vector<KeyConfig> keyConfigs;
unsigned int selectedKeyConfig=0;

#if defined(DS)
static bool hasMenuBinding(const KeyConfig& config) {
    for (int i=0; i<NUM_BINDABLE_BUTTONS; i++) {
        if (config.funcKeys[i] == FUNC_KEY_MENU ||
                config.funcKeys[i] == FUNC_KEY_MENU_PAUSE)
            return true;
    }
    return false;
}
#endif

void loadKeyConfig() {
    KeyConfig* keyConfig = &keyConfigs[selectedKeyConfig];
#if defined(DS)
    // Keep a recovery path for old or hand-edited configurations while still
    // allowing Touch to be assigned like every other DS input.
    if (!hasMenuBinding(*keyConfig))
        keyConfig->funcKeys[12] = FUNC_KEY_MENU;
#endif
    for (int i=0; i<NUM_FUNC_KEYS; i++)
        keyMapping[i] = 0;
    for (int i=0; i<NUM_BINDABLE_BUTTONS; i++) {
        const int function = keyConfig->funcKeys[i];
        if (function >= 0 && function < NUM_FUNC_KEYS)
            keyMapping[function] |= BIT(i);
    }
}

void controlsParseConfig(char* line2) {
    char line[100];
    copyConfigText(line, sizeof(line), line2);
    while (strlen(line) > 0 && (line[strlen(line)-1] == '\n' || line[strlen(line)-1] == ' '))
        line[strlen(line)-1] = '\0';
    if (line[0] == '(') {
        char* bracketEnd;
        if ((bracketEnd = strrchr(line, ')')) != 0) {
            *bracketEnd = '\0';
            const char* name = line+1;

            keyConfigs.push_back(KeyConfig());
            KeyConfig* config = &keyConfigs.back();
            copyConfigText(config->name, sizeof(config->name), name);
            for (int i=0; i<NUM_BINDABLE_BUTTONS; i++)
                config->funcKeys[i] = FUNC_KEY_NONE;
        }
        return;
    }
    char* equalsPos;
    if ((equalsPos = strrchr(line, '=')) != 0 && equalsPos != line+strlen(line)-1) {
        *equalsPos = '\0';

        if (strcasecmp(line, "config") == 0) {
            selectedKeyConfig = atoi(equalsPos+1);
        }
        else {
            int dsKey = -1;
            for (int i=0; i<NUM_BINDABLE_BUTTONS; i++) {
                if (strcasecmp(line, dsKeyNames[i]) == 0) {
                    dsKey = i;
                    break;
                }
            }
            int gbKey = -1;
            for (int i=0; i<NUM_FUNC_KEYS; i++) {
                if (strcasecmp(equalsPos+1, gbKeyNames[i]) == 0) {
                    gbKey = i;
                    break;
                }
            }

            if (gbKey != -1 && dsKey != -1) {
                if (keyConfigs.empty())
                    keyConfigs.push_back(defaultKeyConfig);
                KeyConfig* config = &keyConfigs.back();
                config->funcKeys[dsKey] = gbKey;
            }
        }
    }
}

void controlsCheckConfig() {
    if (keyConfigs.empty())
        keyConfigs.push_back(defaultKeyConfig);
    if (selectedKeyConfig >= keyConfigs.size())
        selectedKeyConfig = 0;
    loadKeyConfig();
}

void controlsPrintConfig(FileHandle* file) {
    file_printf(file, "config=%d\n", selectedKeyConfig);
    for (unsigned int i=0; i<keyConfigs.size(); i++) {
        file_printf(file, "(%s)\n", keyConfigs[i].name);
        for (int j=0; j<NUM_BINDABLE_BUTTONS; j++) {
            const int function = keyConfigs[i].funcKeys[j];
            const char* functionName = function >= 0 &&
                function < NUM_FUNC_KEYS ? gbKeyNames[function] :
                gbKeyNames[FUNC_KEY_NONE];
            file_printf(file, "%s=%s\n", dsKeyNames[j], functionName);
        }
    }
}

int keyConfigChooser_option;

void redrawKeyConfigChooser() {
    int& option = keyConfigChooser_option;
    KeyConfig* config = &keyConfigs[selectedKeyConfig];

    clearConsole();

    printf("Config: ");
    if (option == -1)
        iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, "* %s *\n\n", config->name);
    else
        printf("  %s  \n\n", config->name);

    printf("       Button   Function\n\n");

    for (int i=0; i<NUM_BINDABLE_BUTTONS; i++) {
#if defined(_3DS)
       // These button bits aren't assigned to anything, so no strings for them
       if((i > 15 && i < 24) || i == 12 || i == 13)
            continue;
#endif

        int len = 11-strlen(dsKeyNames[i]);
        while (len > 0) {
            printf(" ");
            len--;
        }
        if (option == i) 
            iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, "* %s | %s *\n", dsKeyNames[i], gbKeyNames[config->funcKeys[i]]);
        else
            printf("  %s | %s  \n", dsKeyNames[i], gbKeyNames[config->funcKeys[i]]);
    }
    printf("\nPress X to make a new config.");
    if (selectedKeyConfig != 0) /* can't erase the default */ {
        printf("\n\nPress Y to delete this config.");
    }
}

void updateKeyConfigChooser() {
    bool redraw = false;

    int& option = keyConfigChooser_option;
    KeyConfig* config = &keyConfigs[selectedKeyConfig];

    if (keyJustPressed(KEY_B)) {
        loadKeyConfig();
        closeSubMenu();
    }
    else if (keyJustPressed(KEY_X)) {
        keyConfigs.push_back(KeyConfig(*config));
        selectedKeyConfig = keyConfigs.size()-1;
        char name[32];
        snprintf(name, sizeof(name), "Custom %u",
                 (unsigned int)keyConfigs.size()-1);
        copyConfigText(keyConfigs.back().name,
                       sizeof(keyConfigs.back().name), name);
        option = -1;
        redraw = true;
    }
    else if (keyJustPressed(KEY_Y)) {
        if (selectedKeyConfig != 0) /* can't erase the default */ {
            keyConfigs.erase(keyConfigs.begin() + selectedKeyConfig);
            if (selectedKeyConfig >= keyConfigs.size())
                selectedKeyConfig = keyConfigs.size() - 1;
            redraw = true;
        }
    }
    else if (keyPressedAutoRepeat(KEY_DOWN)) {
        if (option == NUM_BINDABLE_BUTTONS-1)
            option = -1;
#if defined(_3DS)
        else if(option == 11) //Skip nonexistant keys
            option = 14;
        else if(option == 15)
            option = 24;
#endif
        else
            option++;
        redraw = true;
    }
    else if (keyPressedAutoRepeat(KEY_UP)) {
        if (option == -1)
            option = NUM_BINDABLE_BUTTONS-1;
#if defined(_3DS)
        else if(option == 14) //Skip nonexistant keys
            option = 11;
        else if(option == 24)
            option = 15;
#endif
        else
            option--;
        redraw = true;
    }
    else if (keyPressedAutoRepeat(KEY_LEFT)) {
        if (option == -1) {
            if (selectedKeyConfig == 0)
                selectedKeyConfig = keyConfigs.size()-1;
            else
                selectedKeyConfig--;
        }
        else {
            config->funcKeys[option]--;
            if (config->funcKeys[option] < 0)
                config->funcKeys[option] = NUM_FUNC_KEYS-1;
        }
        redraw = true;
    }
    else if (keyPressedAutoRepeat(KEY_RIGHT)) {
        if (option == -1) {
            selectedKeyConfig++;
            if (selectedKeyConfig >= keyConfigs.size())
                selectedKeyConfig = 0;
        }
        else {
            config->funcKeys[option]++;
            if (config->funcKeys[option] >= NUM_FUNC_KEYS)
                config->funcKeys[option] = 0;
        }
        redraw = true;
    }
    if (redraw)
        doAtVBlank(redrawKeyConfigChooser);
}

void startKeyConfigChooser() {
    keyConfigChooser_option = -1;
    displaySubMenu(updateKeyConfigChooser);
    redrawKeyConfigChooser();
}

int mapFuncKey(int funcKey) {
    return keyMapping[funcKey];
}

int mapMenuKey(int menuKey) {
    switch (menuKey) {
        case MENU_KEY_A:
            return KEY_A;
        case MENU_KEY_B:
            return KEY_B;
        case MENU_KEY_LEFT:
            return KEY_LEFT;
        case MENU_KEY_RIGHT:
            return KEY_RIGHT;
        case MENU_KEY_UP:
            return KEY_UP;
        case MENU_KEY_DOWN:
            return KEY_DOWN;
        case MENU_KEY_R:
            return KEY_R;
        case MENU_KEY_L:
            return KEY_L;
        case MENU_KEY_X:
            return KEY_X;
        case MENU_KEY_Y:
            return KEY_Y;
    }
    return 0;
}

#elif defined(SDL)

#include <SDL.h>

// Stub functions for SDL

void controlsParseConfig(char* line2) {
}
void controlsPrintConfig(FileHandle* file) {
}
void controlsCheckConfig() {
}

void startKeyConfigChooser() {
}

int mapFuncKey(int funcKey) {
    switch(funcKey) {
        case FUNC_KEY_NONE:
            return 0;
        case FUNC_KEY_A:
            return SDLK_SEMICOLON;
        case FUNC_KEY_B:
            return SDLK_q;
        case FUNC_KEY_LEFT:
            return SDLK_LEFT;
        case FUNC_KEY_RIGHT:
            return SDLK_RIGHT;
        case FUNC_KEY_UP:
            return SDLK_UP;
        case FUNC_KEY_DOWN:
            return SDLK_DOWN;
        case FUNC_KEY_START:
            return SDLK_RETURN;
        case FUNC_KEY_SELECT:
            return SDLK_BACKSLASH;
        case FUNC_KEY_MENU:
            return SDLK_o;
        case FUNC_KEY_MENU_PAUSE:
            return 0;
        case FUNC_KEY_SAVE:
            return 0;
        case FUNC_KEY_AUTO_A:
            return 0;
        case FUNC_KEY_AUTO_B:
            return 0;
        case FUNC_KEY_FAST_FORWARD:
            return SDLK_SPACE;
        case FUNC_KEY_FAST_FORWARD_TOGGLE:
            return 0;
        case FUNC_KEY_SCALE:
            return 0;
        case FUNC_KEY_RESET:
            return 0;
        case FUNC_KEY_SWAPFOCUS:
            return SDLK_w;
    }
    return 0;
}

int mapMenuKey(int menuKey) {
    switch (menuKey) {
        case MENU_KEY_A:
            return SDLK_SEMICOLON;
        case MENU_KEY_B:
            return SDLK_q;
        case MENU_KEY_UP:
            return SDLK_UP;
        case MENU_KEY_DOWN:
            return SDLK_DOWN;
        case MENU_KEY_LEFT:
            return SDLK_LEFT;
        case MENU_KEY_RIGHT:
            return SDLK_RIGHT;
        case MENU_KEY_L:
            return SDLK_a;
        case MENU_KEY_R:
            return SDLK_o;
    }
    return 0;
}

#endif
