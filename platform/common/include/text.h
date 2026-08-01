#pragma once

#include <stddef.h>

void textResetGlyphCache();
unsigned int textColumns(const char* text);
void textCopyColumns(char* destination, size_t destinationSize,
                     const char* source, unsigned int maxColumns);
void textPrintColored(int palette, const char* text, int maxColumns = -1);
