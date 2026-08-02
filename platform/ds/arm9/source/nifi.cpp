#include <nds.h>
#include <dswifi9.h>
#include "romfile.h"
#include "nifi.h"
#include "mmu.h"
#include "main.h"
#include "gameboy.h"
#include "console.h"
#include "inputhelper.h"
#include "gbmanager.h"
#include "menu.h"
#include "soundengine.h"
#include "nifi_protocol.h"

void nifiLinkTypeMenu();
void nifiHostMenu();
void nifiClientMenu();

inline int INT_AT(u8* ptr) {
    return (u32)ptr[0] | ((u32)ptr[1] << 8) |
        ((u32)ptr[2] << 16) | ((u32)ptr[3] << 24);
}
inline void INT_TO(u8* ptr, int i) {
    *ptr = i&0xff;
    *(ptr+1) = (i>>8)&0xff;
    *(ptr+2) = (i>>16)&0xff;
    *(ptr+3) = (i>>24)&0xff;
}

enum ClientStatus {
    CLIENT_IDLE=0,
    CLIENT_WAITING,
    CLIENT_CONNECTING,
    CLIENT_CONNECTED
};
enum HostStatus {
    HOST_IDLE=0,
    HOST_WAITING,
    HOST_CONNECTED
};
enum LinkType {
    LINK_CABLE=0,
    LINK_SGB
};
enum NifiCmd {
    NIFI_CMD_HOST=0,
    NIFI_CMD_CLIENT,
    NIFI_CMD_ACKNOWLEDGE,
    NIFI_CMD_INPUT,
    NIFI_CMD_TRANSFER_SRAM,

    NIFI_CMD_FRAGMENT,
    NIFI_CMD_INPUT_REQUEST,
    NIFI_CMD_STATE_HASH
};

const int CLIENT_FRAME_LAG = 4;
const int FRAGMENT_SIZE = 0x400;

const int OLD_INPUTS_BUFFER_SIZE = CLIENT_FRAME_LAG + 2;

u8* fragmentBuffer = NULL;
u8 lastFragment;

bool nifiEnabled=true;
bool nifiInitialized = false;
volatile bool packetAcknowledged;
volatile u16 acknowledgedSequence = 0xffff;
u16 nextSequence = 1;
u32 localRomId = 0;
u32 linkedRomId = 0;

volatile bool foundClient;
volatile bool foundHost;
bool isClient = false;
bool isHost = false;

int nifiFrameCounter;
int nifiLinkType;
volatile bool receivedSram;

u8* nifiInputDest;      // Where input for this DS goes
u8* nifiOtherInputDest; // Where input from other DS goes

int nifiConsecutiveWaitingFrames = 0;

volatile int status = 0;
volatile u32 hostId;

char linkedFilename[MAX_FILENAME_LEN];
char linkedRomTitle[20];

const int INPUT_BUFFER_SIZE = 64;
volatile u8 receivedInput[INPUT_BUFFER_SIZE];
volatile u32 receivedInputFrame[INPUT_BUFFER_SIZE];
volatile bool receivedInputReady[INPUT_BUFFER_SIZE];
u8 sentInput[INPUT_BUFFER_SIZE];
u32 sentInputFrame[INPUT_BUFFER_SIZE];

struct StateHashEntry {
    u32 frame;
    u32 hash;
};
StateHashEntry localStateHashes[4];

u8 oldInputs[OLD_INPUTS_BUFFER_SIZE];

u32 nifiHashBytes(u32 hash, const void* data, u32 length) {
    const u8* bytes = (const u8*)data;
    for (u32 i = 0; i < length; ++i) {
        hash ^= bytes[i];
        hash *= 16777619U;
    }
    return hash;
}

u32 nifiHashGameboy(Gameboy* gb, u32 hash) {
    if (!gb)
        return hash;
    hash = nifiHashBytes(hash, &gb->gbRegs, sizeof(gb->gbRegs));
    hash = nifiHashBytes(hash, gb->wram, sizeof(gb->wram));
    hash = nifiHashBytes(hash, gb->vram, sizeof(gb->vram));
    hash = nifiHashBytes(hash, gb->highram, sizeof(gb->highram));
    return hash;
}

