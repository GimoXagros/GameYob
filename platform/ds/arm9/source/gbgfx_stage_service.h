#pragma once

#include <stdint.h>

struct GbStageCopyMeasurement {
    uint32_t startHostFrame;
    uint32_t endHostFrame;
    uint16_t startVcount;
    uint16_t endVcount;
    uint32_t bytesCopied;
};

// Diagnostic-only, foreground pre-ROM call while game graphics are disabled.
// Copies identical bytes through the complete guest tile/map/OBJ VRAM regions
// and reports one trial. Caller performs bounded repetitions and exports.
bool measureGbStageFullCopy(GbStageCopyMeasurement* output);

// Foreground-only. Safe to call after each guest frame; never from an IRQ.
// In ordinary builds this is a no-op. The experimental staged renderer uses
// it to publish one complete ready frame when the host display is safe.
void servicePendingVideoFrameCommit();
