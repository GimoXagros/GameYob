#pragma once

#include <stddef.h>
#include <stdint.h>
#include "hblank_anomaly_trace.h"

// Called only in foreground with the guest paused in the menu. The supplied
// events were already copied from the frozen IRQ ring on L release.
bool writeHBlankAnomalyReport(const char* directory, const char* sourceRevision,
                              uint32_t releaseGuestFrame,
                              uint32_t releaseHostFrame,
                              uint16_t releaseVcount,
                              uint32_t overwritten,
                              const HBlankAnomalyEvent* events,
                              unsigned eventCount,
                              char* writtenPath, size_t writtenPathSize);
