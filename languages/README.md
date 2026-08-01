# GameYob language files

GameYob loads UTF-8 language files from `/gameyob/languages`, `/languages`, or
the `languages` directory. English (`en`), Japanese (`ja`), and Korean (`ko`)
are supplied in INI, JSON, XML, and YAML. The built-in fallback is English, so
a missing key is displayed as its English key instead of becoming blank.

Select **Settings → Language** for a bundled language, or choose **Custom** and
use **Select Language File**. The selected code/path is stored in
`gameyobds.ini`. INI is tried first for a bundled language, followed by JSON,
YAML, and XML. A custom file can use `.ini`, `.json`, `.xml`, `.yaml`, or `.yml`.

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

Run `python tools/generate_language_files.py` to regenerate the twelve bundled
files from the canonical English-key list.

# GameYob 언어 파일

GameYob은 `/gameyob/languages`, `/languages`, `languages` 폴더의 UTF-8 언어
파일을 읽습니다. 영어(`en`), 일본어(`ja`), 한국어(`ko`)를 INI, JSON, XML,
YAML 네 형식으로 제공합니다. 번역 키가 없으면 빈 문자열이 아니라 영어
원문을 표시합니다.

**설정 → 언어**에서 기본 언어를 선택할 수 있습니다. 직접 만든 파일은
**사용자 → 언어 파일 선택**으로 지정합니다. 새 언어를 추가하려면 예제 파일을
복사하고 `strings`의 값만 번역하십시오. 영어 키는 바꾸지 말고 파일은 UTF-8로
저장해야 합니다.
