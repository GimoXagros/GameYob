# GameYob language files

English (`en`), Japanese (`ja`), and Korean (`ko`) are embedded in the
executable, so **Settings → Language** works even when no external language
directory is installed. Equivalent editable examples are supplied in INI,
JSON, XML, and YAML. A missing key is displayed as its English key instead of
becoming blank.

Choose **Custom** and use **Select Language File** to load a UTF-8 `.ini`,
`.json`, `.xml`, `.yaml`, or `.yml` file. The selected code/path is stored in
`gameyobds.ini`.

Selecting **Save Settings** creates `gameyob_language.ini` beside
`gameyobds.ini` when it does not already exist. This generated file contains
the complete English key/value set and is never overwritten, so it can be used
as a persistent starting point for another language.

To add a language, copy any supplied file and translate the values under
`strings`; do not change the English keys. Keep the file UTF-8 encoded. The
parser accepts at most 512 entries, 127 bytes per key, 511 bytes per value, and
a 128 KiB file. These limits keep loading predictable on Nintendo DS hardware.

Equivalent minimal examples:

```ini
[strings]
Settings=Settings
Load State=Load State
```

```json
{"strings":{"Settings":"Settings","Load State":"Load State"}}
```

```xml
<language><strings>
  <string key="Settings">Settings</string>
  <string key="Load State">Load State</string>
</strings></language>
```

```yaml
strings:
  "Settings": "Settings"
  "Load State": "Load State"
```

Run `python tools/generate_language_files.py` to regenerate the twelve example
files and the embedded-language table from the canonical English-key list.

# GameYob 언어 파일

영어(`en`)·일본어(`ja`)·한국어(`ko`) 메뉴는 실행 파일에 내장되어 외부
언어 폴더가 없어도 **설정 → 언어**에서 즉시 적용됩니다. 같은 내용을 편집할
수 있는 INI·JSON·XML·YAML 예제도 제공합니다. 번역 키가 없으면 빈 문자열이
아니라 영어 원문을 표시합니다.

**설정 → 언어**에서 기본 언어를 선택할 수 있습니다. 직접 만든 파일은
**사용자 → 언어 파일 선택**으로 지정합니다. 새 언어를 추가하려면 예제 파일을
복사하고 `strings`의 값만 번역하십시오. 영어 키는 바꾸지 말고 파일은 UTF-8로
저장해야 합니다.

**설정 저장**을 선택하면 `gameyobds.ini`와 같은 위치에 영어 기본값 전체가
들어 있는 `gameyob_language.ini`를 생성합니다. 이미 파일이 있으면 덮어쓰지
않으므로 이 파일을 그대로 편집해 사용자 언어의 시작점으로 사용할 수 있습니다.
