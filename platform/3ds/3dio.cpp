#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "io.h"

struct FileHandle {
    FILE* file;
    char filename[MAX_FILENAME_LEN + 6];
    char flags[10];
};

static char fsCwd[MAX_FILENAME_LEN] = "/";
static DIR* directory = NULL;

static bool copyString(char* destination, size_t capacity,
        const char* source) {
    if (!destination || !capacity || !source)
        return false;
    const size_t length = strlen(source);
    if (length >= capacity)
        return false;
    memcpy(destination, source, length + 1);
    return true;
}

static bool logicalPath(char* destination, size_t capacity,
        const char* source) {
    if (!destination || !capacity || !source)
        return false;

    if (strncmp(source, "sdmc:", 5) == 0)
        source += 5;

    if (source[0] == '/')
        return copyString(destination, capacity, source);

    const size_t cwdLength = strlen(fsCwd);
    const int written = snprintf(destination, capacity, "%s%s%s", fsCwd,
        cwdLength && fsCwd[cwdLength - 1] == '/' ? "" : "/", source);
    return written >= 0 && written < (int)capacity;
}

static void trimParent(char* path) {
    size_t length = strlen(path);
    while (length > 1 && path[length - 1] == '/')
        path[--length] = '\0';
    char* slash = strrchr(path, '/');
    if (!slash || slash == path)
        strcpy(path, "/");
    else
        *slash = '\0';
}

static bool physicalPath(char* destination, size_t capacity,
        const char* source) {
    char logical[MAX_FILENAME_LEN];
    if (!logicalPath(logical, sizeof(logical), source))
        return false;
    const int written = snprintf(destination, capacity, "sdmc:%s", logical);
    return written >= 0 && written < (int)capacity;
}

void fs_init() {
    strcpy(fsCwd, "/");
    if (directory)
        closedir(directory);
    directory = opendir("sdmc:/");
}

FileHandle* file_open(const char* filename, const char* flags) {
#ifdef EMBEDDED_ROM
    return NULL;
#endif
    if (!filename || !flags)
        return NULL;

    FileHandle* handle = (FileHandle*)calloc(1, sizeof(FileHandle));
    if (!handle)
        return NULL;
    if (!physicalPath(handle->filename, sizeof(handle->filename), filename) ||
            !copyString(handle->flags, sizeof(handle->flags), flags)) {
        free(handle);
        return NULL;
    }

    handle->file = fopen(handle->filename, flags);
    if (!handle->file) {
        free(handle);
        return NULL;
    }
    return handle;
}

void file_close(FileHandle* handle) {
    if (!handle)
        return;
    if (handle->file)
        fclose(handle->file);
    free(handle);
}

void file_flush(FileHandle* handle) {
    if (handle && handle->file)
        fflush(handle->file);
}

void file_read(void* destination, int blockSize, int count,
        FileHandle* handle) {
    if (handle && handle->file)
        fread(destination, blockSize, count, handle->file);
}

void file_write(const void* source, int blockSize, int count,
        FileHandle* handle) {
    if (handle && handle->file)
        fwrite(source, blockSize, count, handle->file);
}

void file_gets(char* destination, int capacity, FileHandle* handle) {
    if (!destination || capacity <= 0)
        return;
    if (!handle || !handle->file || !fgets(destination, capacity, handle->file))
        destination[0] = '\0';
}

void file_putc(char value, FileHandle* handle) {
    if (handle && handle->file)
        fputc(value, handle->file);
}

int file_tell(FileHandle* handle) {
    return handle && handle->file ? (int)ftell(handle->file) : -1;
}

void file_seek(FileHandle* handle, int offset, int origin) {
    if (handle && handle->file)
        fseek(handle->file, offset, origin);
}

int file_getSize(FileHandle* handle) {
    if (!handle || !handle->file)
        return 0;
    const long oldPosition = ftell(handle->file);
    fseek(handle->file, 0, SEEK_END);
    const long size = ftell(handle->file);
    fseek(handle->file, oldPosition, SEEK_SET);
    return size < 0 ? 0 : (int)size;
}

void file_setSize(FileHandle* handle, size_t neededSize) {
    if (!handle || !handle->file)
        return;
    fflush(handle->file);
    if (ftruncate(fileno(handle->file), neededSize) == 0)
        return;

    size_t currentSize = file_getSize(handle);
    fseek(handle->file, 0, SEEK_END);
    while (currentSize++ < neededSize)
        fputc(0, handle->file);
    fflush(handle->file);
}

void file_printf(FileHandle* handle, const char* format, ...) {
    if (!handle || !handle->file || !format)
        return;
    va_list arguments;
    va_start(arguments, format);
    vfprintf(handle->file, format, arguments);
    va_end(arguments);
}

bool file_exists(const char* filename) {
    FileHandle* handle = file_open(filename, "rb");
    if (!handle)
        return false;
    file_close(handle);
    return true;
}

void fs_deleteFile(const char* filename) {
    char path[MAX_FILENAME_LEN + 6];
    if (physicalPath(path, sizeof(path), filename))
        remove(path);
}

struct dirent* fs_readdir() {
    return directory ? readdir(directory) : NULL;
}

void fs_getcwd(char* destination, size_t capacity) {
    if (destination && capacity) {
        strncpy(destination, fsCwd, capacity - 1);
        destination[capacity - 1] = '\0';
    }
}

void fs_chdir(const char* path) {
    if (!path)
        return;

    char next[MAX_FILENAME_LEN];
    if (strcmp(path, "..") == 0 || strcmp(path, "../") == 0) {
        if (!copyString(next, sizeof(next), fsCwd))
            return;
        trimParent(next);
    }
    else if (!logicalPath(next, sizeof(next), path)) {
        return;
    }

    size_t length = strlen(next);
    while (length > 1 && next[length - 1] == '/')
        next[--length] = '\0';

    char physical[MAX_FILENAME_LEN + 6];
    const int written = snprintf(physical, sizeof(physical), "sdmc:%s", next);
    if (written < 0 || written >= (int)sizeof(physical))
        return;

    DIR* replacement = opendir(physical);
    if (!replacement)
        return;
    if (directory)
        closedir(directory);
    directory = replacement;
    copyString(fsCwd, sizeof(fsCwd), next);
}

void fs_closeDirectory() {
    if (directory) {
        closedir(directory);
        directory = NULL;
    }
}
