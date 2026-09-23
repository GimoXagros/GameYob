#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <string>

#include "romfile.h"
#include "gameboy.h"
#include "cheats.h"
#include "menu.h"
#include "sgb_host.h"
#include "gbmanager.h"
#include "filechooser.h"
#include "gbs.h"
#include "inputhelper.h"

// The FileHandle, UI, audio and wireless callback boundaries are host stubs.
// Manager, Gameboy, CPU/MMU, RomFile, mapper and SGB logic remain production
// code. This checks lifecycle ordering and boot state, not DS IRQ scheduling,
// a rendered first frame, actual WiFi quiescence or a physical BIOS boot.
static int liveFiles = 0;
static bool shortBiosRead = false;
static bool failBiosSeek = false;
static bool failSaveRead = false;
static bool failClockRead = false;
bool biosExists = false;
bool gbsMode = false;
u8 gbsHeader[0x70] = {};
u8 gbsNumSongs = 0;
u16 gbsLoadAddress = 0;
u16 gbsInitAddress = 0;
u16 gbsPlayAddress = 0;
#ifndef MANAGER_TEST
Gameboy* gameboy = NULL;
#endif
int biosEnabled = 2;
int gbcModeOption = 2;
int sgbModeOption = 0;
bool gbaModeOption = false;
bool probingForBorder = false;
bool autoSavingEnabled = false;
bool sgbBordersEnabled = false;
bool fastForwardKey = false;
bool fastForwardMode = false;
u8 buttonsPressed = 0;
int singleScreenMode = 0;
int stateNum = 0;
FileChooserState romChooserState;
char biosPath[MAX_FILENAME_LEN] = {};
static bool teardownArmed = false;
static bool stopBeforeUnload = true;
static bool stoppedForTeardown = false;
static int nifiStops = 0;
static const char* chooserPath = NULL;
bool printerEnabled = false;
bool soundDisabled = false;
bool sgbBorderLoaded = false;
u8 gfxMask = 0;
int rumbleStrength = 0;
int rumbleInserted = 0;

void gbsReadHeader() {}
void gbsInit() {}
void printLog(const char*, ...) {}
void fatalerr(const char* format, ...) { throw std::runtime_error(format); }
void CheatEngine::applyGGCheatsToBank(int) {}
CheatEngine::CheatEngine(Gameboy* owner) : numCheats(0), gameboy(owner), romFile(NULL) {}
void CheatEngine::unloadCheats() {
    if (teardownArmed && !stoppedForTeardown) stopBeforeUnload = false;
    numCheats = 0;
}
void CheatEngine::setRomFile(RomFile* rom) { romFile = rom; }
void CheatEngine::applyGSCheats() {}
void CheatEngine::loadCheats(const char*) {}
void enableSleepMode() {}
void initGbPrinter() {}
void updateGbPrinter() {}
u8 sendGbPrinterByte(u8 value) { return value; }
bool nifiIsLinked() { return false; }
void refreshGFX() {}
void resetSgbBorder() {}
void unmuteSND() {}
void drawScanline(int) {}
void drawScanline_P2(int) {}
void handleVideoRegister(u8, u8) {}
void writeVram16(u16, u16) {}
void writeVram(u16, u8) {}
void writeHram(u16, u8) {}
void setSgbMask(int) {}
void setSgbTiles(u8*, u8) {}
void setSgbMap(u8*) {}
void clearConsole() {}
void updateScreens(bool) {}
void drawScreen() {}
void system_checkPolls() {}
void inputUpdateVBlank() {}
bool isMenuOn() { return false; }
void updateMenu() {}
int mapFuncKey(int) { return 0; }
bool keyPressed(int) { return false; }
bool keyJustPressed(int) { return false; }
void forceReleaseKey(int) {}
void displayMenu() {}
void gbsCheckInput() {}
void nifiUpdateInput() {}
void disableMenuOption(const char*) {}
void enableMenuOption(const char*) {}
void loadFileChooserState(FileChooserState*) {}
void saveFileChooserState(FileChooserState*) {}
char* startFileChooser(const char*[], int, bool, bool) {
    if (!chooserPath) return NULL;
    char* path = static_cast<char*>(malloc(strlen(chooserPath) + 1));
    if (path) strcpy(path, chooserPath);
    return path;
}
void system_doRumble(bool) {}
int system_getMotionSensorX() { return 0; }
int system_getMotionSensorY() { return 0; }
void system_getCamera(u8*, const u8*) {}
#ifndef MANAGER_TEST
bool mgr_areBothUsingExternalClock() { return false; }
bool mgr_isExternalClockGb(Gameboy*) { return false; }
bool mgr_isInternalClockGb(Gameboy*) { return false; }
#endif
void nifiStop() { ++nifiStops; stoppedForTeardown = true; }
bool file_exists(const char*) { return false; }
void fs_deleteFile(const char*) {}
void file_flush(FileHandle* handle) { fflush(handle->file); }
void file_putc(char value, FileHandle* handle) { fputc(value, handle->file); }
void file_setSize(FileHandle*, size_t) {}
void file_write(const void* source, int size, int count, FileHandle* handle) {
    fwrite(source, size, count, handle->file);
}
void printMenuMessage(const char*) {}
time_t getTime() { return 0; }
void file_printf(FileHandle*, const char*, ...) {}