u32 nifiStateHash() {
    u32 first = nifiHashGameboy(gameboy, 2166136261U);
    u32 second = nifiHashGameboy(gb2, 2166136261U);
    if (nifiLinkType == LINK_CABLE && second < first) {
        u32 swap = first;
        first = second;
        second = swap;
    }
    u32 result = nifiHashBytes(2166136261U, &first, sizeof(first));
    return nifiHashBytes(result, &second, sizeof(second));
}

void nifiSendInputFrame(u32 frame) {
    const int index = frame & (INPUT_BUFFER_SIZE - 1);
    if (sentInputFrame[index] != frame)
        return;
    u8 packet[6];
    packet[0] = 1;
    INT_TO(packet + 1, frame);
    packet[5] = sentInput[index];
    nifiSendPacket(NIFI_CMD_INPUT, packet, sizeof(packet), false);
}

u32 nifiGetLocalRomId() {
    if (!localRomId && gameboy && gameboy->getRomFile()) {
        localRomId = gameboy->getRomFile()->getContentId();
        if (!localRomId)
            localRomId = 1;
    }
    return localRomId;
}

static int nifiSendSinglePacket(u8 command, const u8* data, u32 dataLen,
        bool acknowledge, u16 ackSequence)
{
    if (!nifiEnabled || !nifiInitialized)
        return 1;
    if (dataLen > nifi::MAX_PACKET_PAYLOAD ||
            (command != NIFI_CMD_FRAGMENT && dataLen > FRAGMENT_SIZE))
        return 1;

    int errcode = 0;
    // Packets are bounded by FRAGMENT_SIZE. Keeping this scratch storage on
    // the stack avoids heap churn and fragmentation during link play.
    u8 buffer[nifi::MAX_PACKET_PAYLOAD + nifi::HEADER_SIZE];

    nifi::PacketHeader header;
    memset(&header, 0, sizeof(header));
    header.command = command;
    header.flags = acknowledge ? nifi::FLAG_ACK_REQUIRED : 0;
    header.hostId = hostId;
    header.sequence = nextSequence++;
    header.ackSequence = ackSequence;
    header.payloadSize = dataLen;
    header.fragmentCount = 1;
    header.totalSize = dataLen;
    header.romId = nifiGetLocalRomId();

    const size_t packetSize = nifi::encodePacket(buffer,
            dataLen + nifi::HEADER_SIZE, header, data);
    if (!packetSize) {
        return 1;
    }

    if (acknowledge) {
        packetAcknowledged = false;
        acknowledgedSequence = 0xffff;
    }
    if (Wifi_RawTxFrame(packetSize, 0x0014, (unsigned short *)buffer) != 0) {
        printLog("Nifi send error\n");
        errcode = 1;
    }
    if (acknowledge) {
        int attemptCounter = 0;
        while (acknowledgedSequence != header.sequence) {
            int frameCounter = 0;
            while (acknowledgedSequence != header.sequence && frameCounter < 10) {
                swiWaitForVBlank();
                frameCounter++;
            }
            if (acknowledgedSequence != header.sequence) {
                if (attemptCounter >= 10) {
                    errcode = 1;
                    printLog("Connection lost.\n");
                    nifiStop();
                    break;
                }
                if (Wifi_RawTxFrame(packetSize, 0x0014,
                            (unsigned short *)buffer) != 0) {
                    printLog("Nifi send error\n");
                    errcode = 1;
                    break;
                }
            }
            attemptCounter++;
        }
    }
    return errcode;
}

static void nifiSendAcknowledge(u16 sequence) {
    nifiSendSinglePacket(NIFI_CMD_ACKNOWLEDGE, NULL, 0, false, sequence);
}

