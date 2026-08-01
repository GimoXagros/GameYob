#include "localization.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check(LanguageFileFormat format, const char* document,
                  const char* expected) {
    assert(languageParseText(format, document));
    assert(languageGetEntryCount() == 2);
    assert(strcmp(tr("Load ROM"), expected) == 0);
    assert(strcmp(tr("Untranslated key"), "Untranslated key") == 0);
}

int main() {
    check(LANGUAGE_FORMAT_INI,
          "[strings]\nLoad ROM=롬 불러오기\nQuit=종료\n",
          "롬 불러오기");
    check(LANGUAGE_FORMAT_JSON,
          "{\"strings\":{\"Load ROM\":\"ROMを開く\",\"Quit\":\"終了\"}}",
          "ROMを開く");
    check(LANGUAGE_FORMAT_XML,
          "<language><strings><string key=\"Load ROM\">롬 &amp; 불러오기"
          "</string><string key=\"Quit\">종료</string></strings></language>",
          "롬 & 불러오기");
    check(LANGUAGE_FORMAT_YAML,
          "strings:\n  \"Load ROM\": \"ROMを開く\"\n  Quit: 終了\n",
          "ROMを開く");

    assert(!languageParseText(LANGUAGE_FORMAT_JSON, "{\"wrong\":{}}"));
    languageReset();
    assert(strcmp(languageGetCode(), "en") == 0);
    assert(strcmp(tr("Load ROM"), "Load ROM") == 0);

    assert(languageLoadCode("ja"));
    assert(strcmp(languageGetCode(), "ja") == 0);
    assert(strcmp(languageGetFile(), "") == 0);
    assert(strcmp(tr("Settings"), "設定") == 0);
    assert(languageLoadCode("ko"));
    assert(strcmp(languageGetCode(), "ko") == 0);
    assert(strcmp(tr("Settings"), "설정") == 0);
    assert(strcmp(tr("Wireless Link"), "무선 링크") == 0);
    assert(languageLoadCode("en"));
    assert(strcmp(tr("Settings"), "Settings") == 0);

    const char* templateFilename = "gameyob-language-test.ini";
    remove(templateFilename);
    assert(languageCreateEnglishTemplate(templateFilename));
    assert(languageLoadFile(templateFilename));
    assert(strcmp(languageGetCode(), "custom") == 0);
    assert(strcmp(tr("Settings"), "Settings") == 0);

    FILE* existing = fopen(templateFilename, "ab");
    assert(existing);
    fputs("; user edit\n", existing);
    fclose(existing);
    existing = fopen(templateFilename, "rb");
    assert(existing);
    fseek(existing, 0, SEEK_END);
    const long editedSize = ftell(existing);
    fclose(existing);
    assert(languageCreateEnglishTemplate(templateFilename));
    existing = fopen(templateFilename, "rb");
    assert(existing);
    fseek(existing, 0, SEEK_END);
    assert(ftell(existing) == editedSize);
    fclose(existing);
    remove(templateFilename);
    return 0;
}
