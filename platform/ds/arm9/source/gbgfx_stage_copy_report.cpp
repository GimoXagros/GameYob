#include "gbgfx_stage_copy_report.h"
#include "gbgfx_stage.h"
#include <stdio.h>
#include <string.h>

bool writeGbStageCopyReport(const char* directory, const char* sourceRevision,
                            const GbStageCopyMeasurement* trials,
                            unsigned trialCount, char* writtenPath,
                            size_t writtenPathSize) {
    if (!directory || !sourceRevision || !trials || !trialCount ||
            !writtenPath || !writtenPathSize)
        return false;
    writtenPath[0] = 0;
    for (unsigned number = 0; number < 100; number++) {
        char path[256];
        const int length = snprintf(path, sizeof(path),
                                    "%s/gameyob_stage_copy_%02u.csv",
                                    directory, number);
        if (length < 0 || (size_t)length >= sizeof(path) ||
                (size_t)length >= writtenPathSize)
            return false;
        FILE* output = fopen(path, "wx");
        if (!output)
            continue;
        bool ok = fprintf(output,
            "source_revision,%s\nprofile,stage_copy_diagnostic\n"
            "experiment,1\nactive,0\nexpected_payload_bytes,%u\n"
            "requested_trials,%u\n"
            "trial,bytes_copied,start_host_frame,end_host_frame,"
            "start_vcount,end_vcount,same_host_frame\n",
            sourceRevision, (unsigned)GB_GFX_STAGE_BYTES, trialCount) > 0;
        for (unsigned index = 0; index < trialCount && ok; index++) {
            const GbStageCopyMeasurement& value = trials[index];
            ok = fprintf(output, "%u,%lu,%lu,%lu,%u,%u,%u\n",
                         index, (unsigned long)value.bytesCopied,
                         (unsigned long)value.startHostFrame,
                         (unsigned long)value.endHostFrame,
                         value.startVcount, value.endVcount,
                         value.startHostFrame == value.endHostFrame) > 0;
        }
        if (fclose(output) != 0)
            ok = false;
        if (!ok) {
            remove(path); // Only the file this call created exclusively.
            return false;
        }
        memcpy(writtenPath, path, (size_t)length + 1);
        return true;
    }
    return false;
}
