#include "hblank_anomaly_report.h"
#include <stdio.h>
#include <string.h>

bool writeHBlankAnomalyReport(const char* directory, const char* sourceRevision,
                              uint32_t releaseGuestFrame,
                              uint32_t releaseHostFrame,
                              uint16_t releaseVcount,
                              uint32_t overwritten,
                              const HBlankAnomalyEvent* events,
                              unsigned eventCount,
                              char* writtenPath, size_t writtenPathSize) {
    if (!directory || !sourceRevision || !events || eventCount > 64 ||
            !writtenPath || !writtenPathSize)
        return false;
    writtenPath[0] = 0;
    for (unsigned number = 0; number < 100; number++) {
        char path[256];
        const int length = snprintf(path, sizeof(path),
                                    "%s/gameyob_hblank_ff_%02u.csv",
                                    directory, number);
        if (length < 0 || (size_t)length >= sizeof(path) ||
                (size_t)length >= writtenPathSize)
            return false;
        FILE* output = fopen(path, "wx");
        if (!output)
            continue;
        bool ok = fprintf(output,
            "source_revision,%s\nprofile,hblank_anomaly_ff_release\n"
            "capture_context,paused_menu_after_l_release\n"
            "release_guest_frame,%lu\nrelease_host_frame,%lu\n"
            "release_vcount,%u\nevent_count,%u\noverwritten,%lu\n"
            "index,host_frame,guest_frame,published_guest_frame,"
            "entry_vcount,exit_vcount,requested_physical_line,guest_line,"
            "dispstat,flags,fast_forward,drawing_buffer,modified,"
            "maps_modified,bg_palettes_modified,spr_palettes_modified,"
            "sprites_modified,screen_disabled,bg0cnt,win_in,bg_palette0,"
            "sprite_palette0,first_guest_obj_attr0\n",
            sourceRevision, (unsigned long)releaseGuestFrame,
            (unsigned long)releaseHostFrame, (unsigned)releaseVcount,
            eventCount, (unsigned long)overwritten) > 0;
        for (unsigned index = 0; index < eventCount && ok; index++) {
            const HBlankAnomalyEvent& event = events[index];
            ok = fprintf(output,
                "%u,%lu,%lu,%lu,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                index, (unsigned long)event.hostFrame,
                (unsigned long)event.guestFrame,
                (unsigned long)event.publishedGuestFrame,
                (unsigned)event.entryVcount, (unsigned)event.exitVcount,
                (unsigned)event.requestedPhysicalLine, (int)event.guestLine,
                (unsigned)event.dispstat, (unsigned)event.flags,
                (unsigned)event.fastForward, (unsigned)event.drawingBuffer,
                (unsigned)event.modified, (unsigned)event.mapsModified,
                (unsigned)event.bgPalettesModified,
                (unsigned)event.sprPalettesModified,
                (unsigned)event.spritesModified,
                (unsigned)event.screenDisabled, (unsigned)event.bg0cnt,
                (unsigned)event.winIn, (unsigned)event.bgPalette0,
                (unsigned)event.spritePalette0,
                (unsigned)event.firstGuestObjAttr0) > 0;
        }
        if (fclose(output) != 0)
            ok = false;
        if (!ok) {
            remove(path); // Only this call's exclusively created file.
            return false;
        }
        memcpy(writtenPath, path, (size_t)length + 1);
        return true;
    }
    return false;
}
