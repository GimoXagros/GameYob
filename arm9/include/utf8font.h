#pragma once

#include <stddef.h>

// Clears the dynamic Hangul tile cache. Call after clearing a full console page.
void utf8FontResetCache();

// Counts display cells without counting UTF-8 or CP949 continuation bytes.
unsigned int utf8TextColumns(const char* text);

// Copies at most maxColumns complete UTF-8/CP949 characters.
void utf8CopyText(char* destination, size_t destinationSize,
                  const char* source, unsigned int maxColumns);

// Prints UTF-8, or CP949 when the byte string is not valid UTF-8.
void utf8PrintColored(int palette, const char* text, int maxColumns = -1);
