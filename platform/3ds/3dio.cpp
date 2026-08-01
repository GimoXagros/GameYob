#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <3ds.h>
#include <3ds/types.h>
#include <3ds/svc.h>
#include <cwchar>
#include "io.h"
#include "error.h"

FS_archive sdmcArchive;

struct FileHandle {
    Handle handle;
    size_t head;
};

struct DirStruct {
    Handle handle;
    struct dirent activeEntry;
};

char fs_cwd[MAX_FILENAME_LEN];
DirStruct dir;

// private functions

static void copyPath(char* dest, const char* src) {
    if (!src)
        src = "";
    strncpy(dest, src, MAX_FILENAME_LEN - 1);
    dest[MAX_FILENAME_LEN - 1] = '\0';
}

static void appendPath(char* dest, const char* src) {
    size_t used = strlen(dest);
    if (used >= MAX_FILENAME_LEN - 1 || !src)
        return;
    strncpy(dest + used, src, MAX_FILENAME_LEN - used - 1);
    dest[MAX_FILENAME_LEN - 1] = '\0';
}

void fs_relativePath(char* dest, const char* src) {
    if (!dest || !src)
        return;

    const bool back = strcmp(src, "..") == 0 || strcmp(src, "../") == 0;
    if (back)
        copyPath(dest, fs_cwd);
    else if (src[0] == '/')
        copyPath(dest, src);
    else {
        if (dest != fs_cwd)
            copyPath(dest, fs_cwd);
        const size_t length = strlen(dest);
        if (length && dest[length - 1] != '/')
            appendPath(dest, "/");
        appendPath(dest, src);
    }

    size_t length = strlen(dest);
    while (length > 1 && dest[length - 1] == '/')
        dest[--length] = '\0';

    if (back) {
        char* slash = strrchr(dest, '/');
        if (slash) {
            *slash = '\0';
            if (!dest[0])
                copyPath(dest, "/");
        }
    }
}

static void utf16ToUtf8(char* dest, size_t capacity, const u16* src,
        size_t sourceCapacity) {
    if (!capacity)
        return;
    size_t output = 0;
    for (size_t input = 0; input < sourceCapacity && src[input]; ++input) {
        u32 codepoint = src[input];
        if (codepoint >= 0xd800 && codepoint <= 0xdbff &&
                input + 1 < sourceCapacity && src[input + 1] >= 0xdc00 &&
                src[input + 1] <= 0xdfff) {
            codepoint = 0x10000 + ((codepoint - 0xd800) << 10) +
                        (src[++input] - 0xdc00);
        }
        else if (codepoint >= 0xd800 && codepoint <= 0xdfff) {
            codepoint = 0xfffd;
        }

        char encoded[4];
        size_t count;
        if (codepoint < 0x80) {
            encoded[0] = (char)codepoint;
            count = 1;
        }
        else if (codepoint < 0x800) {
            encoded[0] = (char)(0xc0 | (codepoint >> 6));
            encoded[1] = (char)(0x80 | (codepoint & 0x3f));
            count = 2;
        }
        else if (codepoint < 0x10000) {
            encoded[0] = (char)(0xe0 | (codepoint >> 12));
            encoded[1] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
            encoded[2] = (char)(0x80 | (codepoint & 0x3f));
            count = 3;
        }
        else {
            encoded[0] = (char)(0xf0 | (codepoint >> 18));
            encoded[1] = (char)(0x80 | ((codepoint >> 12) & 0x3f));
            encoded[2] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
            encoded[3] = (char)(0x80 | (codepoint & 0x3f));
            count = 4;
        }
        if (output + count >= capacity)
            break;
        memcpy(dest + output, encoded, count);
        output += count;
    }
    dest[output] = '\0';
}

