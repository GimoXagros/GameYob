#pragma once

#include <stddef.h>
#include "gbgfx_stage_service.h"

// Writes one complete report with exclusive creation; never replaces a file.
bool writeGbStageCopyReport(const char* directory, const char* sourceRevision,
                            const GbStageCopyMeasurement* trials,
                            unsigned trialCount, char* writtenPath,
                            size_t writtenPathSize);

bool writeGbStageCalibrationReport(const char* directory,
                                   const char* sourceRevision,
                                   bool calibrationCompleted,
                                   const GbStageCalibration* calibration,
                                   char* writtenPath, size_t writtenPathSize);
