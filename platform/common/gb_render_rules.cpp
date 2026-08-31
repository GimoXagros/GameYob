#include "gb_render_rules.h"

namespace gb_render {

WindowSpan windowSpan(uint8_t wx, uint8_t wy, int scanline,
        bool windowEnabled) {
    WindowSpan span;
    span.visible = false;
    span.screenStart = 0;
    span.sourceStart = 0;
    span.width = 0;

    if (!windowEnabled || wy >= 144 || scanline < wy || wx >= 167)
        return span;

    const int left = (int)wx - 7;
    span.screenStart = left < 0 ? 0 : left;
    span.sourceStart = left < 0 ? -left : 0;
    span.width = 160 - span.screenStart;
    span.visible = span.width > 0;
    return span;
}

int nextWindowLine(int currentLine, bool resetPending, uint8_t ly,
        uint8_t wy, uint8_t wx, bool windowEnabled) {
    if (resetPending)
        return (int)ly - (int)wy;
    if (windowEnabled && wx < 167 && wy <= ly)
        return currentLine + 1;
    return currentLine;
}

void sortSpritesForDrawing(SpriteRef* sprites, int count, bool dmgMode) {
    if (!sprites || count < 2)
        return;
    for (int i = 1; i < count; ++i) {
        SpriteRef value = sprites[i];
        int position = i;
        while (position > 0) {
            const SpriteRef previous = sprites[position - 1];
            bool valueDrawsEarlier;
            if (dmgMode) {
                valueDrawsEarlier = value.x > previous.x ||
                    (value.x == previous.x && value.index > previous.index);
            }
            else {
                valueDrawsEarlier = value.index > previous.index;
            }
            if (!valueDrawsEarlier)
                break;
            sprites[position] = previous;
            --position;
        }
        sprites[position] = value;
    }
}

bool cgbSpriteWins(bool bgPriorityEnabled, bool bgTilePriority,
        uint8_t bgColor, bool spriteBehindBg) {
    if (!bgPriorityEnabled)
        return true;
    if (!bgColor)
        return true;
    return !bgTilePriority && !spriteBehindBg;
}

} // namespace gb_render
