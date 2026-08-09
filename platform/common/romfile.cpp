#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "main.h"
#include "romfile.h"
#include "gbs.h"
#include "console.h"
#include "inputhelper.h"
#include "gameboy.h"
#include "cheats.h"
#include "error.h"
#include "io.h"
#include "nifi_protocol.h"

#ifdef EMBEDDED_ROM
#include "rom_gb.h"
#endif


#ifdef DS
extern bool __dsimode;

#define DSI_MAX_BANKS 512 // 8 megabytes
#define DS_MAX_BANKS 32 // 2 megabytes
#endif

namespace {

uint32_t fingerprintFile(FileHandle* file, int sizeBytes) {
    // Use one Game Boy ROM bank per read. Small DLDI reads are expensive on
    // DS flashcards and made ordinary ROM startup look as if it had frozen.
    uint8_t buffer[romlayout::ROM_BANK_SIZE];
    uint32_t identifier = nifi::romIdentifier(NULL, 0);
    file_seek(file, 0, SEEK_SET);
    for (int offset = 0; offset < sizeBytes; offset += sizeof(buffer)) {
        int bytes = sizeBytes - offset;
        if (bytes > (int)sizeof(buffer))
            bytes = sizeof(buffer);
        file_read(buffer, 1, bytes, file);
        identifier = nifi::romIdentifierUpdate(identifier, buffer, bytes);
    }
    return identifier ? identifier : 1;
}

} // namespace

RomFile::RomFile(const char* f, bool halfMemory) {
    romFile=NULL;
    maxLoadedRomBanks = 0;
    contentId = 0;
    storageBasename[0] = '\0';

    if (!f)
        fatalerr("ROM filename is missing.");
    snprintf(filename, sizeof(filename), "%s", f);

    snprintf(basename, sizeof(basename), "%s", filename);
    char* extension = strrchr(basename, '.');
    if (extension)
        *extension = '\0';

    if (halfMemory)
        halfMemoryMode();
    else
        fullMemoryMode();

    u8 cgbFlag = getCgbFlag();

    int nameLength = 16;
    if (cgbFlag == 0x80 || cgbFlag == 0xc0)
        nameLength = 15;
    for (int i=0; i<nameLength; i++) 
        romTitle[i] = (char)romSlot0[i+0x134];
    romTitle[nameLength] = '\0';

    if (gbsMode) {
        MBC = MBC5;
    }
    else {
        switch (getMapper()) {
            case 0: case 8: case 9:
                MBC = MBC0; 
                break;
            case 1: case 2: case 3:
                MBC = MBC1;
                break;
            case 5: case 6:
                MBC = MBC2;
                break;
                //case 0xb: case 0xc: case 0xd:
                //MBC = MMM01;
                //break;
            case 0xf: case 0x10: case 0x11: case 0x12: case 0x13:
                MBC = MBC3;
                break;
                //case 0x15: case 0x16: case 0x17:
                //MBC = MBC4;
                //break;
            case 0x19: case 0x1a: case 0x1b: 
                MBC = MBC5;
                break;
            case 0x1c: case 0x1d: case 0x1e: // MBC5 with rumble
                MBC = MBC5;
                break;
            case 0x22:
                MBC = MBC7;
                break;
            case 0xea: /* Hack for SONIC5 */
                MBC = MBC1;
                break;
            case 0xfc:  /* Game Boy Camera */
                MBC = MBC_POCKET_CAM;
                break;
            case 0xfe:
                MBC = HUC3;
                break;
            case 0xff:
                MBC = HUC1;
                break;
            default:
                printLog("Unsupported MBC %02x\n", getMapper());
                MBC = MBC5;
                break;
        }

    } // !gbsMode

    // Check number of ram banks
    if (gbsMode)
        numRamBanks = 1;
    else {
        // Prefer the physical ROM length over the often-stale size byte used
        // by patched ROMs, but decode the complete standard RAM-size table.
        numRamBanks = romlayout::ramBankCount(getRamSize());
        if (numRamBanks < 0) {
            printLog("Invalid RAM bank number: %x\nDefaulting to 4 banks\n", getRamSize());
            numRamBanks = 4;
        }
        if (getMBC() == MBC2)
            numRamBanks = 1;
        else if (getMBC() == MBC7) // Probably not correct behaviour
            numRamBanks = 1;
        else if (getMBC() == MBC_POCKET_CAM) // 16 x 8KByte
            numRamBanks = 16;
    }
}

RomFile::~RomFile() {
    if (romFile != NULL)
        file_close(romFile);
#ifndef EMBEDDED_ROM
    free(romBankSlots);
#endif
}