FileHandle* file_open(const char* path, const char* mode) {
    FILE* file = fopen(path, mode);
    if (!file) return NULL;
    FileHandle* handle = new FileHandle{};
    handle->file = file;
    handle->filename = const_cast<char*>(path);
    ++liveFiles;
    return handle;
}
const char* file_getPath(FileHandle* handle) { return handle->filename; }
void file_close(FileHandle* handle) {
    if (!handle) return;
    fclose(handle->file);
    delete handle;
    --liveFiles;
}
void file_read(void* data, int size, int count, FileHandle* handle) {
    if (failSaveRead && count == 0x2000 && strstr(handle->filename, ".sav"))
        return;
    if (failClockRead && count == static_cast<int>(sizeof(ClockStruct)) &&
            strstr(handle->filename, ".sav"))
        return;
    if (shortBiosRead && count == 0x900) {
        fread(data, size, count / 2, handle->file);
        return;
    }
    fread(data, size, count, handle->file);
}
void file_seek(FileHandle* handle, int offset, int origin) {
    fseek(handle->file, offset, origin);
}
int file_tell(FileHandle* handle) { return ftell(handle->file); }
int file_getSize(FileHandle* handle) {
    const long previous = ftell(handle->file);
    fseek(handle->file, 0, SEEK_END);
    const long size = ftell(handle->file);
    if (!(failBiosSeek && strcmp(handle->filename, biosPath) == 0))
        fseek(handle->file, previous, SEEK_SET);
    return static_cast<int>(size);
}

static void writeFixture(const char* path, size_t bytes, bool rom, bool color = false) {
    FILE* file = fopen(path, "wb");
    assert(file);
    for (size_t i = 0; i < bytes; ++i) {
        u8 value = static_cast<u8>(i);
        if (rom && i == 0x147) value = 0;
        if (rom && i == 0x143) value = color ? 0x80 : 0;
        if (rom && i == 0x146 && !color) value = 0x03;
        if (rom && i == 0x14b && !color) value = 0x33;
        if (rom && i == 0x149) value = color ? 0x02 : 0;
        fwrite(&value, 1, 1, file);
    }
    fclose(file);
}

static int firstByte(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    const int value = fgetc(file);
    fclose(file);
    return value;
}

