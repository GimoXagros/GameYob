#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>

#include "io.h"
#include "localization.h"

namespace {

const int MAX_LANGUAGE_FILE_SIZE = 128 * 1024;
const int MAX_LANGUAGE_ENTRIES = 512;

struct LanguageEntry {
    std::string key;
    std::string value;
};

std::vector<LanguageEntry> entries;
char currentCode[16] = "en";
char currentFile[MAX_FILENAME_LEN] = "";

bool entryLess(const LanguageEntry& left, const LanguageEntry& right) {
    return left.key < right.key;
}

std::string trim(const std::string& input) {
    std::string::size_type first = 0;
    while (first < input.size() && isspace((unsigned char)input[first]))
        first++;
    std::string::size_type last = input.size();
    while (last > first && isspace((unsigned char)input[last - 1]))
        last--;
    return input.substr(first, last - first);
}

void setEntry(const std::string& key, const std::string& value) {
    if (key.empty() || value.empty() || key.size() > 127 || value.size() > 511)
        return;
    for (unsigned int i = 0; i < entries.size(); i++) {
        if (entries[i].key == key) {
            entries[i].value = value;
            return;
        }
    }
    if ((int)entries.size() >= MAX_LANGUAGE_ENTRIES)
        return;
    LanguageEntry entry;
    entry.key = key;
    entry.value = value;
    entries.push_back(entry);
}

void appendUtf8(std::string& output, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        output += (char)codepoint;
    }
    else if (codepoint <= 0x7FF) {
        output += (char)(0xC0 | (codepoint >> 6));
        output += (char)(0x80 | (codepoint & 0x3F));
    }
    else {
        output += (char)(0xE0 | (codepoint >> 12));
        output += (char)(0x80 | ((codepoint >> 6) & 0x3F));
        output += (char)(0x80 | (codepoint & 0x3F));
    }
}

bool parseHex4(const char* text, unsigned int* value) {
    unsigned int result = 0;
    for (int i = 0; i < 4; i++) {
        const char c = text[i];
        result <<= 4;
        if (c >= '0' && c <= '9') result |= c - '0';
        else if (c >= 'a' && c <= 'f') result |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') result |= c - 'A' + 10;
        else return false;
    }
    *value = result;
    return true;
}

bool parseQuoted(const char*& cursor, std::string& output) {
    while (*cursor && isspace((unsigned char)*cursor)) cursor++;
    if (*cursor != '"')
        return false;
    cursor++;
    output.clear();
    while (*cursor && *cursor != '"') {
        if (*cursor != '\\') {
            output += *cursor++;
            continue;
        }
        cursor++;
        if (!*cursor)
            return false;
        switch (*cursor) {
            case 'n': output += '\n'; cursor++; break;
            case 'r': output += '\r'; cursor++; break;
            case 't': output += '\t'; cursor++; break;
            case '"': output += '"'; cursor++; break;
            case '\\': output += '\\'; cursor++; break;
            case 'u': {
                unsigned int codepoint;
                if (!parseHex4(cursor + 1, &codepoint))
                    return false;
                appendUtf8(output, codepoint);
                cursor += 5;
                break;
            }
            default: output += *cursor++; break;
        }
    }
    if (*cursor != '"')
        return false;
    cursor++;
    return true;
}

std::string unquote(const std::string& input) {
    std::string value = trim(input);
    if (value.size() >= 2 && value[0] == '"' &&
            value[value.size() - 1] == '"') {
        const char* cursor = value.c_str();
        std::string decoded;
        if (parseQuoted(cursor, decoded))
            return decoded;
    }
    return value;
}

std::string xmlDecode(const std::string& input) {
    std::string output = input;
    struct Entity { const char* encoded; const char* decoded; };
    static const Entity entities[] = {
        {"&quot;", "\""}, {"&apos;", "'"}, {"&lt;", "<"},
        {"&gt;", ">"}, {"&amp;", "&"}
    };
    for (unsigned int i = 0; i < sizeof(entities) / sizeof(entities[0]); i++) {
        std::string::size_type pos = 0;
        while ((pos = output.find(entities[i].encoded, pos)) != std::string::npos) {
            output.replace(pos, strlen(entities[i].encoded), entities[i].decoded);
            pos += strlen(entities[i].decoded);
        }
    }
    return output;
}

