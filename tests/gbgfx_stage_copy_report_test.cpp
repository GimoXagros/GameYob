#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "../platform/ds/arm9/source/gbgfx_stage_copy_report.h"

static std::string readAll(const char* path) {
    FILE* file = fopen(path, "rb");
    assert(file);
    std::string result;
    char buffer[256];
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), file)) != 0)
        result.append(buffer, count);
    assert(fclose(file) == 0);
    return result;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    char existing[256];
    assert(snprintf(existing, sizeof(existing), "%s/gameyob_stage_copy_00.csv",
                    argv[1]) < (int)sizeof(existing));
    FILE* sentinel = fopen(existing, "wb");
    assert(sentinel);
    assert(fputs("keep me", sentinel) >= 0);
    assert(fclose(sentinel) == 0);

    const GbStageCopyMeasurement trial = {42, 42, 193, 214, 94208};
    char path[256];
    assert(writeGbStageCopyReport(argv[1], "abc123def456", &trial, 1,
                                  path, sizeof(path)));
    assert(strstr(path, "gameyob_stage_copy_01.csv"));
    assert(readAll(existing) == "keep me");
    const std::string report = readAll(path);
    assert(report.find("source_revision,abc123def456") != std::string::npos);
    assert(report.find("experiment,1") != std::string::npos);
    assert(report.find("active,0") != std::string::npos);
    assert(report.find("expected_payload_bytes,94208") != std::string::npos);
    assert(report.find("0,94208,42,42,193,214,1") != std::string::npos);

    char tooSmall[4];
    assert(!writeGbStageCopyReport(argv[1], "abc", &trial, 1,
                                   tooSmall, sizeof(tooSmall)));
    assert(tooSmall[0] == 0);
    assert(readAll(existing) == "keep me");
    return 0;
}
