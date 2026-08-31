#include "gb_render_rules.h"

#include <assert.h>

static void testWindowClipping() {
    gb_render::WindowSpan span = gb_render::windowSpan(7, 20, 20, true);
    assert(span.visible && span.screenStart == 0 && span.sourceStart == 0);

    span = gb_render::windowSpan(0, 20, 20, true);
    assert(span.visible && span.screenStart == 0 && span.sourceStart == 7);

    span = gb_render::windowSpan(166, 20, 20, true);
    assert(span.visible && span.screenStart == 159 && span.width == 1);

    assert(!gb_render::windowSpan(167, 20, 20, true).visible);
    assert(!gb_render::windowSpan(7, 144, 144, true).visible);

    // WX changes are evaluated per scanline. Hidden lines do not consume the
    // window's internal Y counter; making WX visible resumes from that line.
    int line = -1;
    line = gb_render::nextWindowLine(line, false, 20, 20, 167, true);
    assert(line == -1);
    line = gb_render::nextWindowLine(line, false, 21, 20, 166, true);
    assert(line == 0);
    line = gb_render::nextWindowLine(line, false, 22, 20, 0, true);
    assert(line == 1);
    assert(gb_render::nextWindowLine(line, false, 23, 20, 7, false) == 1);
    assert(gb_render::nextWindowLine(-2, true, 40, 35, 7, true) == 5);
    assert(gb_render::nextWindowLine(-1, false, 19, 20, 7, true) == -1);
}

static void testSpriteOrder() {
    gb_render::SpriteRef sprites[] = {{0, 20}, {1, 10}, {2, 10}, {3, 30}};
    gb_render::sortSpritesForDrawing(sprites, 4, true);
    assert(sprites[0].index == 3);
    assert(sprites[1].index == 0);
    assert(sprites[2].index == 2);
    assert(sprites[3].index == 1);

    gb_render::sortSpritesForDrawing(sprites, 4, false);
    assert(sprites[0].index == 3);
    assert(sprites[3].index == 0);
}

static void testCgbPriorityCancellation() {
    assert(gb_render::cgbSpriteWins(false, true, 3, true));
    assert(gb_render::cgbSpriteWins(true, true, 0, true));
    assert(!gb_render::cgbSpriteWins(true, true, 2, false));
    assert(!gb_render::cgbSpriteWins(true, false, 2, true));
    assert(gb_render::cgbSpriteWins(true, false, 2, false));
}

int main() {
    testWindowClipping();
    testSpriteOrder();
    testCgbPriorityCancellation();
    return 0;
}
