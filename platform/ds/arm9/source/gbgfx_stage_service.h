#pragma once

#include <stdint.h>

struct GbStageCopyMeasurement {
    uint32_t startHostFrame;
    uint32_t endHostFrame;
    uint16_t startVcount;
    uint16_t endVcount;
    uint32_t bytesCopied;
};

enum { GB_STAGE_CALIBRATION_TRIALS = 16 };
enum GbStageFaultCode {
    GB_STAGE_FAULT_NONE = 0,
    GB_STAGE_FAULT_DEADLINE = 1,
    GB_STAGE_FAULT_NO_COMMIT_OPPORTUNITY = 2,
    GB_STAGE_FAULT_DMA_CONFLICT = 3
};

enum GbStageCopyBackend {
    GB_STAGE_COPY_NONE = 0,
    GB_STAGE_COPY_DMA3_WORDS = 1
};

struct GbStageCalibration {
    GbStageCopyMeasurement trials[GB_STAGE_CALIBRATION_TRIALS];
    uint16_t completedTrials;
    uint16_t maxObservedLines;
    uint16_t admittedBoundLines;
    uint8_t eligible;
};

struct GbStageRuntimeStatus {
    uint32_t stagedEntries;
    uint32_t presentedFrames;
    uint32_t deferredForCallbacks;
    uint32_t lastEarlyPollHostFrame;
    uint16_t lastCopyEndVcount;
    uint8_t calibrationEligible;
    uint8_t faultCode;
    uint8_t copyBackend;
};

// Diagnostic-only, foreground pre-ROM call while game graphics are disabled.
// Copies identical bytes through the complete guest tile/map/OBJ VRAM regions
// and reports one trial. Caller performs bounded repetitions and exports.
bool measureGbStageFullCopy(GbStageCopyMeasurement* output);

// Same trial beginning at a requested host line (168 or 192). Line 168
// includes the fixed VBlank IRQ in the measured duration. Returns false if
// a VBlank callback is pending or the pre-ROM/graphics-disabled guard fails.
bool measureGbStageFullCopyAtLine(unsigned startLine,
                                  GbStageCopyMeasurement* output);

// Same-boot, pre-ROM foreground calibration. An ineligible result leaves the
// ordinary renderer active. It never starts staged presentation on its own.
bool calibrateGbStagedVideo(GbStageCalibration* output);

// Foreground status for integration's normal error/reporting path.
GbStageRuntimeStatus getGbStageRuntimeStatus();

// Foreground-only. Safe to call after each guest frame; never from an IRQ.
// In ordinary builds this is a no-op. The experimental staged renderer uses
// it to publish one complete ready frame when the host display is safe.
void servicePendingVideoFrameCommit();
