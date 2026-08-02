#include <3ds.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <malloc.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "gameboy.h"
#include "gbmanager.h"
#include "inputhelper.h"
#include "io.h"
#include "localization.h"
#include "menu.h"
#include "nifi.h"
#include "nifi_protocol.h"
#include "romfile.h"
#include "soundengine.h"

namespace {

enum ClientStatus {
    CLIENT_IDLE = 0,
    CLIENT_WAITING,
    CLIENT_CONNECTED
};

enum HostStatus {
    HOST_IDLE = 0,
    HOST_WAITING,
    HOST_CONNECTED
};

enum LinkType {
    LINK_CABLE = 0,
    LINK_SGB
};

enum NifiCommand {
    NIFI_CMD_HOST = 0,
    NIFI_CMD_CLIENT,
    NIFI_CMD_ACKNOWLEDGE,
    NIFI_CMD_INPUT,
    NIFI_CMD_TRANSFER_SRAM,
    NIFI_CMD_FRAGMENT,
    NIFI_CMD_INPUT_REQUEST,
    NIFI_CMD_STATE_HASH
};

const int LAN_PORT = 35553;
const int SOC_BUFFER_SIZE = 0x100000;
const int CLIENT_FRAME_LAG = 4;
const int INPUT_BUFFER_SIZE = 64;
const int OLD_INPUTS_BUFFER_SIZE = CLIENT_FRAME_LAG + 2;
const int FRAGMENT_SIZE = 0x400;
const int RECEIVE_BUFFER_SIZE = FRAGMENT_SIZE + nifi::HEADER_SIZE + 0x10;
const int DISCOVERY_TIMEOUT_FRAMES = 60 * 10;
const u32 MAX_REASSEMBLY_SIZE = nifi::MAX_FRAGMENT_COUNT * FRAGMENT_SIZE;

inline u32 read32(const u8* data) {
    return (u32)data[0] | ((u32)data[1] << 8) |
        ((u32)data[2] << 16) | ((u32)data[3] << 24);
}

inline void write32(u8* data, u32 value) {
    data[0] = value;
    data[1] = value >> 8;
    data[2] = value >> 16;
    data[3] = value >> 24;
}

u32* socBuffer = NULL;
int udpSocket = -1;
sockaddr_in broadcastAddress;
sockaddr_in peerAddress;
bool peerAddressKnown = false;
bool nifiInitialized = false;
bool socInitialized = false;

bool isHost = false;
bool isClient = false;
int status = 0;
int nifiLinkType = LINK_CABLE;
volatile bool foundHost = false;
volatile bool foundClient = false;
volatile bool receivedSram = false;
volatile u16 acknowledgedSequence = 0xffff;
u16 nextSequence = 1;
u32 hostId = 0;
u32 localRomId = 0;
u32 linkedRomId = 0;

char linkedFilename[MAX_FILENAME_LEN];
char linkedRomTitle[20];

u8* fragmentBuffer = NULL;
u32 fragmentTotalSize = 0;
u8 lastFragment = 0xff;

volatile u8 receivedInput[INPUT_BUFFER_SIZE];
volatile u32 receivedInputFrame[INPUT_BUFFER_SIZE];
volatile bool receivedInputReady[INPUT_BUFFER_SIZE];
u8 sentInput[INPUT_BUFFER_SIZE];
u32 sentInputFrame[INPUT_BUFFER_SIZE];
u8 oldInputs[OLD_INPUTS_BUFFER_SIZE];
u8* nifiInputDest = NULL;
u8* nifiOtherInputDest = NULL;
int nifiFrameCounter = -1;
int consecutiveWaitingFrames = 0;
int nifiWasPaused = -1;

struct StateHashEntry {
    u32 frame;
    u32 hash;
};
StateHashEntry localStateHashes[4];

u32 hashBytes(u32 hash, const void* data, u32 length) {
    const u8* bytes = static_cast<const u8*>(data);
    for (u32 i=0; i<length; ++i) {
        hash ^= bytes[i];
        hash *= 16777619U;
    }
    return hash;
}

u32 hashGameboy(Gameboy* gb, u32 hash) {
    if (!gb)
        return hash;
    hash = hashBytes(hash, &gb->gbRegs, sizeof(gb->gbRegs));
    hash = hashBytes(hash, gb->wram, sizeof(gb->wram));
    hash = hashBytes(hash, gb->vram, sizeof(gb->vram));
    return hashBytes(hash, gb->highram, sizeof(gb->highram));
}

u32 stateHash() {
    u32 first = hashGameboy(gameboy, 2166136261U);
    u32 second = hashGameboy(gb2, 2166136261U);
    if (nifiLinkType == LINK_CABLE && second < first) {
        const u32 temp = first;
        first = second;
        second = temp;
    }
    u32 result = hashBytes(2166136261U, &first, sizeof(first));
    return hashBytes(result, &second, sizeof(second));
}

u32 getLocalRomId() {
    if (!localRomId && gameboy && gameboy->getRomFile()) {
        localRomId = gameboy->getRomFile()->getContentId();
        if (!localRomId)
            localRomId = 1;
    }
    return localRomId;
}

void pumpPackets();

int sendSinglePacket(u8 command, const u8* data, u32 dataLength,
                     bool acknowledge, u16 ackSequence) {
    if (!nifiInitialized || udpSocket < 0 ||
            dataLength > nifi::MAX_PACKET_PAYLOAD ||
            (command != NIFI_CMD_FRAGMENT && dataLength > FRAGMENT_SIZE))
        return 1;
    if (!peerAddressKnown && !(isHost && status == HOST_WAITING))
        return 1;

    u8 buffer[RECEIVE_BUFFER_SIZE];
    nifi::PacketHeader header;
    memset(&header, 0, sizeof(header));
    header.command = command;
    header.flags = acknowledge ? nifi::FLAG_ACK_REQUIRED : 0;
    header.hostId = hostId;
    header.sequence = nextSequence++;
    header.ackSequence = ackSequence;
    header.payloadSize = dataLength;
    header.fragmentCount = 1;
    header.totalSize = dataLength;
    header.romId = getLocalRomId();
    const size_t packetSize = nifi::encodePacket(buffer, sizeof(buffer),
                                                  header, data);
    if (!packetSize)
        return 1;

    const sockaddr_in& destination = peerAddressKnown ?
        peerAddress : broadcastAddress;
    if (acknowledge)
        acknowledgedSequence = 0xffff;
    for (int attempt=0; attempt<(acknowledge ? 10 : 1); ++attempt) {
        const int sent = sendto(udpSocket, buffer, packetSize, 0,
            reinterpret_cast<const sockaddr*>(&destination),
            sizeof(destination));
        if (sent != static_cast<int>(packetSize))
            return 1;
        if (!acknowledge)
            return 0;

        for (int frame=0; frame<10; ++frame) {
            pumpPackets();
            if (acknowledgedSequence == header.sequence)
                return 0;
            system_waitForVBlank();
        }
    }
    return 1;
}

void sendAcknowledge(u16 sequence) {
    sendSinglePacket(NIFI_CMD_ACKNOWLEDGE, NULL, 0, false, sequence);
}

void sendInputFrame(u32 frame) {
    const int index = frame & (INPUT_BUFFER_SIZE - 1);
    if (sentInputFrame[index] != frame)
        return;
    u8 packet[6];
    packet[0] = 1;
    write32(packet + 1, frame);
    packet[5] = sentInput[index];
    nifiSendPacket(NIFI_CMD_INPUT, packet, sizeof(packet), false);
}

void handlePacketCommand(int command, const u8* data, u32 dataLength) {
    switch (command) {
        case NIFI_CMD_CLIENT:
            if (isHost && status == HOST_WAITING) {
                u8 ignoredLinkType;
                if (!nifi::decodeIdentity(data, dataLength,
                        &ignoredLinkType, &linkedRomId,
                        linkedFilename, sizeof(linkedFilename),
                        linkedRomTitle, sizeof(linkedRomTitle)))
                    return;
                foundClient = true;
            }
            break;

        case NIFI_CMD_INPUT:
            if (dataLength < 5)
                return;
            {
                const int count = data[0];
                const u32 firstFrame = read32(data + 1);
                if (static_cast<u32>(count) > dataLength - 5)
                    return;
                for (int i=0; i<count; ++i) {
                    const u32 frame = firstFrame + i;
                    if (frame >= static_cast<u32>(mgr_frameCounter) &&
                            frame < static_cast<u32>(mgr_frameCounter +
                                                     INPUT_BUFFER_SIZE)) {
                        const int index = frame & (INPUT_BUFFER_SIZE - 1);
                        receivedInputReady[index] = true;
                        receivedInputFrame[index] = frame;
                        receivedInput[index] = data[5+i];
                    }
                }
            }
            break;

        case NIFI_CMD_INPUT_REQUEST:
            if (dataLength == 4)
                sendInputFrame(read32(data));
            break;

        case NIFI_CMD_STATE_HASH:
            if (dataLength == 8) {
                const u32 frame = read32(data);
                const u32 remoteHash = read32(data + 4);
                StateHashEntry& local = localStateHashes[(frame / 60) & 3];
                if (local.frame == frame && local.hash != remoteHash)
                    printLog("3DS link desync at frame %x (%x != %x)\n",
                             frame, local.hash, remoteHash);
            }
            break;

        case NIFI_CMD_TRANSFER_SRAM:
            {
                u32 expectedSize = 0;
                u8* destination = NULL;
                if (nifiLinkType == LINK_SGB && gameboy) {
                    expectedSize = gameboy->getNumSramBanks() * 0x2000;
                    destination = gameboy->externRam;
                }
                else if (gb2) {
                    expectedSize = gb2->getNumSramBanks() * 0x2000;
                    destination = gb2->externRam;
                }
                if (destination && dataLength == expectedSize) {
                    memcpy(destination, data, expectedSize);
                    receivedSram = true;
                }
            }
            break;

        case NIFI_CMD_FRAGMENT:
            if (dataLength < 0x10)
                return;
            {
                const u32 totalSize = read32(data);
                const u8 innerCommand = data[4];
                const u8 fragmentCount = data[5];
                const u8 fragment = data[6];
                if (!totalSize || totalSize > MAX_REASSEMBLY_SIZE ||
                        !fragmentCount || fragment >= fragmentCount ||
                        totalSize > static_cast<u32>(fragmentCount) *
                            FRAGMENT_SIZE ||
                        totalSize <= static_cast<u32>(fragmentCount - 1) *
                            FRAGMENT_SIZE)
                    return;
                u32 fragmentLength = FRAGMENT_SIZE;
                if (fragment == fragmentCount - 1) {
                    fragmentLength = totalSize % FRAGMENT_SIZE;
                    if (!fragmentLength)
                        fragmentLength = FRAGMENT_SIZE;
                }
                if (fragmentLength + 0x10 != dataLength)
                    return;

                if (fragment == 0) {
                    free(fragmentBuffer);
                    fragmentBuffer = static_cast<u8*>(malloc(totalSize));
                    fragmentTotalSize = totalSize;
                    lastFragment = 0xff;
                }
                if (!fragmentBuffer || totalSize != fragmentTotalSize ||
                        (fragment != 0 && fragment != static_cast<u8>(lastFragment + 1))) {
                    free(fragmentBuffer);
                    fragmentBuffer = NULL;
                    fragmentTotalSize = 0;
                    lastFragment = 0xff;
                    return;
                }
                memcpy(fragmentBuffer + fragment * FRAGMENT_SIZE,
                       data + 0x10, fragmentLength);
                lastFragment = fragment;
                if (fragment == fragmentCount - 1) {
                    handlePacketCommand(innerCommand, fragmentBuffer, totalSize);
                    free(fragmentBuffer);
                    fragmentBuffer = NULL;
                    fragmentTotalSize = 0;
                    lastFragment = 0xff;
                }
            }
            break;
    }
}

void pumpPackets() {
    if (udpSocket < 0)
        return;
    for (int packetNumber=0; packetNumber<16; ++packetNumber) {
        u8 packet[RECEIVE_BUFFER_SIZE];
        sockaddr_in sender;
        socklen_t senderLength = sizeof(sender);
        const int received = recvfrom(udpSocket, packet, sizeof(packet), 0,
            reinterpret_cast<sockaddr*>(&sender), &senderLength);
        if (received <= 0)
            break;

        nifi::PacketView view;
        if (nifi::decodePacket(packet, received, &view) != nifi::DECODE_OK)
            continue;
        if (view.header.command == NIFI_CMD_HOST) {
            if (!isClient || status != CLIENT_WAITING)
                continue;
            u8 receivedLinkType = 0;
            if (!nifi::decodeIdentity(view.payload, view.header.payloadSize,
                    &receivedLinkType, &linkedRomId,
                    linkedFilename, sizeof(linkedFilename),
                    linkedRomTitle, sizeof(linkedRomTitle)) ||
                    receivedLinkType > LINK_SGB)
                continue;
            hostId = view.header.hostId;
            nifiLinkType = receivedLinkType;
            peerAddress = sender;
            peerAddressKnown = true;
            foundHost = true;
            continue;
        }
        if (view.header.hostId != hostId)
            continue;

        if (isHost && status == HOST_WAITING &&
                view.header.command == NIFI_CMD_CLIENT) {
            peerAddress = sender;
            peerAddressKnown = true;
        }
        if (!peerAddressKnown || sender.sin_addr.s_addr !=
                peerAddress.sin_addr.s_addr || sender.sin_port !=
                peerAddress.sin_port)
            continue;

        if (view.header.command == NIFI_CMD_ACKNOWLEDGE) {
            acknowledgedSequence = view.header.ackSequence;
            continue;
        }
        if (view.header.flags & nifi::FLAG_ACK_REQUIRED)
            sendAcknowledge(view.header.sequence);
        handlePacketCommand(view.header.command, view.payload,
                            view.header.payloadSize);
    }
}

void sendSram() {
    if (!gameboy || !gameboy->externRam)
        return;
    nifiSendPacket(NIFI_CMD_TRANSFER_SRAM, gameboy->externRam,
                   gameboy->getNumSramBanks() * 0x2000, true);
}

int receiveSram() {
    for (int frame=0; frame<60*5 && !receivedSram; ++frame) {
        pumpPackets();
        system_waitForVBlank();
    }
    return receivedSram ? 0 : 1;
}

int loadOtherRom() {
    if (nifiLinkType != LINK_CABLE ||
            gameboy->getRomFile()->getContentId() == linkedRomId)
        return 0;
    if (!file_exists(linkedFilename))
        return 1;

    gameboy->getRomFile()->halfMemoryMode();
    if (gb2->getRomFile() && gameboy->getRomFile() != gb2->getRomFile())
        delete gb2->getRomFile();
    RomFile* linkedRom = new RomFile(linkedFilename, true);
    if (!linkedRom || linkedRom->getContentId() != linkedRomId) {
        delete linkedRom;
        return 1;
    }
    gb2->setRomFile(linkedRom);
    if (gb2->loadSave(-1) != 0)
        return 1;
    gb2->init();
    return 0;
}

int startLink() {
    receivedSram = false;
    nifiFrameCounter = -1;
    consecutiveWaitingFrames = 0;
    memset((void*)receivedInputReady, 0, sizeof(receivedInputReady));
    memset(sentInputFrame, 0xff, sizeof(sentInputFrame));
    memset(localStateHashes, 0, sizeof(localStateHashes));
    memset(oldInputs, 0xff, sizeof(oldInputs));

    mgr_reset();
    if (nifiLinkType == LINK_CABLE) {
        if (!mgr_startGb2(-1))
            return 1;
        if (loadOtherRom())
            return 1;
    }

    if (isHost) {
        if (nifiLinkType == LINK_CABLE)
            mgr_setInternalClockGb(gameboy);
        for (int i=0; i<CLIENT_FRAME_LAG; ++i) {
            receivedInputReady[i] = true;
            receivedInputFrame[i] = i;
            receivedInput[i] = 0xff;
        }
        nifiInputDest = &gameboy->controllers[0];
        nifiOtherInputDest = nifiLinkType == LINK_SGB ?
            &gameboy->controllers[1] : &gb2->controllers[0];
    }
    else {
        if (nifiLinkType == LINK_CABLE)
            mgr_setInternalClockGb(gb2);
        nifiInputDest = nifiLinkType == LINK_SGB ?
            &gameboy->controllers[1] : &gameboy->controllers[0];
        nifiOtherInputDest = nifiLinkType == LINK_SGB ?
            &gameboy->controllers[0] : &gb2->controllers[0];
    }

    const bool localHasSram = gameboy->getNumSramBanks() != 0;
    const bool remoteHasSram = nifiLinkType == LINK_CABLE && gb2 &&
        gb2->getNumSramBanks() != 0;
    if (isHost) {
        if (localHasSram)
            sendSram();
        if (remoteHasSram && receiveSram())
            return 1;
    }
    else {
        if ((nifiLinkType == LINK_SGB ? localHasSram : remoteHasSram) &&
                receiveSram())
            return 1;
        if (nifiLinkType == LINK_CABLE && localHasSram)
            sendSram();
    }
    return 0;
}

bool chooseLinkType() {
    int selection = 0;
    for (;;) {
        clearConsole();
        iprintfColored(selection == 0 ? CONSOLE_COLOR_LIGHT_YELLOW :
            CONSOLE_COLOR_WHITE, "%s%s\n\n", selection == 0 ? "* " : "  ",
            tr("Cable Link"));
        iprintfColored(selection == 1 ? CONSOLE_COLOR_LIGHT_YELLOW :
            CONSOLE_COLOR_WHITE, "%s%s\n", selection == 1 ? "* " : "  ",
            tr("SGB Multiplayer"));
        system_waitForVBlank();
        inputUpdateVBlank();
        if (keyJustPressed(mapMenuKey(MENU_KEY_B)))
            return false;
        if (keyJustPressed(mapMenuKey(MENU_KEY_A))) {
            nifiLinkType = selection;
            return true;
        }
        if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_UP) |
                                 mapMenuKey(MENU_KEY_DOWN)))
            selection = !selection;
    }
}

} // namespace

