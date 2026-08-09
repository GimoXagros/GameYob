#include <assert.h>
#include <string.h>

#include "filechooser_path.h"

static void expectRoot(const char* path) {
    char match[32] = "unchanged";
    assert(!fileChooserParentMatch(path, match, sizeof(match)));
    assert(match[0] == '\0');
}

static void expectParent(const char* path, const char* expectedMatch) {
    char match[32];
    assert(fileChooserParentMatch(path, match, sizeof(match)));
    assert(strcmp(match, expectedMatch) == 0);
}

int main() {
    expectRoot(NULL);
    expectRoot("");
    expectRoot("/");
    expectRoot("fat:");
    expectRoot("fat:/");
    expectRoot("sdmc:/");

    expectParent("/gb", "gb");
    expectParent("/gb/", "gb");
    expectParent("fat:/games", "games");
    expectParent("fat:/games/", "games");
    expectParent("fat:/games/한글", "한글");
    return 0;
}
