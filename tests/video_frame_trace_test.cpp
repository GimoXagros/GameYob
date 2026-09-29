#include <assert.h>
#include "../platform/ds/arm9/source/video_frame_trace.h"

static VideoFrameEvent line(unsigned host, unsigned guest,
                            unsigned physicalLine) {
    VideoFrameEvent event = {};
    event.hostFrame = host;
    event.publishedFrame = guest;
    event.physicalLine = physicalLine;
    event.type = VIDEO_HOST_LINE;
    return event;
}

int main() {
    VideoFrameEvent publication = {};
    publication.type = VIDEO_PUBLISH;
    publication.hostFrame = 7;
    publication.guestFrame = 12;
    publication.physicalLine = 81;
    publication.fastForward = 1;
    publication.gbMode = 0;
    publication.sgbMode = 1;
    publication.gfxMask = 2;
    publication.tileQueueLength = 0x300;
    publication.mapQueueLength = 0x800;
    VideoFrameTraceRing<4> publicationTrace;
    publicationTrace.record(publication);
    publication.type = VIDEO_UPLOAD_END;
    publication.physicalLine = 86;
    publication.tileQueueLength = 0;
    publication.mapQueueLength = 0;
    publicationTrace.record(publication);
    VideoFrameEvent uploadSamples[2] = {};
    assert(publicationTrace.pop(&uploadSamples[0]));
    assert(publicationTrace.pop(&uploadSamples[1]));
    assert(uploadSamples[0].type == VIDEO_PUBLISH);
    assert(uploadSamples[1].type == VIDEO_UPLOAD_END);
    assert(uploadSamples[0].hostFrame == uploadSamples[1].hostFrame);
    assert(uploadSamples[0].guestFrame == uploadSamples[1].guestFrame);
    assert(uploadSamples[0].physicalLine == 81);
    assert(uploadSamples[1].physicalLine == 86);
    assert(uploadSamples[0].fastForward == 1 && uploadSamples[1].fastForward == 1);
    assert(uploadSamples[0].gbMode == 0 && uploadSamples[0].sgbMode == 1);
    assert(uploadSamples[0].gfxMask == 2);
    assert(uploadSamples[0].tileQueueLength == 0x300);
    assert(uploadSamples[0].mapQueueLength == 0x800);
    assert(uploadSamples[1].tileQueueLength == 0);
    assert(uploadSamples[1].mapQueueLength == 0);

    VideoFrameTraceRing<4> trace;
    trace.record(line(10, 5, 24));
    trace.record(line(10, 5, 72));
    trace.record(line(11, 6, 24));
    VideoFrameEvent samples[4] = {};
    for (unsigned i = 0; i < 3; ++i)
        assert(trace.pop(&samples[i]));
    assert(!videoTraceHasMixedFrame(samples, 3));
    assert(!trace.pop(&samples[3]));

    // Reproduce the source-level risk: the guest publishes during physical
    // scanout after the host has already displayed earlier lines.
    trace.record(line(12, 6, 24));
    trace.record(line(12, 7, 120));
    assert(trace.pop(&samples[0]));
    assert(trace.pop(&samples[1]));
    assert(videoTraceHasMixedFrame(samples, 2));

    for (unsigned i = 0; i < 5; ++i)
        trace.record(line(20, i, i * 8));
    assert(trace.overwritten() == 1);
    for (unsigned i = 1; i < 5; ++i) {
        assert(trace.pop(&samples[i - 1]));
        assert(samples[i - 1].publishedFrame == i);
    }
    // Opening the diagnostic menu must preserve the pre-menu evidence even
    // if rendering continues while the user navigates to Export.
    trace.record(line(30, 8, 24));
    trace.freeze();
    for (unsigned i = 0; i < 32; ++i)
        trace.record(line(31 + i, 9, 72));
    assert(trace.overwritten() == 1);
    assert(trace.pop(&samples[0]));
    assert(samples[0].hostFrame == 30);
    assert(!trace.pop(&samples[0]));
    trace.resume();
    trace.record(line(70, 10, 120));
    assert(trace.pop(&samples[0]));
    assert(samples[0].hostFrame == 70);
    return 0;
}