void RomFile::loadRomBank(int romBank) {
    romBank = normalizeRomBank(romBank);
    if (romBank < 0)
        return;
    if (bankSlotIDs[romBank] != -1) {
        romSlot1 = romBankSlots+bankSlotIDs[romBank]*0x4000;
        return;
    }
    int bankToUnload = lastBanksUsed.back();
    lastBanksUsed.pop_back();
    int slot = bankSlotIDs[bankToUnload];
    bankSlotIDs[bankToUnload] = -1;
    bankSlotIDs[romBank] = slot;

    memset(romBankSlots+slot*0x4000, 0xff, 0x4000);
    file_seek(romFile, 0x4000*romBank, SEEK_SET);
    file_read(romBankSlots+slot*0x4000, 1, 0x4000, romFile);

    lastBanksUsed.insert(lastBanksUsed.begin(), romBank);

    gameboy->getCheatEngine()->applyGGCheatsToBank(romBank);

    romSlot1 = romBankSlots+slot*0x4000;
}

bool RomFile::isRomBankLoaded(int bank) {
    bank = normalizeRomBank(bank);
    return bank >= 0 && bankSlotIDs[bank] != -1;
}
u8* RomFile::getRomBank(int bank) {
    bank = normalizeRomBank(bank);
    if (!isRomBankLoaded(bank))
        return 0;
    return romBankSlots+bankSlotIDs[bank]*0x4000;
}

const char* RomFile::getBasename() {
    return basename;
}

const char* RomFile::getStorageBasename() {
    return storageBasename[0] ? storageBasename : basename;
}

const char* RomFile::getFilename() {
    return filename;
}

uint32_t RomFile::getContentId() {
    if (contentId != 0)
        return contentId;

    // The identifier is only needed when a link session is started. Deferring
    // this full-file hash keeps normal ROM startup responsive on slow DLDI
    // storage. When all banks are resident (the usual DSi/3DS case), avoid a
    // second filesystem pass altogether.
    if (!gbsMode && romSizeBytes > 0 &&
            numRomBanks <= numLoadedRomBanks) {
        contentId = nifi::romIdentifier(romBankSlots, romSizeBytes);
    }
    else {
        FileHandle* source = romFile;
        bool closeSource = false;
        int oldPosition = 0;

        if (source != NULL) {
            oldPosition = file_tell(source);
        }
        else {
            source = file_open(filename, "rb");
            closeSource = source != NULL;
        }

        if (source != NULL) {
            contentId = fingerprintFile(source, romSizeBytes);
            if (closeSource)
                file_close(source);
            else
                file_seek(source, oldPosition, SEEK_SET);
        }
    }

    // A zero identifier is reserved for "unknown" by the link protocol.
    // This deterministic header fallback also keeps diagnostics usable if a
    // cartridge file becomes unavailable after it has been loaded.
    if (contentId == 0) {
        contentId = nifi::romIdentifier(romSlot0, 0x150);
        contentId = nifi::romIdentifierUpdate(contentId,
                reinterpret_cast<const uint8_t*>(&romSizeBytes),
                sizeof(romSizeBytes));
        if (contentId == 0)
            contentId = 1;
    }

    return contentId;
}


bool RomFile::loadBios(const char* filename) {
    // biosExists is shared platform state, but the BIOS buffer belongs to
    // each RomFile. Always reload it for a newly selected cartridge instead
    // of assuming a previous RomFile's buffer is still alive.
    biosExists = false;
    if (!filename || !filename[0])
        return false;

    FileHandle* file = file_open(filename, "rb");
    if (!file)
        return false;

    // GameYob implements the 0x900-byte Game Boy Color boot ROM. Rejecting
    // partial or unrelated files prevents uninitialised BIOS bytes from
    // being mapped into the emulated address space.
    if (file_getSize(file) != (int)sizeof(bios)) {
        file_close(file);
        return false;
    }

    file_read(bios, 1, sizeof(bios), file);
    file_close(file);
    biosExists = true;

    // Little hack to preserve "quickread" from gbcpu.cpp.
    for (int i=0x100; i<0x150; i++)
        bios[i] = romSlot0[i];

    return true;
}

char* RomFile::getRomTitle() {
    return romTitle;
}

void RomFile::halfMemoryMode() {
    int _maxLoadedRomBanks;
#ifdef DS
    if (__dsimode)
        _maxLoadedRomBanks = DSI_MAX_BANKS;
    else
        _maxLoadedRomBanks = DS_MAX_BANKS;

    _maxLoadedRomBanks >>= 1;

#else
    _maxLoadedRomBanks = 512;
#endif

    if (maxLoadedRomBanks == _maxLoadedRomBanks)
        return;

    maxLoadedRomBanks = _maxLoadedRomBanks;
    loadBanks();
}

void RomFile::fullMemoryMode() {
    int _maxLoadedRomBanks;
#ifdef DS
    if (__dsimode)
        _maxLoadedRomBanks = DSI_MAX_BANKS;
    else
        _maxLoadedRomBanks = DS_MAX_BANKS;

#else
    _maxLoadedRomBanks = 512;
#endif

    if (maxLoadedRomBanks == _maxLoadedRomBanks)
        return;

    maxLoadedRomBanks = _maxLoadedRomBanks;
    loadBanks();
}


