#include <stdio.h>
#include "console.h"
#include "nifi_protocol.h"

#ifndef GIT_REVISION
#define GIT_REVISION "unknown"
#endif

#ifndef GAMEYOB_VERSION
#define GAMEYOB_VERSION "v0.5.10"
#endif

#ifdef DS
extern bool __dsimode;
#endif

void printVersionInfo() {
    clearConsole();
    printf("GameYob %s\n", GAMEYOB_VERSION);
    printf("Revision: %s\n", GIT_REVISION);
#ifdef DS
    printf("Target: %s\n", __dsimode ? "Nintendo DSi mode" :
            "Nintendo DS mode");
    printf("Framework: BlocksDS 1.22.2\n");
#elif defined(_3DS)
    printf("Target: Nintendo 3DS (native)\n");
    printf("Framework: libctru/devkitARM\n");
#elif defined(SDL)
    printf("Target: SDL test build\n");
#else
    printf("Target: unknown\n");
#endif
    printf("Link protocol: v%d\n", nifi::PROTOCOL_VERSION);
}