int nifiSendPacket(u8 command, u8* data, u32 dataLen, bool acknowledge)
{
    if (command == NIFI_CMD_FRAGMENT || dataLen <= FRAGMENT_SIZE)
        return nifiSendSinglePacket(command, data, dataLen, acknowledge, 0xffff);

    int errcode = 0;
    {
        u8 buffer[FRAGMENT_SIZE + 0x10];

        u8 numFragments = (dataLen+(FRAGMENT_SIZE-1))/FRAGMENT_SIZE;
        if (!numFragments || numFragments > nifi::MAX_FRAGMENT_COUNT) {
            return 1;
        }

        for (int i=0; i<numFragments; i++) {
            int fragmentSize = FRAGMENT_SIZE;
            if (i == numFragments-1) {
                fragmentSize = dataLen % FRAGMENT_SIZE;
                if (fragmentSize == 0)
                    fragmentSize = FRAGMENT_SIZE;
            }

            INT_TO(buffer, dataLen);
            buffer[4] = command;
            buffer[5] = numFragments;
            buffer[6] = i;
            memcpy(buffer+0x10, data+i*FRAGMENT_SIZE, fragmentSize);

            if (nifiSendPacket(NIFI_CMD_FRAGMENT, buffer,
                        fragmentSize+0x10, acknowledge)) {
                errcode = 1;
                break;
            }
            swiWaitForVBlank();
        }

    }

    return errcode;
}

void handlePacketCommand(int command, u8* data, u32 dataLen) {
    switch(command) {
        case NIFI_CMD_CLIENT:
            if (isHost && status == HOST_WAITING) {
                u8 ignoredLinkType = 0;
                if (!nifi::decodeIdentity(data, dataLen, &ignoredLinkType,
                            &linkedRomId, linkedFilename, sizeof(linkedFilename),
                            linkedRomTitle, sizeof(linkedRomTitle))) {
                    printLog("Nifi invalid client identity\n");
                    break;
                }
                foundClient = true;

                printLog("Link romTitle: %s\n", linkedRomTitle);
                printLog("Link filename: %s\n", linkedFilename);
            }
            break;

        case NIFI_CMD_INPUT:
            if (dataLen >= 5) {
                int num = data[0];
                int frame1 = INT_AT(data+1);
                if (num < 0 || (u32)num > dataLen - 5)
                    break;

                if (nifiConsecutiveWaitingFrames >= 60)
                    printLog("Received packet: %x\n", frame1);

                for (int i=0; i<num; i++) {
                    int frame = frame1+i;

                    if (frame >= mgr_frameCounter &&
                            frame < mgr_frameCounter + INPUT_BUFFER_SIZE) {
                        int index = frame & (INPUT_BUFFER_SIZE - 1);
                        if (receivedInputReady[index] &&
                                receivedInputFrame[index] == (u32)frame) {
                            if (receivedInput[index] != data[5+i])
                                printLog("MISMATCH %x\n", frame);
                        }
                        else {
                            receivedInputReady[index] = true;
                            receivedInputFrame[index] = frame;
                            receivedInput[index] = data[5+i];
                        }
                    }
                }
            }
            break;
        case NIFI_CMD_INPUT_REQUEST:
            if (dataLen == 4)
                nifiSendInputFrame(INT_AT(data));
            break;
        case NIFI_CMD_STATE_HASH:
            if (dataLen == 8) {
                const u32 frame = INT_AT(data);
                const u32 remoteHash = INT_AT(data + 4);
                StateHashEntry& local = localStateHashes[(frame / 60) & 3];
                if (local.frame == frame && local.hash != remoteHash)
                    printLog("Nifi desync at frame %x (%x != %x)\n",
                            frame, local.hash, remoteHash);
            }
            break;
        case NIFI_CMD_TRANSFER_SRAM:
            {
                Gameboy* destination = nifiLinkType == LINK_SGB ?
                    gameboy : gb2;
                if (!destination ||
                        !destination->importLinkSaveData(data, dataLen)) {
                    printLog("Nifi bad save-data size\n");
                    break;
                }
                printLog("Received save data.\n");
                receivedSram = true;
            }
            break;

            // A command broken up into multiple packets
        case NIFI_CMD_FRAGMENT:
            {
                if (dataLen < 0x10)
                    return;
                u32 totalSize = INT_AT(data);
                u8 command = data[4];
                u8 numFragments = data[5];
                u8 fragment = data[6];
                if (!totalSize || !numFragments || fragment >= numFragments ||
                        totalSize > (u32)numFragments * FRAGMENT_SIZE ||
                        totalSize <= (u32)(numFragments - 1) * FRAGMENT_SIZE)
                    return;

                int fragmentSize = FRAGMENT_SIZE;
                if (fragment == numFragments-1) {
                    fragmentSize = totalSize % FRAGMENT_SIZE;
                    if (fragmentSize == 0)
                        fragmentSize = FRAGMENT_SIZE;
                }
                if ((u32)fragmentSize + 0x10 != dataLen)
                    return;

                if (fragmentBuffer == NULL && fragment != 0) {
                    printLog("NULL Buffer.\n");
                    return;
                }
                if (fragment == 0) {
                    if (fragmentBuffer != NULL)
                        free(fragmentBuffer);
                    fragmentBuffer = (u8*)malloc(totalSize);
                    if (fragmentBuffer == 0) {
                        printLog("Nifi not enough memory\n");
                        return;
                    }
                }
                else if ((u8)(lastFragment + 1) != fragment) {
                    if (fragmentBuffer != NULL) {
                        free(fragmentBuffer);
                        fragmentBuffer = NULL;
                    }
                    printLog("Fragment mismatch\n");
                    lastFragment = -1;
                    return;
                }

                if (fragment == 0 || lastFragment+1 == fragment)
                    memcpy(fragmentBuffer+fragment*FRAGMENT_SIZE, data+0x10, fragmentSize);

                lastFragment = fragment;

                if (fragment == numFragments-1) {
                    handlePacketCommand(command, fragmentBuffer, totalSize);
                    free(fragmentBuffer);
                    fragmentBuffer = NULL;
                    lastFragment = -1;
                }
            }
            break;
    }
}