bool parseIni(const char* text) {
    bool stringsSection = false;
    const char* cursor = text;
    while (*cursor) {
        const char* end = strchr(cursor, '\n');
        std::string line(cursor, end ? end - cursor : strlen(cursor));
        line = trim(line);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (!line.empty() && line[0] == '[') {
            stringsSection = line == "[strings]" || line == "[Strings]";
        }
        else if (stringsSection && !line.empty() && line[0] != ';' && line[0] != '#') {
            const std::string::size_type equals = line.find('=');
            if (equals != std::string::npos)
                setEntry(trim(line.substr(0, equals)),
                         unquote(line.substr(equals + 1)));
        }
        if (!end) break;
        cursor = end + 1;
    }
    return !entries.empty();
}

bool parseYaml(const char* text) {
    bool stringsSection = false;
    const char* cursor = text;
    while (*cursor) {
        const char* end = strchr(cursor, '\n');
        std::string raw(cursor, end ? end - cursor : strlen(cursor));
        if (!raw.empty() && raw[raw.size() - 1] == '\r')
            raw.erase(raw.size() - 1);
        const std::string line = trim(raw);
        if (line == "strings:") {
            stringsSection = true;
        }
        else if (stringsSection && !line.empty() && line[0] != '#') {
            if (!isspace((unsigned char)raw[0]))
                stringsSection = false;
            else {
                const std::string::size_type colon = line.find(':');
                if (colon != std::string::npos)
                    setEntry(unquote(line.substr(0, colon)),
                             unquote(line.substr(colon + 1)));
            }
        }
        if (!end) break;
        cursor = end + 1;
    }
    return !entries.empty();
}

bool parseJson(const char* text) {
    const char* cursor = strstr(text, "\"strings\"");
    if (!cursor)
        return false;
    cursor = strchr(cursor, '{');
    if (!cursor)
        return false;
    cursor++;
    while (*cursor) {
        while (*cursor && (isspace((unsigned char)*cursor) || *cursor == ',')) cursor++;
        if (*cursor == '}')
            break;
        std::string key;
        std::string value;
        if (!parseQuoted(cursor, key))
            return false;
        while (*cursor && isspace((unsigned char)*cursor)) cursor++;
        if (*cursor++ != ':')
            return false;
        if (!parseQuoted(cursor, value))
            return false;
        setEntry(key, value);
    }
    return !entries.empty();
}

bool parseXml(const char* text) {
    const char* cursor = text;
    while ((cursor = strstr(cursor, "<string")) != NULL) {
        const char* tagEnd = strchr(cursor, '>');
        if (!tagEnd)
            return false;
        const char* keyPos = strstr(cursor, "key=\"");
        if (!keyPos || keyPos > tagEnd) {
            cursor = tagEnd + 1;
            continue;
        }
        keyPos += 5;
        const char* keyEnd = strchr(keyPos, '"');
        const char* valueEnd = strstr(tagEnd + 1, "</string>");
        if (!keyEnd || keyEnd > tagEnd || !valueEnd)
            return false;
        setEntry(xmlDecode(std::string(keyPos, keyEnd - keyPos)),
                 xmlDecode(std::string(tagEnd + 1, valueEnd - tagEnd - 1)));
        cursor = valueEnd + 9;
    }
    return !entries.empty();
}

LanguageFileFormat formatFromFilename(const char* filename) {
    const char* extension = strrchr(filename, '.');
    if (!extension) return LANGUAGE_FORMAT_AUTO;
    extension++;
    if (strcasecmp(extension, "ini") == 0) return LANGUAGE_FORMAT_INI;
    if (strcasecmp(extension, "json") == 0) return LANGUAGE_FORMAT_JSON;
    if (strcasecmp(extension, "xml") == 0) return LANGUAGE_FORMAT_XML;
    if (strcasecmp(extension, "yaml") == 0 || strcasecmp(extension, "yml") == 0)
        return LANGUAGE_FORMAT_YAML;
    return LANGUAGE_FORMAT_AUTO;
}