void RomFile::loadBanks() {
    // Check if this is a GBS file
    const char* extension = strrchr(filename, '.');
    gbsMode = extension && strcasecmp(extension, ".gbs") == 0;

    if (romFile == NULL)
        romFile = file_open(filename, "rb");
    if (romFile == NULL)
    {
        const int openError = errno;
        FileHandle* diagnostic = file_open("/gameyob_error.log", "w");
        if (diagnostic) {
            file_printf(diagnostic, "ROM open failed (errno %d): %s\n",
                        openError, filename);
            file_close(diagnostic);
        }
        fatalerr("Error opening %s.", filename);
        return;
    }

    // Keep the path that actually succeeded. On DS/DSi this may be the FAT
    // 8.3 alias of a Korean or Japanese long filename. Sidecar files must use
    // a path that can also be created again after the directory is closed.
    setStorageBasename(file_getPath(romFile));

    int payloadSize = 0;
    if (gbsMode) {
        file_read(gbsHeader, 1, 0x70, romFile);
        gbsReadHeader();
        file_seek(romFile, 0, SEEK_END);
        romSizeBytes = file_tell(romFile);
        payloadSize = romSizeBytes > 0x70 ? romSizeBytes - 0x70 : 0;
        numRomBanks = romlayout::bankCountForSize(payloadSize);
    }
    else {
        file_seek(romFile, 0, SEEK_END);
        romSizeBytes = file_tell(romFile);
        payloadSize = romSizeBytes;
        numRomBanks = romlayout::bankCountForSize(payloadSize);
    }

    if (!romlayout::isSupportedSize(payloadSize))
        fatalerr("Unsupported ROM size: %d bytes (maximum is 8 MiB).",
                romSizeBytes);

    //int rawRomSize = file_tell(romFile);
    file_seek(romFile, 0, SEEK_SET);

    if (numRomBanks <= maxLoadedRomBanks)
        numLoadedRomBanks = numRomBanks;
    else
        numLoadedRomBanks = maxLoadedRomBanks;

    for (int i=0; i<numRomBanks; i++) {
        bankSlotIDs[i] = -1;
    }

    // Load rom banks and initialize all those "bank" arrays
    lastBanksUsed = std::vector<int>();

    if (romBankSlots != NULL)
        free(romBankSlots);
    int allocationSlots = numLoadedRomBanks < 2 ? 2 : numLoadedRomBanks;
    romBankSlots = (u8*)malloc(allocationSlots*0x4000);
    if (romBankSlots == NULL)
        fatalerr("Not enough memory to load ROM banks.");
    memset(romBankSlots, 0xff, allocationSlots*0x4000);

    // Read bank 0
    if (gbsMode) {
        bankSlotIDs[0] = 0;
        file_seek(romFile, 0x70, SEEK_SET);
        file_read(romBankSlots+gbsLoadAddress, 1, 0x4000-gbsLoadAddress, romFile);
    }
    else {
        bankSlotIDs[0] = 0;
        file_seek(romFile, 0, SEEK_SET);
        file_read(romBankSlots, 1, 0x4000, romFile);
    }
    // Read the rest of the banks
    for (int i=1; i<numLoadedRomBanks; i++) {
        bankSlotIDs[i] = i;
        file_read(romBankSlots+0x4000*i, 1, 0x4000, romFile);
        lastBanksUsed.push_back(i);
    }

    romSlot0 = romBankSlots;
    romSlot1 = romBankSlots + 0x4000;

    // If we've loaded everything, close the rom file
    if (numRomBanks <= numLoadedRomBanks) {
        file_close(romFile);
        romFile = NULL;
    }
}

void RomFile::setStorageBasename(const char* openedPath) {
    if (!openedPath || !*openedPath)
        openedPath = basename;

    strncpy(storageBasename, openedPath, sizeof(storageBasename) - 1);
    storageBasename[sizeof(storageBasename) - 1] = '\0';
    char* extension = strrchr(storageBasename, '.');
    char* slash = strrchr(storageBasename, '/');
    if (extension && (!slash || extension > slash))
        *extension = '\0';

#ifdef DS
    bool asciiPath = true;
    for (const unsigned char* p =
            reinterpret_cast<const unsigned char*>(storageBasename);
            *p; ++p) {
        if (*p >= 0x80) {
            asciiPath = false;
            break;
        }
    }
    if (!asciiPath) {
        // Some DLDI drivers enumerate a legacy Korean LFN but provide no SFN.
        // Use a deterministic, strict 8.3 basename for writable sidecars in
        // the already-selected directory. The original LFN is still retained
        // for display and for opening an existing same-name cheat/save file.
        uint32_t id = nifi::romIdentifier(
            reinterpret_cast<const uint8_t*>(filename), strlen(filename));
        snprintf(storageBasename, sizeof(storageBasename), "%08lX",
                 static_cast<unsigned long>(id));
    }
#endif
}