volatile int linkReceivedData;
volatile int linkSendData;
volatile bool transferWaiting;
volatile bool receivedPacket;
volatile int nifiSendid;
bool nifiEnabled = true;

void nifiHostMenu();
void nifiClientMenu();

void releaseNetworkResources() {
    if (udpSocket >= 0)
        closesocket(udpSocket);
    udpSocket = -1;
    if (socInitialized) {
        SOC_Shutdown();
        socInitialized = false;
    }
    free(socBuffer);
    socBuffer = NULL;
    peerAddressKnown = false;
    nifiInitialized = false;
}

void enableNifi() {
    if (nifiInitialized)
        return;
    socBuffer = static_cast<u32*>(memalign(0x1000, SOC_BUFFER_SIZE));
    if (!socBuffer || SOC_Initialize(socBuffer, SOC_BUFFER_SIZE) != 0) {
        releaseNetworkResources();
        printLog("%s\n", tr("Network unavailable."));
        printMenuMessage("Network unavailable.");
        return;
    }
    socInitialized = true;

    const long hostAddress = gethostid();
    if (hostAddress == 0 || hostAddress == -1L) {
        releaseNetworkResources();
        printLog("%s\n", tr("Network unavailable."));
        printMenuMessage("Network unavailable.");
        return;
    }

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        releaseNetworkResources();
        printMenuMessage("Network unavailable.");
        return;
    }
    int enabled = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));
    setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
    fcntl(udpSocket, F_SETFL, O_NONBLOCK);

    sockaddr_in localAddress;
    memset(&localAddress, 0, sizeof(localAddress));
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(LAN_PORT);
    localAddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(udpSocket, reinterpret_cast<sockaddr*>(&localAddress),
             sizeof(localAddress)) < 0) {
        releaseNetworkResources();
        printMenuMessage("Network unavailable.");
        return;
    }

    memset(&broadcastAddress, 0, sizeof(broadcastAddress));
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons(LAN_PORT);
    broadcastAddress.sin_addr.s_addr = INADDR_BROADCAST;
    peerAddressKnown = false;
    nifiInitialized = true;
}