void packetHandler(int packetID, int readlength)
{
    static u32 pkt[4096/2];
    static u8* packet = (u8*)pkt;
    if (readlength < 32 + nifi::HEADER_SIZE || readlength > (int)sizeof(pkt))
        return;

    // Wifi_RxRawReadPacket:  Allows user code to read a packet from within the WifiPacketHandler function
    //  long packetID:		a non-unique identifier which locates the packet specified in the internal buffer
    //  long readlength:		number of bytes to read (actually reads (number+1)&~1 bytes)
    //  unsigned short * data:	location for the data to be read into
    
	// bytesRead = Wifi_RxRawReadPacket(packetID, readlength, (unsigned short *)data); // Not used
	Wifi_RxRawReadPacket(packetID, readlength, (unsigned short *)packet);

    nifi::PacketView view;
    nifi::DecodeResult decodeResult = nifi::decodePacket(packet + 32,
            readlength - 32, &view);
    if (decodeResult != nifi::DECODE_OK) {
        if (decodeResult == nifi::DECODE_BAD_CHECKSUM)
            printLog("Nifi bad packet checksum\n");
        return;
    }
    if (!((isClient && status == CLIENT_WAITING) ||
            view.header.hostId == hostId))
        return;

    if (view.header.command == NIFI_CMD_ACKNOWLEDGE) {
        acknowledgedSequence = view.header.ackSequence;
        packetAcknowledged = true;
        return;
    }
    if (view.header.flags & nifi::FLAG_ACK_REQUIRED)
        nifiSendAcknowledge(view.header.sequence);

    u8* data = (u8*)view.payload;
    if (view.header.command == NIFI_CMD_HOST) {
        if (isClient && status == CLIENT_WAITING) {
            u8 receivedLinkType = 0;
            if (!nifi::decodeIdentity(data, view.header.payloadSize,
                        &receivedLinkType, &linkedRomId,
                        linkedFilename, sizeof(linkedFilename),
                        linkedRomTitle, sizeof(linkedRomTitle))) {
                printLog("Nifi invalid host identity\n");
                return;
            }
            if (receivedLinkType > LINK_SGB)
                return;
            hostId = view.header.hostId;
            nifiLinkType = receivedLinkType;
            foundHost = true;
        }
    }
    else
        handlePacketCommand(view.header.command, data, view.header.payloadSize);
}


void nifiStop() {
    isClient = false;
    isHost = false;
    disableNifi();
    nifiUnpause();
}

