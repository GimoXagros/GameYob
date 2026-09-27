#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "../platform/ds/arm9/source/hblank_anomaly_report.h"

static std::string readAll(const char* path) {
    FILE* input = fopen(path, "rb");
    assert(input);
    std::string result;
    char buffer[256];
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), input)) != 0)
        result.append(buffer, count);
    assert(fclose(input) == 0);
    return result;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    char existing[256];
    assert(snprintf(existing, sizeof(existing),
                    "%s/gameyob_hblank_ff_00.csv", argv[1]) <
           (int)sizeof(existing));
    FILE* sentinel = fopen(existing, "wb");
    assert(sentinel);
    assert(fputs("keep me", sentinel) >= 0);
    assert(fclose(sentinel) == 0);
    HBlankAnomalyEvent event = {};
    event.hostFrame = 42;
    event.guestFrame = 75;
    event.publishedGuestFrame = 74;
    event.entryVcount = 48;
    event.exitVcount = 49;
    event.requestedPhysicalLine = 49;
    event.guestLine = 25;
    event.flags = HBLANK_CROSSED_LINE;
    event.fastForward = 1;
    char path[256];
    assert(writeHBlankAnomalyReport(argv[1], "abc123def456", 75, 42, 200,
                                    2, &event, 1, path, sizeof(path)));
    assert(strstr(path, "gameyob_hblank_ff_01.csv"));
    assert(readAll(existing) == "keep me");
    const std::string report = readAll(path);
    assert(report.find("release_guest_frame,75") != std::string::npos);
    assert(report.find("event_count,1") != std::string::npos);
    assert(report.find("overwritten,2") != std::string::npos);
    assert(report.find("0,42,75,74,48,49,49,25,") != std::string::npos);
    char zeroPath[256];
    assert(writeHBlankAnomalyReport(argv[1], "abc123def456", 76, 43, 201,
                                    0, &event, 0, zeroPath, sizeof(zeroPath)));
    assert(readAll(zeroPath).find("event_count,0") != std::string::npos);
    assert(readAll(path) == report);
    char tooSmall[4];
    assert(!writeHBlankAnomalyReport(argv[1], "abc", 1, 2, 3, 0, &event, 1,
                                     tooSmall, sizeof(tooSmall)));
    assert(tooSmall[0] == 0);
    assert(readAll(existing) == "keep me");
    return 0;
}