void disableNifi() {
    releaseNetworkResources();
}

int nifiSendPacket(u8 command, u8* data, u32 dataLength, bool acknowledge) {
    if (command == NIFI_CMD_FRAGMENT || dataLength <= FRAGMENT_SIZE)
        return sendSinglePacket(command, data, dataLength, acknowledge, 0xffff);
    const u32 fragmentCount32 =
        (dataLength + FRAGMENT_SIZE - 1) / FRAGMENT_SIZE;
    if (!fragmentCount32 || fragmentCount32 > nifi::MAX_FRAGMENT_COUNT)
        return 1;
    u8 buffer[FRAGMENT_SIZE + 0x10];
    for (u32 fragment=0; fragment<fragmentCount32; ++fragment) {
        u32 fragmentLength = FRAGMENT_SIZE;
        if (fragment == fragmentCount32 - 1) {
            fragmentLength = dataLength % FRAGMENT_SIZE;
            if (!fragmentLength)
                fragmentLength = FRAGMENT_SIZE;
        }
        memset(buffer, 0, 0x10);
        write32(buffer, dataLength);
        buffer[4] = command;
        buffer[5] = fragmentCount32;
        buffer[6] = fragment;
        memcpy(buffer + 0x10, data + fragment * FRAGMENT_SIZE,
               fragmentLength);
        if (sendSinglePacket(NIFI_CMD_FRAGMENT, buffer,
                fragmentLength + 0x10, acknowledge, 0xffff))
            return 1;
        system_waitForVBlank();
    }
    return 0;
}

