#include <assert.h>
#include <string.h>

#include "filechooser_path.h"

static void expectRoot(const char* path) {
    char parent[32] = "unchanged";
    char match[32] = "unchanged";
    assert(!fileChooserParentInfo(path, parent, sizeof(parent),
                                  match, sizeof(match)));
    assert(parent[0] == '\0');
    assert(match[0] == '\0');
}

static void expectParent(const char* path, const char* expectedParent,
                         const char* expectedMatch) {
    char parent[32];
    char match[32];
    assert(fileChooserParentInfo(path, parent, sizeof(parent),
                                 match, sizeof(match)));
    assert(strcmp(parent, expectedParent) == 0);
    assert(strcmp(match, expectedMatch) == 0);
}

int main() {
    expectRoot(NULL);
    expectRoot("");
    expectRoot("/");
    expectRoot("fat:");
    expectRoot("fat:/");
    expectRoot("sdmc:/");

    expectParent("/gb", "/", "gb");
    expectParent("/gb/", "/", "gb");
    expectParent("fat:/games", "fat:/", "games");
    expectParent("fat:/games/", "fat:/", "games");
    expectParent("fat:/games/한글", "fat:/games", "한글");
    expectParent("sd:/gb", "sd:/", "gb");
    return 0;
}
