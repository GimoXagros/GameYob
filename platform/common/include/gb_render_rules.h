#pragma once

#include <stdint.h>

namespace gb_render {

struct WindowSpan {
    bool visible;
    int screenStart;
    int sourceStart;
    int width;
};

struct SpriteRef {
    uint8_t index;
    int x;
};

WindowSpan windowSpan(uint8_t wx, uint8_t wy, int scanline,
        bool windowEnabled);

int nextWindowLine(int currentLine, bool resetPending, uint8_t ly,
        uint8_t wy, uint8_t wx, bool windowEnabled);
void sortSpritesForDrawing(SpriteRef* sprites, int count, bool dmgMode);
bool cgbSpriteWins(bool bgPriorityEnabled, bool bgTilePriority,
        uint8_t bgColor, bool spriteBehindBg);

} // namespace gb_render
