# GameYob

<p align="center">
  <img src="logo.png" alt="GameYob logo" width="480">
</p>

GameYob v0.5.11 is a homebrew Game Boy / Game Boy Color emulator for Nintendo
DS and DSi. A Nintendo 3DS can run the NDS version in DS mode. Native 3DSX
development is paused and no 3DSX executable is included in this release.
No ROM, BIOS, or firmware is included.

GameYob v0.5.11은 Nintendo DS·DSi용 Game Boy / Game Boy Color 홈브루
에뮬레이터입니다. Nintendo 3DS에서는 NDS판을 DS 모드로 실행할 수 있습니다.
네이티브 3DSX 개발은 보류 중이며 이번 릴리스에는 3DSX 실행 파일이 없습니다.
ROM·BIOS·펌웨어도 포함하지 않습니다.

## Download / 다운로드

- [Latest stable release: GameYob v0.5.11](https://github.com/GimoXagros/GameYob/releases/tag/v0.5.11)
- [DS/DSi package](https://github.com/GimoXagros/GameYob/releases/download/v0.5.11/gameyob-v0.5.11.zip): `gameyob.nds`, `gameyob_dsi.nds`, guides, language examples, notices, and checksums
- [Detailed release record](docs/releases/v0.5.11.md) · [Changelog](CHANGELOG) · [Build instructions](BUILDING.md)

Back up saves, states, settings, and printer BMPs before replacing an older
build. The launcher determines DS or DSi execution mode; a filename alone does
not force that mode. 이전 빌드를 교체하기 전에 세이브·상태·설정·프린터 BMP를
백업하십시오. 실행 모드는 파일 이름이 아니라 런처가 결정합니다.

## Current features / 주요 기능

- Touch-capable DS/DSi menus and file chooser; physical controls remain available.
- GB/GBC cartridge support including MBC7, HuC1, HuC3, and MMM01, with
  cartridge-specific physical validation still incomplete.
- SGB border and partial host-runtime support; a complete SNES host is **not** claimed.
- Game Boy Printer packet handling and numbered BMP output beside the ROM.
- English, Japanese, and Korean menus and guides; editable language files.
- The v0.5.11 DS banner reads `GameYob Custom` / `A Gameboy Emulator for DS` /
  `GimoXagros`, using the supplied transparent icon.

기능의 범위와 검증 한계는 [v0.5.11 릴리스 기록](docs/releases/v0.5.11.md)과
[CHANGELOG](CHANGELOG)에 있습니다. SGB 호스트 구현과 희귀 카트리지 실기
검증은 아직 완료되지 않았습니다.

## Guides / 사용 설명서

- [English](docs/guides/user-guide.en.md)
- [日本語](docs/guides/user-guide.ja.md)
- [한국어](docs/guides/user-guide.ko.md)

## Known issues / 알려진 문제

- Castlevania Legends can show intermittent thin white horizontal lines while
  L fast-forward is held. This remains [open issue #8](https://github.com/GimoXagros/GameYob/issues/8),
  not a claimed fix. Castlevania Legends에서 L 배속 중 얇은 흰 가로줄이
  간헐적으로 나타나는 문제는 미해결입니다.
- A user reports that Korean Pokémon Silver shows a model warning with
  **Detect GBA On** but starts normally with it Off. The exact cause is not
  verified. Detect GBA is a boot-identification flag, not a speed option.
  한국어판 포켓몬 은의 GBA 감지 관련 보고는 원인 미확인입니다.
- Native 3DSX development, scaling, performance work, and LAN validation are
  deferred together; see [the archived 3DSX plan](backup/3dsx/TODO.md).

User-reported good overall performance on a 3DS in DS mode does not establish
all-games or all-device compatibility. When reporting an issue, provide the
revision, device/launcher, settings, steps, and ROM SHA-256—not the ROM or saves.

## Development and credits / 개발 및 기여

See [BUILDING.md](BUILDING.md) for the pinned DS/DSi build and tests,
[CONTRIBUTORS.md](CONTRIBUTORS.md) for attribution, and
[repository policy](docs/maintenance/repository-policy.md) for branch, tag,
and release handling. Earlier release archives remain in
[`old_releases`](old_releases); the deferred native 3DSX binary remains in
[`backup/3dsx`](backup/3dsx). The inherited 3-in-1 code's exact older
permission remains unverified and is disclosed in
[THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt); this is not a claim of
license clearance.