bool readWholeFile(const char* filename, std::vector<char>& output) {
    FileHandle* file = file_open(filename, "rb");
    if (!file)
        return false;
    const int size = file_getSize(file);
    if (size <= 0 || size > MAX_LANGUAGE_FILE_SIZE) {
        file_close(file);
        return false;
    }
    output.resize(size + 1);
    file_read(&output[0], 1, size, file);
    file_close(file);
    output[size] = '\0';
    if (size >= 3 && (unsigned char)output[0] == 0xEF &&
            (unsigned char)output[1] == 0xBB &&
            (unsigned char)output[2] == 0xBF) {
        memmove(&output[0], &output[3], size - 2);
    }
    return true;
}

} // namespace

const char* tr(const char* englishKey) {
    if (!englishKey)
        return "";
    unsigned int low = 0;
    unsigned int high = entries.size();
    while (low < high) {
        const unsigned int middle = low + (high - low) / 2;
        if (entries[middle].key < englishKey)
            low = middle + 1;
        else
            high = middle;
    }
    if (low < entries.size() && entries[low].key == englishKey)
        return entries[low].value.c_str();
    return englishKey;
}

void languageReset() {
    entries.clear();
    strcpy(currentCode, "en");
    currentFile[0] = '\0';
}

bool languageParseText(LanguageFileFormat format, const char* text) {
    entries.clear();
    if (!text)
        return false;
    bool parsed = false;
    switch (format) {
        case LANGUAGE_FORMAT_INI: parsed = parseIni(text); break;
        case LANGUAGE_FORMAT_JSON: parsed = parseJson(text); break;
        case LANGUAGE_FORMAT_XML: parsed = parseXml(text); break;
        case LANGUAGE_FORMAT_YAML: parsed = parseYaml(text); break;
        default: return false;
    }
    if (parsed)
        std::sort(entries.begin(), entries.end(), entryLess);
    return parsed;
}

bool languageLoadFile(const char* filename) {
    std::vector<char> contents;
    const LanguageFileFormat format = formatFromFilename(filename);
    if (format == LANGUAGE_FORMAT_AUTO || !readWholeFile(filename, contents))
        return false;
    if (!languageParseText(format, &contents[0]))
        return false;
    strncpy(currentFile, filename, sizeof(currentFile));
    currentFile[sizeof(currentFile) - 1] = '\0';
    strcpy(currentCode, "custom");
    return true;
}

bool languageLoadCode(const char* code) {
    if (!code || !*code)
        code = "en";
    static const char* roots[] = {
        "/gameyob/languages", "/languages", "languages"
    };
    static const char* extensions[] = {"ini", "json", "yaml", "xml"};
    char filename[MAX_FILENAME_LEN];
    for (unsigned int root = 0; root < sizeof(roots) / sizeof(roots[0]); root++) {
        for (unsigned int extension = 0;
                extension < sizeof(extensions) / sizeof(extensions[0]); extension++) {
            snprintf(filename, sizeof(filename), "%s/%s.%s", roots[root], code,
                     extensions[extension]);
            if (languageLoadFile(filename)) {
                strncpy(currentCode, code, sizeof(currentCode));
                currentCode[sizeof(currentCode) - 1] = '\0';
                return true;
            }
        }
    }
    entries.clear();
    strncpy(currentCode, code, sizeof(currentCode));
    currentCode[sizeof(currentCode) - 1] = '\0';
    currentFile[0] = '\0';
    return strcasecmp(code, "en") == 0;
}

const char* languageGetCode() {
    return currentCode;
}

const char* languageGetFile() {
    return currentFile;
}

int languageGetEntryCount() {
    return entries.size();
}