static bool makeUtf16Path(const char* source, u16* units, size_t capacity,
        FS_path* path) {
    if (!source || !units || capacity < 2 || !path)
        return false;
    const unsigned char* input =
        reinterpret_cast<const unsigned char*>(source);
    size_t output = 0;
    while (*input) {
        u32 codepoint;
        size_t bytes;
        if (input[0] < 0x80) {
            codepoint = input[0];
            bytes = 1;
        }
        else if (input[0] >= 0xc2 && input[0] <= 0xdf &&
                (input[1] & 0xc0) == 0x80) {
            codepoint = ((input[0] & 0x1f) << 6) | (input[1] & 0x3f);
            bytes = 2;
        }
        else if (input[0] >= 0xe0 && input[0] <= 0xef && input[1] &&
                input[2] && (input[1] & 0xc0) == 0x80 &&
                (input[2] & 0xc0) == 0x80 &&
                !(input[0] == 0xe0 && input[1] < 0xa0) &&
                !(input[0] == 0xed && input[1] >= 0xa0)) {
            codepoint = ((input[0] & 0x0f) << 12) |
                        ((input[1] & 0x3f) << 6) | (input[2] & 0x3f);
            bytes = 3;
        }
        else if (input[0] >= 0xf0 && input[0] <= 0xf4 && input[1] &&
                input[2] && input[3] && (input[1] & 0xc0) == 0x80 &&
                (input[2] & 0xc0) == 0x80 && (input[3] & 0xc0) == 0x80 &&
                !(input[0] == 0xf0 && input[1] < 0x90) &&
                !(input[0] == 0xf4 && input[1] >= 0x90)) {
            codepoint = ((input[0] & 7) << 18) |
                        ((input[1] & 0x3f) << 12) |
                        ((input[2] & 0x3f) << 6) | (input[3] & 0x3f);
            bytes = 4;
        }
        else {
            return false;
        }

        if (codepoint <= 0xffff) {
            if (output + 1 >= capacity)
                return false;
            units[output++] = (u16)codepoint;
        }
        else {
            if (output + 2 >= capacity)
                return false;
            codepoint -= 0x10000;
            units[output++] = 0xd800 | (codepoint >> 10);
            units[output++] = 0xdc00 | (codepoint & 0x3ff);
        }
        input += bytes;
    }
    units[output] = 0;
    path->type = PATH_WCHAR;
    path->size = (output + 1) * sizeof(u16);
    path->data = reinterpret_cast<const u8*>(units);
    return true;
}

// public functions

void fs_init() {
    /*
    // These checks don't seem to be working?
    u32 detected, writable;
    FSUSER_IsSdmcDetected(NULL, &detected);
    if (!detected)
        fatalerr("SD card not detected.");
    FSUSER_IsSdmcWritable(NULL, &writable);
    if (!writable)
        fatalerr("SD card not writable.");
    */


    sdmcArchive = (FS_archive){0x9, FS_makePath(PATH_EMPTY, "")};
    FSUSER_OpenArchive(NULL, &sdmcArchive);

    strcpy(fs_cwd, "");
    dir.handle = 0;
    fs_chdir("/");
}

FileHandle* file_open(const char* filename, const char* flags) {
#ifdef EMBEDDED_ROM
    return NULL;
#endif

    u32 openFlags = 0;

    for (uint i=0; i<strlen(flags); i++) {
        char c = flags[i];
        switch(c) {
            case 'r':
                openFlags |= FS_OPEN_READ;
                break;
            case 'w':
                openFlags |= FS_OPEN_WRITE | FS_OPEN_CREATE;
                break;
            case '+':
                if (openFlags & FS_OPEN_READ)
                    openFlags |= FS_OPEN_WRITE;
                else if (openFlags & FS_OPEN_WRITE)
                    openFlags |= FS_OPEN_READ | FS_OPEN_CREATE;
                break;
        }
    }

    char buffer[MAX_FILENAME_LEN];
    fs_relativePath(buffer, filename);

    u16 utf16Path[MAX_FILENAME_LEN + 1];
    FS_path filePath;
    if (!makeUtf16Path(buffer, utf16Path,
            sizeof(utf16Path) / sizeof(utf16Path[0]), &filePath))
        return NULL;

    FileHandle* fileHandle = (FileHandle*)malloc(sizeof(FileHandle));
    if (!fileHandle)
        return NULL;
    Result res = FSUSER_OpenFile(NULL, &fileHandle->handle, sdmcArchive,
            filePath, openFlags, FS_ATTRIBUTE_NONE);

    if (res) {
        free(fileHandle);
        return NULL;
    }

    fileHandle->head = 0;
    return fileHandle;
}

void file_close(FileHandle* fileHandle) {
    FSFILE_Close(fileHandle->handle);
    free(fileHandle);
}

void file_read(void* dest, int bs, int size, FileHandle* fileHandle) {
    u32 bytesRead;
    FSFILE_Read(fileHandle->handle, &bytesRead, fileHandle->head, dest, bs*size);
    fileHandle->head += bytesRead;
}

void file_write(const void* src, int bs, int size, FileHandle* fileHandle) {
    u32 bytesWritten;
    FSFILE_Write(fileHandle->handle, &bytesWritten, fileHandle->head, src, bs*size,
            FS_WRITE_FLUSH);
    fileHandle->head += bytesWritten;
}