void nifiInterLinkMenu() {
    if (!gameboy || !gameboy->getRomFile())
        return;
    int selection = 0;
    for (;;) {
        clearConsole();
        iprintfColored(selection == 0 ? CONSOLE_COLOR_LIGHT_YELLOW :
            CONSOLE_COLOR_WHITE, "%s%s\n\n", selection == 0 ? "* " : "  ",
            tr("As Host"));
        iprintfColored(selection == 1 ? CONSOLE_COLOR_LIGHT_YELLOW :
            CONSOLE_COLOR_WHITE, "%s%s\n", selection == 1 ? "* " : "  ",
            tr("As Client"));
        system_waitForVBlank();
        inputUpdateVBlank();
        if (keyJustPressed(mapMenuKey(MENU_KEY_B)))
            return;
        if (keyJustPressed(mapMenuKey(MENU_KEY_A)))
            break;
        if (keyPressedAutoRepeat(mapMenuKey(MENU_KEY_UP) |
                                 mapMenuKey(MENU_KEY_DOWN)))
            selection = !selection;
    }

    isHost = selection == 0;
    isClient = !isHost;
    if (isHost) {
        if (!chooseLinkType()) {
            isHost = false;
            return;
        }
        nifiHostMenu();
    }
    else
        nifiClientMenu();
}

