#pragma once

#include <stddef.h>

void textResetGlyphCache();
bool textEquivalent(const char* first, const char* second);
unsigned int textColumns(const char* text);
void textCopyColumns(char* destination, size_t destinationSize,
                     const char* source, unsigned int maxColumns);
void textPrintColored(int palette, const char* text, int maxColumns = -1);
