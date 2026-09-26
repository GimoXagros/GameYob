#pragma once

// The Windows host harness substitutes the filesystem adapter. Production
// Unix and DS builds use their normal unistd.h, not this test-only shim.
#ifndef _WIN32
#include_next <unistd.h>
#endif