void nifiHostMenu() {
    enableNifi();
    if (!nifiInitialized) {
        isHost = false;
        isClient = false;
        status = HOST_IDLE;
        return;
    }
    clearConsole();
    iprintfColored(CONSOLE_COLOR_WHITE, "%s\n%s\n", tr("Waiting for client..."),
                   tr("Press B to give up."));
    foundClient = false;
    isHost = true;
    isClient = false;
    status = HOST_WAITING;
    peerAddressKnown = false;
    hostId = static_cast<u32>(osGetTime()) ^
             static_cast<u32>(svcGetSystemTick());
    localRomId = 0;
    nextSequence = 1;

    int announceTimer = 0;
    nifi::FrameDeadline discoveryDeadline(DISCOVERY_TIMEOUT_FRAMES);
    while (!foundClient && !discoveryDeadline.expired()) {
        pumpPackets();
        if (announceTimer-- <= 0) {
            u8 identity[MAX_FILENAME_LEN + 64];
            const size_t length = nifi::encodeIdentity(identity,
                sizeof(identity), nifiLinkType, getLocalRomId(),
                gameboy->getRomFile()->getFilename(),
                gameboy->getRomFile()->getRomTitle());
            if (length)
                nifiSendPacket(NIFI_CMD_HOST, identity, length, false);
            announceTimer = 15;
        }
        system_waitForVBlank();
        inputUpdateVBlank();
        if (keyJustPressed(mapMenuKey(MENU_KEY_B)))
            break;
        discoveryDeadline.advance();
    }
    if (!foundClient) {
        printMenuMessage("Couldn't find client.");
        nifiStop();
        return;
    }
    status = HOST_CONNECTED;
    if (startLink()) {
        printLog("%s\n", tr("Link failed."));
        nifiStop();
    }
}

