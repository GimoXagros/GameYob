#pragma once

// Host lifecycle harness on MSVC/clang-cl: only the FileHandle boundary is
// exercised, so no directory enumeration implementation is needed.
#ifdef _WIN32
struct DIR;
struct dirent;
#else
#include_next <dirent.h>
#endif
