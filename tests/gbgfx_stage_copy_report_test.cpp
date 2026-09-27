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

    GbStageCalibration calibration = {};
    calibration.trials[0] = trial;
    calibration.completedTrials = 1;
    calibration.maxObservedLines = 46;
    calibration.admittedBoundLines = 62;
    calibration.eligible = 1;
    char calibrationPath[256];
    assert(writeGbStageCalibrationReport(argv[1], "abc123def456", true,
                                         &calibration, calibrationPath,
                                         sizeof(calibrationPath)));
    const std::string eligibleReport = readAll(calibrationPath);
    assert(eligibleReport.find("result,eligible") != std::string::npos);
    assert(eligibleReport.find("active_compiled,1") != std::string::npos);
    assert(eligibleReport.find("copy_backend,dma3_words") != std::string::npos);
    assert(eligibleReport.find("max_observed_lines,46") != std::string::npos);
    assert(eligibleReport.find("admitted_bound_lines,62") != std::string::npos);
    assert(eligibleReport.find("0,94208,42,42,193,214,1") !=
           std::string::npos);
    calibration.eligible = 0;
    calibration.admittedBoundLines = 0;
    char secondPath[256];
    assert(writeGbStageCalibrationReport(argv[1], "abc123def456", false,
                                         &calibration, secondPath,
                                         sizeof(secondPath)));
    assert(strcmp(calibrationPath, secondPath) != 0);
    assert(readAll(secondPath).find("result,aborted") != std::string::npos);
    assert(readAll(calibrationPath) == eligibleReport);
    calibration.completedTrials = GB_STAGE_CALIBRATION_TRIALS + 1;
    assert(!writeGbStageCalibrationReport(argv[1], "abc", true,
                                          &calibration, secondPath,
                                          sizeof(secondPath)));

    GbStageRuntimeStatus status = {};
    status.copyBackend = GB_STAGE_COPY_DMA3_WORDS;
    status.calibrationEligible = 1;
    status.stagedEntries = 1;
    status.presentedFrames = 3;
    status.deferredForCallbacks = 5;
    status.lastEarlyPollHostFrame = 123;
    status.lastCopyEndVcount = 211;
    status.faultCode = GB_STAGE_FAULT_DMA_CONFLICT;
    char statusPath[256];
    assert(writeGbStageRuntimeStatusReport(argv[1], "abc123def456", &status,
                                           statusPath, sizeof(statusPath)));
    const std::string statusReport = readAll(statusPath);
    assert(statusReport.find("capture_context,paused_menu") !=
           std::string::npos);
    assert(statusReport.find("copy_backend,1") != std::string::npos);
    assert(statusReport.find("calibration_eligible,1") != std::string::npos);
    assert(statusReport.find("staged_entries,1") != std::string::npos);
    assert(statusReport.find("presented_frames,3") != std::string::npos);
    assert(statusReport.find("fault_code,3") != std::string::npos);
    assert(statusReport.find("last_copy_end_vcount,211") !=
           std::string::npos);
    char nextStatusPath[256];
    assert(writeGbStageRuntimeStatusReport(argv[1], "abc123def456", &status,
                                           nextStatusPath,
                                           sizeof(nextStatusPath)));
    assert(strcmp(statusPath, nextStatusPath) != 0);
    assert(readAll(statusPath) == statusReport);
    assert(!writeGbStageRuntimeStatusReport(argv[1], "abc", &status,
                                            tooSmall, sizeof(tooSmall)));
    assert(tooSmall[0] == 0);
    return 0;
}
