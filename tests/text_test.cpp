#include "text.h"

#include <assert.h>
#include <string.h>

int main() {
    assert(textEquivalent("GameYob", "GameYob"));
    assert(!textEquivalent("GameYob", "GameYob2"));
    assert(textEquivalent("\xED\x95\x9C\xEA\xB8\x80",
                          "\xED\x95\x9C\xEA\xB8\x80"));
    assert(textColumns("GameYob") == 7);
    assert(textColumns("한글 이름") == 5);
    assert(textColumns("日本語") == 3);

    char output[32];
    textCopyColumns(output, sizeof(output), "한글 파일명.gbc", 5);
    assert(strcmp(output, "한글 파일") == 0);

    char small[5];
    textCopyColumns(small, sizeof(small), "가나다", 3);
    assert(strcmp(small, "가") == 0);
    return 0;
}
