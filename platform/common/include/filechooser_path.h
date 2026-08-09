#pragma once

#include <stddef.h>

// Returns true when the current path has a parent directory that can be
// entered. When true, parentPath receives an absolute parent path and
// matchName receives the final component so the chooser can select the
// directory that was just left.
bool fileChooserParentInfo(const char* currentPath,
                           char* parentPath, size_t parentPathCapacity,
                           char* matchName, size_t matchNameCapacity);
