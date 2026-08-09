#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include "io.h"

#ifdef DS
#include <fat.h>
#include "text.h"
#endif


#ifdef C_IO_FUNCTIONS

DIR* directory = 0;

#ifdef DS
struct FatAlias {
    std::string longName;
    std::string shortName;
};

static std::vector<FatAlias> fatAliases;

static bool cachedFatAlias(const char* filename, char* openedPath,
                           size_t openedPathCapacity) {
    const char* slash = strrchr(filename, '/');
    const char* basename = slash ? slash + 1 : filename;
    for (size_t i = 0; i < fatAliases.size(); i++) {
        if (fatAliases[i].longName != basename &&
                !textEquivalent(fatAliases[i].longName.c_str(), basename))
            continue;

        const size_t prefixLength = slash ?
            static_cast<size_t>(slash - filename) + 1 : 0;
        const std::string& openName = fatAliases[i].shortName.empty() ?
            fatAliases[i].longName : fatAliases[i].shortName;
        const size_t openLength = openName.size();
        if (prefixLength + openLength >= openedPathCapacity)
            return false;
        if (prefixLength)
            memcpy(openedPath, filename, prefixLength);
        memcpy(openedPath + prefixLength, openName.c_str(), openLength + 1);
        return true;
    }
    return false;
}

static FILE* openFatAlias(const char* filename, const char* params,
                          char* openedPath, size_t openedPathCapacity) {
    if (!filename || !params || !openedPath || !openedPathCapacity)
        return NULL;

    if (cachedFatAlias(filename, openedPath, openedPathCapacity)) {
        FILE* cached = fopen(openedPath, params);
        if (cached)
            return cached;
    }

    char shortName[FAT_SHORT_FILE_NAME_MAX + 1];
    if (!FAT_getShortNameFor(filename, shortName) || !shortName[0])
        return NULL;

    const char* slash = strrchr(filename, '/');
    const size_t prefixLength = slash ?
        static_cast<size_t>(slash - filename) + 1 : 0;
    const size_t shortLength = strlen(shortName);
    if (prefixLength + shortLength >= openedPathCapacity)
        return NULL;

    if (prefixLength)
        memcpy(openedPath, filename, prefixLength);
    memcpy(openedPath + prefixLength, shortName, shortLength + 1);
    return fopen(openedPath, params);
}
#endif

FileHandle* file_open(const char* filename, const char* params) {
    if (!filename || !params)
        return NULL;

    char openedPath[MAX_FILENAME_LEN];
    strncpy(openedPath, filename, sizeof(openedPath) - 1);
    openedPath[sizeof(openedPath) - 1] = '\0';

    FILE* file = fopen(openedPath, params);
#ifdef DS
    // FatFs exposes UTF-8 LFNs, but a few DSi launchers and legacy FAT images
    // still fail to reopen an LFN returned by readdir().  Resolve the same
    // directory entry through its ASCII 8.3 alias before reporting failure.
    if (!file)
        file = openFatAlias(filename, params, openedPath,
                            sizeof(openedPath));
#endif
    if (!file)
        return NULL;

    FileHandle* h = (FileHandle*)malloc(sizeof(FileHandle));
    if (!h) {
        fclose(file);
        return NULL;
    }
    h->filename = (char*)malloc(strlen(openedPath)+1);
    if (!h->filename) {
        fclose(file);
        free(h);
        return NULL;
    }
    strcpy(h->filename, openedPath);
    h->file = file;

    strncpy(h->flags, params, sizeof(h->flags) - 1);
    h->flags[sizeof(h->flags) - 1] = '\0';
    return h;
}

const char* file_getPath(FileHandle* h) {
    return h ? h->filename : NULL;
}

void file_close(FileHandle* h) {
    fclose(h->file);
    free(h->filename);
    free(h);
}
void file_flush(FileHandle* h) {
    if (h && h->file)
        fflush(h->file);
}
void file_read(void* buf, int bs, int size, FileHandle* h) {
    fread(buf, bs, size, h->file);
}
void file_write(const void* buf, int bs, int size, FileHandle* h) {
    fwrite(buf, bs, size, h->file);
}
void file_gets(char* buf, int size, FileHandle* h) {
    fgets(buf, size, h->file);
}
void file_putc(char c, FileHandle* h) {
    fputc(c, h->file);
}

void file_rewind(FileHandle* h) {
    rewind(h->file);
}
int file_tell(FileHandle* h) {
    return ftell(h->file);
}
void file_seek(FileHandle* h, int pos, int flags) {
    fseek(h->file, pos, flags);
}
int file_getSize(FileHandle* h) {
    int pos = ftell(h->file);
    fseek(h->file, 0, SEEK_END);
    int ret = ftell(h->file);
    fseek(h->file, pos, SEEK_SET);
    return ret;
}
void file_setSize(FileHandle* h, size_t neededSize) {
    size_t fileSize = file_getSize(h);
    fclose(h->file);
    h->file = fopen(h->filename, "ab");
    for (; fileSize<neededSize; fileSize++)
        fputc(0, h->file);
    fclose(h->file);
    h->file = fopen(h->filename, h->flags);
}

void file_printf(FileHandle* h, const char* s, ...) {
    va_list args;
    va_start(args, s);

    char buf[512];
    vsnprintf(buf, sizeof(buf), s, args);
    va_end(args);

    fputs(buf, h->file);
}

bool file_exists(const char* filename) {
    FileHandle* file = file_open(filename, "rb");
    if (!file)
        return false;
    file_close(file);
    return true;
}

void fs_deleteFile(const char* filename) {
    if (unlink(filename) == 0)
        return;
#ifdef DS
    char aliasPath[MAX_FILENAME_LEN];
    if (cachedFatAlias(filename, aliasPath, sizeof(aliasPath)))
        unlink(aliasPath);
#endif
}


struct dirent* fs_readdir() {
    return directory ? readdir(directory) : NULL;
}

void fs_getcwd(char* dest, size_t maxLen) {
    if (!dest || !maxLen)
        return;
    if (!getcwd(dest, maxLen))
        dest[0] = '\0';
}
void fs_chdir(const char* s) {
    if (!s)
        return;
    if (chdir(s) != 0 && directory != 0)
        return;

    char cwd[MAX_FILENAME_LEN];
    if (!getcwd(cwd, MAX_FILENAME_LEN))
        return;

    DIR* replacement = opendir(cwd);
    if (!replacement)
        return;
    if (directory != 0)
        closedir(directory);
    directory = replacement;
}

int fs_getPathType(const char* filename) {
    if (!filename || !*filename)
        return FS_PATH_UNKNOWN;
    struct stat info;
    if (stat(filename, &info) != 0)
        return FS_PATH_UNKNOWN;
    return S_ISDIR(info.st_mode) ? FS_PATH_DIRECTORY : FS_PATH_FILE;
}

void fs_closeDirectory() {
    if (directory != 0) {
        closedir(directory);
        directory = 0;
    }
}

void fs_cacheDirectoryAliases() {
#ifdef DS
    fatAliases.clear();
    if (!directory)
        return;

    rewinddir(directory);
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            continue;
        FatAlias alias;
        alias.longName = entry->d_name;
        char shortName[FAT_SHORT_FILE_NAME_MAX + 1] = "";
        if (FAT_getShortNameFor(entry->d_name, shortName) && shortName[0])
            alias.shortName = shortName;
        fatAliases.push_back(alias);
    }
    rewinddir(directory);
#endif
}

#endif