void nifiClientMenu() {
    enableNifi();
    if (!nifiInitialized) {
        isHost = false;
        isClient = false;
        status = CLIENT_IDLE;
        return;
    }
    clearConsole();
    iprintfColored(CONSOLE_COLOR_WHITE, "%s\n%s\n", tr("Waiting for host..."),
                   tr("Press B to give up."));
    foundHost = false;
    isClient = true;
    isHost = false;
    status = CLIENT_WAITING;
    peerAddressKnown = false;
    localRomId = 0;
    nextSequence = 1;

    nifi::FrameDeadline discoveryDeadline(DISCOVERY_TIMEOUT_FRAMES);
    while (!foundHost && !discoveryDeadline.expired()) {
        pumpPackets();
        system_waitForVBlank();
        inputUpdateVBlank();
        if (keyJustPressed(mapMenuKey(MENU_KEY_B)))
            break;
        discoveryDeadline.advance();
    }
    if (!foundHost) {
        printMenuMessage("Couldn't find host.");
        nifiStop();
        return;
    }

    clearConsole();
    iprintfColored(CONSOLE_COLOR_WHITE, "%s: %s\n%s\n",
                   tr("Host ROM:"), linkedRomTitle,
                   tr("Press A to connect, B to cancel."));
    for (;;) {
        pumpPackets();
        system_waitForVBlank();
        inputUpdateVBlank();
        if (keyJustPressed(mapMenuKey(MENU_KEY_B))) {
            nifiStop();
            return;
        }
        if (keyJustPressed(mapMenuKey(MENU_KEY_A)))
            break;
    }

    u8 identity[MAX_FILENAME_LEN + 64];
    const size_t length = nifi::encodeIdentity(identity, sizeof(identity),
        nifiLinkType, getLocalRomId(), gameboy->getRomFile()->getFilename(),
        gameboy->getRomFile()->getRomTitle());
    if (!length || nifiSendPacket(NIFI_CMD_CLIENT, identity, length, true)) {
        nifiStop();
        return;
    }
    status = CLIENT_CONNECTED;
    if (startLink()) {
        printLog("%s\n", tr("Link failed."));
        nifiStop();
    }
}

