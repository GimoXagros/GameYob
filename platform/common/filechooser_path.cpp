#include "filechooser_path.h"

#include <string.h>

bool fileChooserParentInfo(const char* currentPath,
                           char* parentPath, size_t parentPathCapacity,
                           char* matchName, size_t matchNameCapacity) {
    if (parentPath && parentPathCapacity)
        parentPath[0] = '\0';
    if (matchName && matchNameCapacity)
        matchName[0] = '\0';
    if (!currentPath || !currentPath[0])
        return false;

    size_t length = strlen(currentPath);
    while (length > 1 && currentPath[length - 1] == '/') {
        // libfat represents the filesystem root as "fat:/". Preserve that
        // slash: reducing it to "fat:" used to make strrchr() return NULL and
        // the file chooser then constructed std::string(NULL + 1).
        if (length >= 2 && currentPath[length - 2] == ':')
            return false;
        length--;
    }

    if (length == 1 && currentPath[0] == '/')
        return false;

    const char* colon = static_cast<const char*>(
        memchr(currentPath, ':', length));
    const size_t volumeLength = colon ?
        static_cast<size_t>(colon - currentPath) + 1 : 0;
    if (length <= volumeLength)
        return false;

    size_t componentStart = length;
    while (componentStart > volumeLength &&
           currentPath[componentStart - 1] != '/') {
        componentStart--;
    }

    const size_t componentLength = length - componentStart;
    if (!componentLength)
        return false;

    size_t parentLength = componentStart;
    while (parentLength > 1 && currentPath[parentLength - 1] == '/' &&
           currentPath[parentLength - 2] != ':') {
        parentLength--;
    }
    if (parentPath && parentPathCapacity) {
        if (parentLength == 0) {
            if (parentPathCapacity < 3)
                return false;
            memcpy(parentPath, "..", 3);
        }
        else {
            if (parentLength >= parentPathCapacity)
                return false;
            memcpy(parentPath, currentPath, parentLength);
            parentPath[parentLength] = '\0';
        }
    }
    if (matchName && matchNameCapacity) {
        const size_t copyLength = componentLength < matchNameCapacity - 1 ?
            componentLength : matchNameCapacity - 1;
        memcpy(matchName, currentPath + componentStart, copyLength);
        matchName[copyLength] = '\0';
    }
    return true;
}
