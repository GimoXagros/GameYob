#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "io.h"

int main() {
    const char* path = "/tmp/gameyob_io_test.bin";
    fs_deleteFile(path);

    FileHandle* file = file_open(path, "w+b");
    assert(file != NULL);
    const char prefix[] = "GameYob";
    file_write(prefix, 1, sizeof(prefix) - 1, file);
    assert(file_getSize(file) == static_cast<int>(sizeof(prefix) - 1));

    file_setSize(file, 4096);
    assert(file_getSize(file) == 4096);
    file_seek(file, 0, SEEK_SET);
    char readback[sizeof(prefix)] = {};
    file_read(readback, 1, sizeof(prefix) - 1, file);
    assert(strcmp(readback, prefix) == 0);
    file_close(file);

    file = file_open(path, "rb");
    assert(file != NULL);
    file_seek(file, 0, SEEK_END);
    char line[8] = "stale";
    file_gets(line, sizeof(line), file);
    assert(line[0] == '\0');
    file_close(file);

    fs_deleteFile(path);
    assert(!file_exists(path));

    // Public file helpers are intentionally null-safe so an I/O failure can
    // be handled without turning into an unrelated Guru Meditation crash.
    file_close(NULL);
    file_setSize(NULL, 16);
    file_gets(line, sizeof(line), NULL);
    assert(line[0] == '\0');
    return 0;
}