void enableNifi()
{
    if (nifiInitialized)
        return;

    // Local DS multiplayer doesn't require an Internet access point. Keeping
    // the IP stack disabled also makes DSWiFi fully deinitializable, so local
    // link remains usable after leaving and re-entering the link menu.
    if (!Wifi_InitDefault(INIT_ONLY | WIFI_LOCAL_ONLY)) {
        printLog("Nifi initialization failed\n");
        return;
    }

// Wifi_SetPromiscuousMode: Allows the DS to enter or leave a "promsicuous" mode, in which 
//   all data that can be received is forwarded to the arm9 for user processing.
//   Best used with Wifi_RawSetPacketHandler, to allow user code to use the data
//   (well, the lib won't use 'em, so they're just wasting CPU otherwise.)
//  int enable:  0 to disable promiscuous mode, nonzero to engage
	Wifi_SetPromiscuousMode(1);

// Wifi_RawSetPacketHandler: Set a handler to process all raw incoming packets
//  WifiPacketHandler wphfunc:  Pointer to packet handler (see WifiPacketHandler definition for more info)
	Wifi_RawSetPacketHandler(packetHandler);

// Wifi_SetChannel: If the wifi system is not connected or connecting to an access point, instruct
//   the chipset to change channel
//  int channel: the channel to change to, in the range of 1-13
	Wifi_SetChannel(10);

// Wifi_EnableWifi: Instructs the ARM7 to go into a basic "active" mode, not actually
//   associated to an AP, but actively receiving and potentially transmitting
	Wifi_EnableWifi();

    nifiInitialized = true;
}

void disableNifi() {
    if (!nifiInitialized)
        return;

    Wifi_RawSetPacketHandler(NULL);
    Wifi_SetPromiscuousMode(0);
    Wifi_DisableWifi();
    // DSWiFi requires the ARM7 to observe disabled mode before resources can
    // be released. These bounded waits avoid stale handlers on the next link.
    swiWaitForVBlank();
    swiWaitForVBlank();
    if (!Wifi_Deinit())
        printLog("Nifi deinitialization failed\n");
    nifiInitialized = false;
}

void nifiInterLinkMenu() {
    int selection = 0;
    receivedSram = false;

    for (;;) {
        swiWaitForVBlank();
        clearConsole();

        printf("\n\n");
        if (selection == 0) {
            iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, "* As Host\n\n");
            iprintfColored(CONSOLE_COLOR_WHITE, "  As Client\n\n");
        }
        else {
            iprintfColored(CONSOLE_COLOR_WHITE, "  As Host\n\n");
            iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, "* As Client\n\n");
        }

        inputUpdateVBlank();
        if (keyJustPressed(mapMenuKey(MENU_KEY_B)))
            break;
        if (keyJustPressed(mapMenuKey(MENU_KEY_A))) {
            isHost = selection == 0;
            if (isHost)
                nifiLinkTypeMenu();
            else
                nifiClientMenu();
            break;
        }
        if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_UP) | mapMenuKey(MENU_KEY_DOWN)))
            selection = !selection;
    }
}

void nifiLinkTypeMenu() {
    int selection = 0;

    if (gameboy)
        gameboy->getSoundEngine()->mute();

    for (;;) {
        swiWaitForVBlank();
        clearConsole();

        printf("\n\n");
        if (selection == 0) {
            iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, "* Cable Link\n\n");
            iprintfColored(CONSOLE_COLOR_WHITE, "  SGB Multiplayer\n\n");
        }
        else {
            iprintfColored(CONSOLE_COLOR_WHITE, "  Cable Link\n\n");
            iprintfColored(CONSOLE_COLOR_LIGHT_YELLOW, "* SGB Multiplayer\n\n");
        }

        inputUpdateVBlank();
        if (keyJustPressed(mapMenuKey(MENU_KEY_B)))
            break;
        if (keyJustPressed(mapMenuKey(MENU_KEY_A))) {
            nifiLinkType = selection;

            if (isHost)
                nifiHostMenu();
            else
                nifiClientMenu();
//             closeMenu();
            break;
        }
        if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_UP) | mapMenuKey(MENU_KEY_DOWN)))
            selection = !selection;
    }

    if (gameboy)
        gameboy->getSoundEngine()->unmute();
}

void nifiSendSram() {
    const u32 dataSize = gameboy->getLinkSaveDataSize();
    if (!dataSize)
        return;
    u8* data = (u8*)malloc(dataSize);
    if (!data)
        return;
    if (gameboy->exportLinkSaveData(data, dataSize))
        nifiSendPacket(NIFI_CMD_TRANSFER_SRAM, data, dataSize, true);
    free(data);
    printLog("Sent save data.\n");
}

