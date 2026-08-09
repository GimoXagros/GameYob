#pragma once

#include <stddef.h>

// Returns true when the current path has a parent directory that can be
// entered. When true, matchName receives the final path component so the file
// chooser can select the directory that was just left.
bool fileChooserParentMatch(const char* currentPath, char* matchName,
                            size_t matchNameCapacity);
