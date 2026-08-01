# Embedded Unicode font assets

`unicode_font.bin` is generated from Galmuri 11 bitmap sources and embedded in
the DS/DSi and 3DS builds. `cp949_table.bin` provides the legacy Korean CP949
fallback used for older filenames and cheat lists. Regenerate both files with:

```sh
python tools/generate_unicode_assets.py \
  --galmuri-dir PATH_TO_GALMURI/dist --output-dir assets/fonts
```

The source used for `v0.5.4-ko` is
[Galmuri](https://github.com/quiple/galmuri) commit
`71e1cacf1437a11220307120e63e30bc275312d4`. Galmuri is distributed under the
SIL Open Font License 1.1; see `OFL.txt`. The generated font remains subject to
that license. The CP949 mapping table is generated from Python's standard
codec mapping and contains no game data.

# 내장 유니코드 글꼴

`unicode_font.bin`은 Galmuri 11에서 생성되어 DS/DSi 및 3DS 실행 파일에
포함됩니다. `cp949_table.bin`은 오래된 한글 파일명과 치트 목록을 위한 CP949
호환 표입니다. Galmuri 파생 글꼴에는 `OFL.txt`의 SIL Open Font License 1.1이
적용됩니다.