int nifiReceiveSram() {
    nifiConsecutiveWaitingFrames = 0;
    while (!receivedSram) {
        swiWaitForVBlank();
        nifiConsecutiveWaitingFrames++;
        if (nifiConsecutiveWaitingFrames >= 60*5) {
            return 1;
        }
    }
    return 0;
}

int loadOtherRom() {
    if (nifiLinkType != LINK_CABLE)
        return 0;
    if (gameboy->getRomFile()->getContentId() == linkedRomId)
        return 0;
    if (!file_exists(linkedFilename))
        return 1;

    gameboy->getRomFile()->halfMemoryMode();

    if (gb2->getRomFile() != NULL && gameboy->getRomFile() != gb2->getRomFile())
        delete gb2->getRomFile();
    RomFile* linkedRom = new RomFile(linkedFilename, true);
    if (!linkedRom || linkedRom->getContentId() != linkedRomId) {
        delete linkedRom;
        return 1;
    }
    gb2->setRomFile(linkedRom);

    // Size and map SRAM before initMMU stores its bank pointers.
    if (gb2->loadSave(-1) != 0)
        return 1;
    gb2->init();

    return 0;
}

int nifiStartLink() {
    nifiFrameCounter = -1;
    memset((void*)receivedInputReady, 0, sizeof(receivedInputReady));
    memset(sentInputFrame, 0xff, sizeof(sentInputFrame));
    memset(localStateHashes, 0, sizeof(localStateHashes));
    memset(oldInputs, 0xff, sizeof(oldInputs));

    mgr_reset();
    if (nifiLinkType == LINK_CABLE) {
        printLog("Start Gb2\n");
        if (!mgr_startGb2(-1))
            return 1;
        if (loadOtherRom() != 0) {
            printf("Error loading \"%s\".\n");
            return 1;
        }
    }

    if (isHost) {
        if (nifiLinkType == LINK_CABLE)
            mgr_setInternalClockGb(gameboy);

        // Fill in first few frames of client's input
        for (int i=0; i<CLIENT_FRAME_LAG; i++) {
            receivedInputReady[i] = true;
            receivedInputFrame[i] = i;
            receivedInput[i] = 0xff;
        }

        // Set input destinations
        if (nifiLinkType == LINK_SGB) {
            nifiInputDest = &gameboy->controllers[0];
            nifiOtherInputDest = &gameboy->controllers[1];
        }
        else if (nifiLinkType == LINK_CABLE) {
            nifiInputDest = &gameboy->controllers[0];
            nifiOtherInputDest = &gb2->controllers[0];
        }

    }
    else if (isClient) {
        if (nifiLinkType == LINK_CABLE)
            mgr_setInternalClockGb(gb2);

        // Set input destinations
        if (nifiLinkType == LINK_SGB) {
            nifiInputDest = &gameboy->controllers[1];
            nifiOtherInputDest = &gameboy->controllers[0];
        }
        else if (nifiLinkType == LINK_CABLE) {
            nifiInputDest = &gameboy->controllers[0];
            nifiOtherInputDest = &gb2->controllers[0];
        }

    }

    const bool localHasSram = gameboy->getLinkSaveDataSize() != 0;
    const bool remoteHasSram = nifiLinkType == LINK_CABLE && gb2 &&
        gb2->getLinkSaveDataSize() != 0;
    if (isHost) {
        if (localHasSram)
            nifiSendSram();
        swiWaitForVBlank();
        if (remoteHasSram) {
            if (nifiReceiveSram())
                return 1;
        }
    }
    else {
        if (nifiLinkType == LINK_SGB ? localHasSram : remoteHasSram) {
            if (nifiReceiveSram())
                return 1;
        }
        if (nifiLinkType == LINK_CABLE && localHasSram)
            nifiSendSram();
    }

    nifiConsecutiveWaitingFrames = 0;
    return 0;
}