void nifiStop() {
    isHost = false;
    isClient = false;
    status = 0;
    free(fragmentBuffer);
    fragmentBuffer = NULL;
    fragmentTotalSize = 0;
    lastFragment = 0xff;
    disableNifi();
    nifiUnpause();
}

bool nifiIsHost() { return isHost && status == HOST_CONNECTED; }
bool nifiIsClient() { return isClient && status == CLIENT_CONNECTED; }
bool nifiIsLinked() { return nifiIsHost() || nifiIsClient(); }

void nifiPause() {
    if (nifiWasPaused == -1)
        nifiWasPaused = mgr_isPaused();
    mgr_pause();
}

void nifiUnpause() {
    if (nifiWasPaused == -1)
        return;
    if (!nifiWasPaused)
        mgr_unpause();
    nifiWasPaused = -1;
}

void nifiUpdateInput() {
    pumpPackets();
    u8* inputDestination = nifiIsLinked() ?
        nifiInputDest : &gameboy->controllers[0];
    const u32 actualFrame = mgr_frameCounter;
    u32 inputFrame = actualFrame;
    const bool framePassed = nifiFrameCounter != mgr_frameCounter;
    if (framePassed && nifiFrameCounter > 0)
        receivedInputReady[(nifiFrameCounter - 1) &
                           (INPUT_BUFFER_SIZE - 1)] = false;
    nifiFrameCounter = mgr_frameCounter;
    if (nifiIsClient())
        inputFrame += CLIENT_FRAME_LAG;
    const u8 olderInput = oldInputs[OLD_INPUTS_BUFFER_SIZE - CLIENT_FRAME_LAG];

    if (nifiIsLinked()) {
        if (framePassed) {
            memmove(oldInputs, oldInputs + 1, OLD_INPUTS_BUFFER_SIZE - 1);
            oldInputs[OLD_INPUTS_BUFFER_SIZE - 1] = buttonsPressed;
        }
        const int sentIndex = inputFrame & (INPUT_BUFFER_SIZE - 1);
        sentInputFrame[sentIndex] = inputFrame;
        sentInput[sentIndex] = buttonsPressed;

        u8 packet[5 + OLD_INPUTS_BUFFER_SIZE];
        packet[0] = OLD_INPUTS_BUFFER_SIZE;
        write32(packet + 1, inputFrame - OLD_INPUTS_BUFFER_SIZE + 1);
        memcpy(packet + 5, oldInputs, OLD_INPUTS_BUFFER_SIZE);
        nifiSendPacket(NIFI_CMD_INPUT, packet, sizeof(packet), false);

        const int receiveIndex = actualFrame & (INPUT_BUFFER_SIZE - 1);
        if (receivedInputReady[receiveIndex] &&
                receivedInputFrame[receiveIndex] == actualFrame) {
            *nifiOtherInputDest = receivedInput[receiveIndex];
            consecutiveWaitingFrames = 0;
            nifiUnpause();
        }
        else {
            ++consecutiveWaitingFrames;
            if (consecutiveWaitingFrames == 1 ||
                    consecutiveWaitingFrames % 10 == 0) {
                u8 request[4];
                write32(request, actualFrame);
                nifiSendPacket(NIFI_CMD_INPUT_REQUEST, request,
                               sizeof(request), false);
            }
            nifiPause();
        }
        if (consecutiveWaitingFrames >= 120) {
            printLog("%s\n", tr("Connection lost."));
            nifiStop();
        }

        if (framePassed && actualFrame % 60 == 0) {
            u8 hashPacket[8];
            StateHashEntry& entry = localStateHashes[(actualFrame / 60) & 3];
            entry.frame = actualFrame;
            entry.hash = stateHash();
            write32(hashPacket, entry.frame);
            write32(hashPacket + 4, entry.hash);
            nifiSendPacket(NIFI_CMD_STATE_HASH, hashPacket,
                           sizeof(hashPacket), false);
        }
    }

    if (!nifiIsLinked() || nifiIsHost())
        *inputDestination = buttonsPressed;
    else
        *inputDestination = olderInput;
}
