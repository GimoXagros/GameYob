#pragma once

enum LanguageFileFormat {
    LANGUAGE_FORMAT_AUTO,
    LANGUAGE_FORMAT_INI,
    LANGUAGE_FORMAT_JSON,
    LANGUAGE_FORMAT_XML,
    LANGUAGE_FORMAT_YAML
};

const char* tr(const char* englishKey);

bool languageLoadCode(const char* code);
bool languageLoadFile(const char* filename);
bool languageParseText(LanguageFileFormat format, const char* text);
void languageReset();

const char* languageGetCode();
const char* languageGetFile();
int languageGetEntryCount();