void nifiHostMenu() {
    enableNifi();
    clearConsole();

    if (!nifiInitialized) {
        printf("Wireless hardware is unavailable.\n");
        printf("Press B to return.\n");
        while (true) {
            swiWaitForVBlank();
            inputUpdateVBlank();
            if (keyJustPressed(KEY_B))
                return;
        }
    }

    foundClient = false;
    isHost = true;
    isClient = false;
    status = HOST_WAITING;
    hostId = rand();
    localRomId = 0;
    nextSequence = 1;

    printf("Waiting for client...\n");
    printf("Host ID: %d\n\n", hostId);
    printf("Press B to give up.\n\n");

    bool willConnect=false;
    while (!foundClient) {
        swiWaitForVBlank();
        swiWaitForVBlank();
        swiWaitForVBlank();
        inputUpdateVBlank();
        if (keyJustPressed(KEY_B))
            break;

        const char* filename = gameboy->getRomFile()->getFilename();
        u8 buffer[MAX_FILENAME_LEN + 64];
        size_t bufferSize = nifi::encodeIdentity(buffer, sizeof(buffer),
                nifiLinkType, nifiGetLocalRomId(), filename,
                gameboy->getRomFile()->getRomTitle());
        if (bufferSize)
            nifiSendPacket(NIFI_CMD_HOST, buffer, bufferSize, false);
    }

    if (foundClient) {
        printf("Found client.\n");
        status = HOST_CONNECTED;
        willConnect = true;
    }
    else {
        isHost = false;
        status = 0;
        printf("Couldn't find client.\n");
        willConnect = false;
    }
    
    if (willConnect) {
        if (nifiStartLink() != 0)
            printf("Link failed.\n");
        else
            printf("Starting link.\n");
    }

    for (int i=0; i<90; i++) swiWaitForVBlank();
}

void nifiClientMenu() {
    enableNifi();
    consoleClear();

    if (!nifiInitialized) {
        printf("Wireless hardware is unavailable.\n");
        printf("Press B to return.\n");
        while (true) {
            swiWaitForVBlank();
            inputUpdateVBlank();
            if (keyJustPressed(KEY_B))
                return;
        }
    }
    printf("Waiting for host...\n\n");
    printf("Press B to give up.\n\n");

    foundHost = false;
    isClient = true;
    isHost = false;
    status = CLIENT_WAITING;
    localRomId = 0;
    nextSequence = 1;

    while (!foundHost) {
        swiWaitForVBlank();
        swiWaitForVBlank();
        swiWaitForVBlank();
        inputUpdateVBlank();
        if (keyJustPressed(KEY_B))
            break;
    }

    bool willConnect = false;
    if (foundHost) {
        printf("Found host.\n\n");
        printf("Host ROM: \"%s\"\n", linkedRomTitle);
        printf("Filename: \"%s\"\n", linkedFilename);
        printf("Link Type: ");
        if (nifiLinkType == LINK_CABLE)
            printf("Cable Link\n\n");
        else if (nifiLinkType == LINK_SGB)
            printf("SGB Multiplayer\n\n");
        printf("Press A to connect, B to cancel.\n\n");

        while (true) {
            swiWaitForVBlank();
            inputUpdateVBlank();

            if (keyJustPressed(KEY_A)) {
                const char* filename = gameboy->getRomFile()->getFilename();
                u8 buffer[MAX_FILENAME_LEN + 64];
                size_t bufferSize = nifi::encodeIdentity(buffer, sizeof(buffer),
                        nifiLinkType, nifiGetLocalRomId(), filename,
                        gameboy->getRomFile()->getRomTitle());
                if (!bufferSize ||
                        nifiSendPacket(NIFI_CMD_CLIENT, buffer, bufferSize, true)) {
                    printf("Connection handshake failed.\n");
                    willConnect = false;
                    break;
                }

                willConnect = true;

                printf("Connected to host.\nHost Id: %d\n", hostId);
                status = CLIENT_CONNECTED;

                memset((void*)receivedInputReady, 0, sizeof(receivedInputReady));
                break;
            }
            else if (keyJustPressed(KEY_B)) {
                willConnect = false;
                printf("Connection cancelled.\n");
                break;
            }
        }
    }
    else {
        isClient = false;
        status = 0;
        printf("Couldn't find host.\n");
        nifiStop();
    }
    
    if (willConnect) {
        if (nifiStartLink() != 0)
            printf("Link failed.\n");
        else
            printf("Starting link.\n");
    }

    for (int i=0; i<90; i++) swiWaitForVBlank();
}

bool nifiIsHost() { return isHost; }
bool nifiIsClient() { return isClient; }
bool nifiIsLinked() { return isHost || isClient; }