void file_gets(char* buffer, int bufferSize, FileHandle* fileHandle) {
    u32 bytesRead;
    char* ptr = buffer;

    while (bufferSize > 1) {
        char c;
        FSFILE_Read(fileHandle->handle, &bytesRead, fileHandle->head, &c, 1);
        fileHandle->head += bytesRead;
        if (bytesRead != 1)
            break;

        *(ptr++) = c;
        bufferSize--;

        if (c == '\n' || c == '\0')
            break;
    }

    // bufferSize >= 1. Always terminate, including an empty/EOF read.
    *ptr = '\0';
}

void file_putc(char c, FileHandle* fileHandle) {
    file_write(&c, 1, 1, fileHandle);
}

int file_tell(FileHandle* fileHandle) {
    return fileHandle->head;
}

void file_seek(FileHandle* fileHandle, int pos, int origin) {
    switch (origin) {
        case SEEK_SET:
            fileHandle->head = pos;
            break;
        case SEEK_CUR:
            fileHandle->head += pos;
            break;
        case SEEK_END:
            fileHandle->head = file_getSize(fileHandle) - pos;
            break;
    }
}

int file_getSize(FileHandle* fileHandle) {
    u64 size;
    FSFILE_GetSize(fileHandle->handle, &size);
    return size;
}

void file_setSize(FileHandle* fileHandle, size_t size) {
    FSFILE_SetSize(fileHandle->handle, size);
}

void file_printf(FileHandle* fileHandle, const char* format, ...) {
    char buffer[512];

    va_list args;
    va_start(args, format);

    vsnprintf(buffer, 512, format, args);
    va_end(args);

    file_write(buffer, 1, strlen(buffer), fileHandle);
}

bool file_exists(const char* filename) {
    FileHandle* h = file_open(filename, "r");
    if (h == NULL)
        return false;
    else {
        file_close(h);
        return true;
    }
}

void fs_deleteFile(const char* filename) {
    char buffer[MAX_FILENAME_LEN];
    fs_relativePath(buffer, filename);
    u16 utf16Path[MAX_FILENAME_LEN + 1];
    FS_path filePath;
    if (makeUtf16Path(buffer, utf16Path,
            sizeof(utf16Path) / sizeof(utf16Path[0]), &filePath))
        FSUSER_DeleteFile(NULL, sdmcArchive, filePath);
}

struct dirent* fs_readdir() {
    u32 numEntries = 0;
    FS_dirent entry;
    FSDIR_Read(dir.handle, &numEntries, 1, &entry);

    if (numEntries == 0)
        return 0;

    // FS returns UTF-16 names. Converting instead of narrowing each code unit
    // is what makes Korean/Japanese filenames round-trip through the chooser.
    utf16ToUtf8(dir.activeEntry.d_name, sizeof(dir.activeEntry.d_name),
                entry.name, sizeof(entry.name) / sizeof(entry.name[0]));
    dir.activeEntry.d_type = 0;
    if (entry.isDirectory)
        dir.activeEntry.d_type |= DT_DIR;

    return &dir.activeEntry;
}

void fs_getcwd(char* dest, size_t maxLen) {
    if (!dest || !maxLen)
        return;
    strncpy(dest, fs_cwd, maxLen - 1);
    dest[maxLen - 1] = '\0';
}
void fs_chdir(const char* s) {
    char buffer[MAX_FILENAME_LEN];
    fs_relativePath(buffer, s);

    u16 utf16Path[MAX_FILENAME_LEN + 1];
    FS_path directoryPath;
    if (!makeUtf16Path(buffer, utf16Path,
            sizeof(utf16Path) / sizeof(utf16Path[0]), &directoryPath))
        return;

    if (dir.handle != 0)
        FSDIR_Close(dir.handle);

    Result res = FSUSER_OpenDirectory(NULL, &dir.handle, sdmcArchive,
            directoryPath);

    if (res) {
        u16 previousUtf16[MAX_FILENAME_LEN + 1];
        FS_path previousPath;
        if (makeUtf16Path(fs_cwd, previousUtf16,
                sizeof(previousUtf16) / sizeof(previousUtf16[0]),
                &previousPath))
            FSUSER_OpenDirectory(NULL, &dir.handle, sdmcArchive, previousPath);
        return;
    }

    copyPath(fs_cwd, buffer);
}