int main(int argc, char** argv) {
    assert(argc == 1);
    const std::string defaultRom = std::string(argv[0]) + ".gb";
    const std::string defaultBios = std::string(argv[0]) + ".bios";
    const char* romPath = defaultRom.c_str();
    const char* biosPathArg = defaultBios.c_str();
    writeFixture(romPath, 0x8000, true);
    const std::string largePath = std::string(romPath) + ".large.gb";
    const std::string gbsPath = std::string(romPath) + ".gbs";
    writeFixture(largePath.c_str(), 0x10000, true, true);
    writeFixture(gbsPath.c_str(), 0x8070, false);
    writeFixture(biosPathArg, 0x900, false);
#ifdef MANAGER_TEST
    snprintf(biosPath, sizeof(biosPath), "%s", biosPathArg);
    mgr_init();
    bool savedLarge = false;
    for (int session = 0; session < 1000; ++session) {
        sgbModeOption = session % 10 == 2 ? 1 : 0;
        gbcModeOption = session % 10 == 2 ? 0 : 2;
        biosEnabled = session % 10 == 2 ? 0 : 2;
        shortBiosRead = (session % 2) != 0;
        mgr_loadRom(session % 2 ? largePath.c_str() : romPath);
        if (session % 2) {
            if (!gameboy->externRam) return 7;
            if (savedLarge && gameboy->externRam[0] != 0x5a) return 8;
            gameboy->externRam[0] = 0x5a;
            gameboy->saveGame();
            savedLarge = true;
        }
        const bool expected = !shortBiosRead && biosEnabled == 2;
        const int expectedPc = expected ? 0 : 0x100;
        const u8* expectedMap = expected ? gameboy->getRomFile()->bios :
            gameboy->getRomFile()->romSlot0;
        if (gameboy->gbRegs.pc.w != expectedPc || gameboy->memory[0] != expectedMap)
            return 3;
        if (session % 10 == 2 && !gameboy->sgbMode) return 12;
        if (session % 100 == 0) {
            RomFile* shared = gameboy->getRomFile();
            if (!mgr_startGb2(-1) || gb2->getRomFile() != shared) return 9;
            mgr_swapFocus();
            if (gameboy->getRomFile() != shared || gb2->getRomFile() != shared)
                return 10;
        }
        teardownArmed = true;
        stoppedForTeardown = false;
        mgr_unloadRom();
        if (!stopBeforeUnload || !stoppedForTeardown || gameboy->getRomFile() || gb2) {
            fprintf(stderr, "first mismatch session=%d phase=manager_unload stopBeforeUnload=%d stopped=%d mainRom=%d gb2=%d\n",
                    session, stopBeforeUnload, stoppedForTeardown,
                    gameboy->getRomFile() != NULL, gb2 != NULL);
            return 6;
        }
        teardownArmed = false;
        mgr_unloadRom();
        if (liveFiles != 0) return 2;
    }
    biosEnabled = 2;
    sgbModeOption = 0;
    gbcModeOption = 2;
    shortBiosRead = false;
    mgr_loadRom(romPath);
    chooserPath = largePath.c_str();
    teardownArmed = true;
    stoppedForTeardown = false;
    mgr_selectRom();
    if (!stopBeforeUnload || !gameboy->getRomFile() ||
            strcmp(gameboy->getRomFile()->getFilename(), largePath.c_str()) != 0)
        return 11;
    teardownArmed = false;
    mgr_unloadRom();
    const std::string wrongBios = std::string(biosPathArg) + ".wrong";
    writeFixture(wrongBios.c_str(), 0x100, false);
    const std::string missingBios = std::string(biosPathArg) + ".missing";
    const char* failedBiosPaths[] = {missingBios.c_str(), wrongBios.c_str(), biosPathArg};
    for (int failure = 0; failure < 3; ++failure) {
        snprintf(biosPath, sizeof(biosPath), "%s", failedBiosPaths[failure]);
        failBiosSeek = failure == 2;
        mgr_loadRom(romPath);
        if (biosEnabled != 2 || biosExists || gameboy->biosOn ||
                gameboy->gbRegs.pc.w != 0x100 ||
                gameboy->memory[0] != gameboy->getRomFile()->romSlot0)
            return 14;
        mgr_unloadRom();
    }
    failBiosSeek = false;
    snprintf(biosPath, sizeof(biosPath), "%s", biosPathArg);
    failSaveRead = true;
    mgr_loadRom(largePath.c_str());
    if (!gameboy->externRam) return 18;
    const std::string largeSave =
        std::string(gameboy->getRomFile()->getStorageBasename()) + ".sav";
    if (liveFiles != 0 || firstByte(largeSave.c_str()) != 0x5a) return 20;
    gameboy->saveGame();
    if (firstByte(largeSave.c_str()) != 0x5a) return 21;
    mgr_unloadRom();
    failSaveRead = false;
    mgr_loadRom(largePath.c_str());
    if (!gameboy->externRam || gameboy->externRam[0] != 0x5a) {
        fprintf(stderr, "first mismatch phase=save_read_failure expected=5a actual=%02x\n",
                gameboy->externRam ? gameboy->externRam[0] : 0);
        return 19;
    }
    mgr_unloadRom();
    const std::string rtcPath = std::string(romPath) + ".rtc.gb";
    writeFixture(rtcPath.c_str(), 0x10000, true, true);
    {
        FILE* rtcFixture = fopen(rtcPath.c_str(), "r+b");
        assert(rtcFixture);
        fseek(rtcFixture, 0x147, SEEK_SET);
        fputc(0x13, rtcFixture); // MBC3 with RAM and RTC.
        fclose(rtcFixture);
    }
    mgr_loadRom(rtcPath.c_str());
    if (!gameboy->externRam) return 22;
    gameboy->externRam[0] = 0x6b;
    gameboy->saveGame();
    mgr_unloadRom();
    failClockRead = true;
    mgr_loadRom(rtcPath.c_str());
    const std::string rtcSave =
        std::string(gameboy->getRomFile()->getStorageBasename()) + ".sav";
    if (liveFiles != 0 || firstByte(rtcSave.c_str()) != 0x6b) return 23;
    gameboy->saveGame();
    mgr_unloadRom();
    failClockRead = false;
    mgr_loadRom(rtcPath.c_str());
    if (!gameboy->externRam || gameboy->externRam[0] != 0x6b) return 24;
    mgr_unloadRom();
    mgr_loadRom(romPath);
    if (!biosExists || !gameboy->biosOn || gameboy->gbRegs.pc.w != 0 ||
            gameboy->memory[0] != gameboy->getRomFile()->bios)
        return 15;
    gameboy->gbRegs.pc.w = 0x2345;
    gameboy->saveState(0);
    const std::string badState =
        std::string(gameboy->getRomFile()->getStorageBasename()) + ".ys1";
    writeFixture(badState.c_str(), 3, false);
    gameboy->gbRegs.pc.w = 0x3456;
    if (gameboy->loadState(1) == 0 || gameboy->gbRegs.pc.w != 0x3456)
        return 16;
    if (gameboy->loadState(0) != 0 || gameboy->gbRegs.pc.w != 0x2345)
        return 17;
    mgr_unloadRom();
    mgr_loadRom(gbsPath.c_str());
    if (!gbsMode || gameboy->biosOn || gameboy->gbRegs.pc.w != 0x100)
        return 13;
    mgr_unloadRom();
    mgr_exit();
    printf("PASS: 1000 production manager load/unload cycles, NiFi stop count=%d\n", nifiStops);
    return 0;
#endif
    for (int session = 0; session < 1000; ++session) {
        RomFile rom(session % 2 ? largePath.c_str() : romPath);
        shortBiosRead = (session % 2) != 0;
        const bool loaded = rom.loadBios(biosPathArg);
        const bool expected = !shortBiosRead;
        if (loaded != expected || biosExists != expected) {
            fprintf(stderr, "first mismatch session=%d phase=bios_read expected=%d loaded=%d biosExists=%d liveFiles=%d\n",
                    session, expected, loaded, biosExists, liveFiles);
            return 1;
        }
        if (liveFiles != 0) return 2;
#ifndef ROMFILE_ONLY
        {
            Gameboy gb;
            gameboy = &gb;
            gb.setRomFile(&rom);
            if (gb.loadSave(-1) != 0) return 4;
            gb.init();
            const int expectedPc = expected ? 0 : 0x100;
            const u8* expectedMap = expected ? rom.bios : rom.romSlot0;
            if (gb.gbRegs.pc.w != expectedPc || gb.memory[0] != expectedMap) {
                fprintf(stderr, "first mismatch session=%d phase=cpu_mmu expectedPC=%04x actualPC=%04x biosMapped=%d\n",
                        session, expectedPc, gb.gbRegs.pc.w, gb.memory[0] == rom.bios);
                return 3;
            }
            gb.unloadRom();
            gb.unloadRom();
            if (gb.getRomFile() != NULL || gb.externRam != NULL) return 5;
        }
        gameboy = NULL;
#endif
    }
    puts("PASS: 1000 production RomFile/Gameboy boot/unload cycles");
    return 0;
}