int nifiWasPaused = -1;
void nifiPause() {
    if (nifiWasPaused == -1) {
        nifiWasPaused = mgr_isPaused();
    }
    mgr_pause();
}
void nifiUnpause() {
    if (nifiWasPaused == -1)
        return;
    if (!nifiWasPaused) {
        mgr_unpause();
    }
    nifiWasPaused = -1;
}

void nifiUpdateInput() {
    u8* inputDest;
    u8* otherInputDest = nifiOtherInputDest;
    if (nifiIsLinked())
        inputDest = nifiInputDest;
    else
        inputDest = &gameboy->controllers[0];

    u32 bfr[4];
    u8* buffer = (u8*)bfr;

    u32 actualFrame = mgr_frameCounter;
    u32 inputFrame = mgr_frameCounter;
    bool frameHasPassed = nifiFrameCounter != mgr_frameCounter;
    if (nifiFrameCounter == -1)
        printf("Start at %d", mgr_frameCounter);
    if (frameHasPassed && nifiFrameCounter > 0)
        receivedInputReady[(nifiFrameCounter-1)&(INPUT_BUFFER_SIZE-1)] = false;
    nifiFrameCounter = mgr_frameCounter;

    if (nifiIsClient())
        inputFrame += CLIENT_FRAME_LAG;

    u8 olderInput = oldInputs[OLD_INPUTS_BUFFER_SIZE-CLIENT_FRAME_LAG];

    if (nifiIsLinked()) {
        if (frameHasPassed) {
            for (int i=0; i<OLD_INPUTS_BUFFER_SIZE-1; i++)
                oldInputs[i] = oldInputs[i+1];
            oldInputs[OLD_INPUTS_BUFFER_SIZE-1] = buttonsPressed;
        }

        int sentIndex = inputFrame & (INPUT_BUFFER_SIZE - 1);
        sentInputFrame[sentIndex] = inputFrame;
        sentInput[sentIndex] = buttonsPressed;

        // Send input to other ds
        INT_TO(buffer+1, inputFrame-OLD_INPUTS_BUFFER_SIZE+1);
        for (int i=0; i<OLD_INPUTS_BUFFER_SIZE; i++)
            buffer[5+i] = oldInputs[i];
        buffer[0] = OLD_INPUTS_BUFFER_SIZE;
        nifiSendPacket(NIFI_CMD_INPUT, buffer, 5+OLD_INPUTS_BUFFER_SIZE, false);

        // Set other controller's input
        int receiveIndex = actualFrame & (INPUT_BUFFER_SIZE - 1);
        if (receivedInputReady[receiveIndex] &&
                receivedInputFrame[receiveIndex] == actualFrame) {
            *otherInputDest = receivedInput[receiveIndex];
            nifiUnpause();
            nifiConsecutiveWaitingFrames = 0;
        }
        else {
            nifiConsecutiveWaitingFrames++;
            printLog("NIFI NOT READY %x\n", nifiFrameCounter);
            if (nifiConsecutiveWaitingFrames == 1 ||
                    (nifiConsecutiveWaitingFrames % 10) == 0) {
                u8 request[4];
                INT_TO(request, actualFrame);
                nifiSendPacket(NIFI_CMD_INPUT_REQUEST, request,
                        sizeof(request), false);
            }
            nifiPause();
        }
        if (nifiConsecutiveWaitingFrames >= 120) {
            nifiConsecutiveWaitingFrames = 0;
            printLog("Connection lost!\n");
            nifiStop();
            printLog("Nifi turned off.\n");
        }

        if (frameHasPassed && (actualFrame % 60) == 0) {
            u8 statePacket[8];
            StateHashEntry& entry = localStateHashes[(actualFrame / 60) & 3];
            entry.frame = actualFrame;
            entry.hash = nifiStateHash();
            INT_TO(statePacket, entry.frame);
            INT_TO(statePacket + 4, entry.hash);
            nifiSendPacket(NIFI_CMD_STATE_HASH, statePacket,
                    sizeof(statePacket), false);
        }
    }

    if (!nifiIsLinked() || nifiIsHost()) {
        *inputDest = buttonsPressed;
    }
    else if (nifiIsClient()) {
        *inputDest = olderInput;
    }
}
