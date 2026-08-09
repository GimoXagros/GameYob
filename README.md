## Info about GameYob Branches

The master branch of this repository (https://github.com/Drenn1/GameYob)
currently contains the source for an unfinished overhaul of GameYob after
release 0.5. In particular, this overhaul allows for the emulation of multiple
gameboys, in order to improve wireless link cable emulation by emulating both
gameboys on both systems. It also facilitated the 3DS port (and SDL for
testing purposes).

However, because speed suffered slightly and kinks were never fully worked out,
no public releases were made with this branch. Updates were instead done on the
"v0.5-hotfix" branch to fix issues with GameYob version 0.5.

Also, despite some residual 3DS code in this repository, SteveIce's fork for the
3DS version had far more work done on it. That repository is down now, but
a fork can be found here: https://github.com/ZopharsDomain/GAMEBOYC-3DS-GameYob

The 3DS version had its own kinks, which means you may still prefer to use the
DS version even on a 3DS. It is recommended to install it using the DSiWare CIA
file provided with the newer releases of GameYob DS.

## GameYob v0.5.5-ko

This unofficial homebrew release completes the next repository milestone and
keeps the emulator core and its redistributable homebrew binaries under their
existing licenses. No ROM image or BIOS is included.

### Completed work

- DS/DSi development migrated from legacy devkitARM makefiles to the pinned
  BlocksDS 1.22.2 SDK and Wonderful toolchain, with reproducible container CI.
  The native 3DS target remains on libctru/devkitARM because BlocksDS targets
  Nintendo DS-family binaries rather than native 3DS applications.
- Extensible UTF-8 localization with embedded English, Japanese, and Korean
  menus, English fallback, custom file selection, and editable examples in
  INI, JSON, XML, and YAML; saving settings creates a non-destructive English
  `gameyob_language.ini` template beside `gameyobds.ini`
- Full Galmuri BMP Unicode UI font plus UTF-8/CP949 Korean filename and cheat
  list handling on DS/DSi and native 3DS; native 3DS UTF-16 directory entries
  are converted to UTF-8 without lossy narrowing
- Current native-3DS devkitPro/libctru ABI, public framebuffer and UTF-8 SD
  APIs, stable absolute ROM paths, and NDSP audio with a CSND hardware fallback;
  the user's DSP firmware remains external and is never distributed
- Native 3DS LAN link backend with room discovery, host/client connection,
  link-cable and SGB multiplayer modes, ACK/retry, fragmentation, buffered
  input, state hashes, timeout handling, initial SRAM/RTC synchronization, and a
  bounded offline/no-peer return path
- Reliable local-link restart after a wireless session, isolated secondary
  save state, correct cross-ROM initialization, full-ROM peer fingerprints,
  and native 3DS CGB fast-serial timing
- Stable MBC3/HuC3 real-time clocks with latch-edge, halt, day-carry,
  backward-host-clock, RTC-only cartridge, save, and state handling
- Patched-ROM layout hardening: physical file-size detection, safe
  non-power-of-two bank mirroring, partial-bank fill, the complete standard
  SRAM header table through 128 KiB, and an explicit 8 MiB hardware boundary
- Native 3DS custom PNG/BMP borders, automatic SGB border probing, safer SGB
  attribute commands, and state handling for `JUMP`, `OBJ_TRN`, and `PAL_PRI`
- Completed-frame native 3DS output with direct Off mode plus PICA200/Citro2D
  accelerated Aspect/Full scaling and optional GPU filtering, preventing
  partially updated scanout frames while avoiding per-output-pixel ARM11
  interpolation
- DS, DSi, and native 3DS GBC BIOS selection with exact 0x900-byte validation;
  **Save Settings** records the selected absolute path as `biosfile` in
  `gameyobds.ini`
- Independent DS NR50 SO1/SO2 volume/routing behavior and GameShark `0x8x`
  external-SRAM bank writes
- 2026 maintenance pass: table-driven CRC32, allocation-free DS packet sends,
  hashed DS glyph caching, bounded formatting/path handling, strict cheat-code
  validation, and removal of unnecessary 3DS compiler temporary output

See [language-file documentation](languages/README.md),
[wireless-link design](docs/features/wireless-link.md), and the
[v0.5.5-ko release record](docs/releases/v0.5.5-ko.md).

### User guides / ユーザーガイド / 사용자 가이드

- [English](docs/guides/user-guide.en.md)
- [日本語](docs/guides/user-guide.ja.md)
- [한국어](docs/guides/user-guide.ko.md)

### 완료된 작업

- 기존 devkitARM Makefile에서 고정된 BlocksDS 1.22.2 SDK와 Wonderful
  툴체인으로 DS/DSi 개발 환경을 전환하고 컨테이너 CI를 재현 가능하게 구성.
  BlocksDS는 DS 계열용이므로 네이티브 3DS 대상은 libctru/devkitARM 유지
- 파일 없이도 작동하는 내장 영어·일본어·한국어 메뉴, 영어 원문 대체,
  사용자 파일 선택을 갖춘 UTF-8 다국어 기능과 INI·JSON·XML·YAML 예제;
  설정 저장 시 기존 파일을 덮어쓰지 않는 영어 기본
  `gameyob_language.ini`를 `gameyobds.ini` 옆에 생성
- DS/DSi 및 네이티브 3DS의 전체 Galmuri BMP 유니코드 UI 글꼴,
  UTF-8/CP949 한글 파일명·치트 목록, 3DS UTF-16 파일명의 무손실 UTF-8 변환
- 최신 네이티브 3DS devkitPro/libctru ABI, 공개 프레임버퍼 및 UTF-8 SD API,
  안정적인 ROM 절대 경로와 NDSP 사운드·실기 CSND 대체 경로 적용;
  사용자 소유 DSP 펌웨어는 외부 파일로 유지하며 배포하지 않음
- 방 검색, 호스트/클라이언트 연결, 링크 케이블·SGB 멀티플레이, 재전송,
  분할 전송, 입력 버퍼, 상태 해시, 연결 시간 제한, 초기 SRAM/RTC 동기화를 갖춘
  네이티브 3DS LAN 링크와 네트워크 없음·상대 기기 없음 상태의 제한 시간 복귀
- 무선 링크 사용 뒤에도 정상적으로 다시 시작되는 로컬 링크, 분리된 두 번째
  세이브 상태, 서로 다른 ROM의 올바른 초기화, 전체 ROM 지문 확인 및
  네이티브 3DS GBC 고속 직렬 통신 타이밍
- 래치 0→1 경계, 정지, 날짜 캐리, 호스트 시계 역행, RTC 전용 카트리지,
  세이브·상태 파일을 처리하는 안정된 MBC3/HuC3 실시간 시계
- 실제 파일 크기, 비2제곱 뱅크 미러링, 불완전 뱅크 채움, 128KiB까지의
  표준 SRAM 헤더 및 8MiB 하드웨어 경계를 반영한 패치 ROM 안정화
- 네이티브 3DS 사용자 PNG/BMP 보더, 자동 SGB 보더 탐지, 안전한 SGB 속성
  명령과 `JUMP`·`OBJ_TRN`·`PAL_PRI` 상태 처리
- 빠른 장면 전환 중 불완전한 화면이 표시되지 않는 네이티브 3DS 완성 프레임
  출력, 직접 출력 방식의 끄기 모드, 출력 픽셀별 ARM11 보간을 피하는
  PICA200/Citro2D 기반 비율/전체 화면 확대 및 GPU 필터
- DS·DSi·네이티브 3DS의 GBC BIOS 파일 선택과 정확한 0x900바이트 검사;
  **설정 저장** 시 선택한 절대 경로를 `gameyobds.ini`의 `biosfile`로 기록
- DS NR50의 SO1/SO2 독립 음량·라우팅과 GameShark `0x8x` 외부 SRAM 뱅크 쓰기
- 2026년 유지보수 최적화: 테이블 CRC32, DS 패킷 송신 힙 할당 제거,
  DS 글리프 해시 캐시, 경계가 있는 문자열·경로 처리, 엄격한 치트 코드 검사,
  불필요한 3DS 컴파일 임시 파일 제거

## GameYob v0.5.3-ko

This unofficial homebrew release carries the Korean filename and cheat-list
support forward to the current GameYob overhaul and adds the compatibility work
completed in this repository.

### Completed work

- Korean UTF-8/CP949 ROM filenames, directory names, cheat names, and related
  save/state/printer paths
- Native Nintendo 3DS Super Game Boy border decoding and display, verified in
  Azahar with *Tales of Phantasia: Narikiri Dungeon* (AN6J)
- DMG sprite ordering, horizontal Window clipping, LCD-off white output, and
  CGB background-priority fixes
- Correct 15-bit and 7-bit Game Boy noise-channel LFSR behavior
- DS/DSi NiFi protocol v2 with bounds checks, packet versioning, sequencing,
  checksums, ROM identity, retransmission, input buffering, state hashes, and
  reliable initial SRAM transfer
- Reproducible CI builds for Nintendo DS, Nintendo DSi, and Nintendo 3DS

### 완료된 작업

- 한국어 UTF-8/CP949 ROM 파일명, 폴더명, 치트명과 관련
  세이브·스테이트·프린터 경로 지원
- *테일즈 오브 판타지아 나리키리 던전* (AN6J)으로 Azahar에서 검증한
  네이티브 3DS Super Game Boy 보더 해석 및 표시
- DMG 스프라이트 순서, 가로 방향 Window 잘림, LCD 비활성화 시 흰 화면,
  CGB 배경 우선순위 수정
- Game Boy 노이즈 채널의 15비트/7비트 LFSR 동작 수정
- 길이 검사, 패킷 버전·순번·체크섬·ROM 식별자, 재전송, 입력 버퍼,
  상태 해시, 신뢰성 있는 초기 SRAM 전송을 포함한 DS/DSi NiFi 프로토콜 v2
- Nintendo DS, Nintendo DSi, Nintendo 3DS 재현 가능 CI 빌드

### Downloads

The `v0.5.3-ko` release packages these files in `gameyob.zip`:

- `gameyob.nds`: DS/DS Lite, Nintendo 3DS DS mode, and most flashcards
- `gameyob_dsi.nds`: launchers that explicitly support DSi mode
- `gameyob.3dsx`: native Nintendo 3DS homebrew

### 다운로드

`v0.5.3-ko` 릴리스의 `gameyob.zip`에는 다음 파일이 들어 있습니다.

- `gameyob.nds`: DS/DS Lite, Nintendo 3DS의 DS 모드 및 대부분의 플래시카드용
- `gameyob_dsi.nds`: DSi 모드를 명시적으로 지원하는 런처용
- `gameyob.3dsx`: 네이티브 Nintendo 3DS 홈브루용

## GameYob v0.5.2-ko.1 한글 파일명 지원

GameYob 공식 v0.5.2 안정판을 기반으로 현대 한국어 파일명과 치트 목록 지원을
보강한 비공식 홈브루 빌드입니다.

### 주요 변경 사항

- FAT 긴 파일명 변환을 UTF-8 로케일로 초기화
- 파일 선택기에서 완성형 한글 11,172자와 현대 자모 표시
- UTF-8 및 CP949(Windows ANSI) 치트명, UTF-8 BOM 지원
- 한글 ROM명에 연결되는 세이브, 치트, 스테이트, 프린터 파일 경로 확장
- UTF-8/CP949 문자열을 중간 바이트에서 자르지 않도록 처리
- 확장자가 없는 항목에서 발생하던 잘못된 포인터 접근 수정
- 치트명의 `%`를 포맷 문자열로 오인하던 문제 수정

메뉴 UI 번역은 포함하지 않습니다. 현대 한국어 파일·폴더·치트명 표시에
초점을 둔 빌드이며, Galmuri에 없는 역사적 확장 자모 191자는 `?`로 표시됩니다.

### 다운로드 및 설치

- [Version 0.5.2-ko.1 릴리스](https://github.com/GimoXagros/GameYob/releases/tag/v0.5.2-ko.1)
- `gameyob.nds`: DS/DS Lite, 3DS의 DS 모드와 대부분의 플래시카드에 권장
- `gameyob_dsi.nds`: DSi 모드 실행을 명시적으로 지원하는 런처용

두 NDS 파일은 릴리스의 `gameyob.zip`에 들어 있습니다. 사용 중인
플래시카드에서 필요하면 DLDI 자동 패치를 활성화하세요. 기존 GameYob 설정과
세이브 형식은 그대로 사용합니다.

3DS에서 플래시카드나 DS 모드로 실행한다면 일반판이 가장 호환성이 높습니다.
DSi판은 실제 DSi 모드로 실행될 때만 의미가 있으며, 눈에 띄는 성능 향상을
보장하지 않습니다.

치트 파일은 ROM과 같은 경로에 같은 기본 이름으로 둡니다. 예를 들어
`포켓몬 크리스탈.gbc`의 치트 파일은 `포켓몬 크리스탈.cht`이며,
UTF-8(권장) 또는 CP949로 저장할 수 있습니다.

### 검증

- DS/DSi 두 대상 클린 빌드 성공
- 동일 소스 반복 빌드 SHA-256 일치
- `ndstool` Nintendo logo/header/banner CRC 통과
- 완성형 한글 11,172자 글리프 검사 통과
- CP949 `B0 A1` → Unicode `U+AC00` 매핑 검사 통과

자동 NDS 실행 검증은 테스트 PC에 emucap용 DeSmuME 실행 바이너리가 없어
수행하지 못했습니다. 실제 기기에서는 한글 파일명의 테스트 ROM과 UTF-8/CP949
치트 파일을 한 번 확인해 주세요.

### 소스와 라이선스

- [수정 소스 브랜치](https://github.com/GimoXagros/GameYob/tree/release/v0.5.2-ko.1)
- [상세 릴리스 기록](docs/releases/v0.5.2-ko.1.md)
- GameYob 소스 및 이 수정본: MIT License
- 포함된 Galmuri 파생 비트맵 글꼴: SIL Open Font License 1.1

MIT 및 OFL-1.1 라이선스 전문은 저장소와 `gameyob.zip`에 포함되어 있습니다.
게임 ROM과 BIOS는 포함하지 않습니다.

## Features

- Gameboy, Gameboy Color, Super Gameboy emulation
- Supports Gameboy Color Bios ("boot rom") for custom palettes
- Auto SRAM saving
- Save States
- Cheat Codes
- Remappable controls
- Custom borders
- Scale to fill the screen
- Gameboy Printer emulation
- GBS music playback

## Feature development

- [Native 3DS Super Game Boy borders](docs/features/sgb-borders.md) (verified in Azahar; real-hardware validation pending)
- [DS/DSi wireless link protocol](docs/features/wireless-link.md) (protocol v2 implemented; real-device matrix pending)
- [Emulation regression and compatibility status](docs/features/emulation-compatibility.md)

## TODO

The following work is not completed by `v0.5.5-ko`.

1. Validate raw NiFi on physical DS-to-DS, DSi-to-DSi, and Nintendo 3DS in DS
   mode-to-DS/DSi combinations.
2. Validate native LAN link play on two physical Nintendo 3DS systems. A bridge
   between native 3DS UDP and the `.nds` raw-802.11 transport remains a separate
   design and implementation task.
3. Run the renderer, custom-border, sound-routing, cheat, and long-session
   performance matrix on physical DS/DSi/3DS hardware.
4. Expand the game compatibility matrix, especially games that change WX during
   a scanline, rare cartridge hardware, and more SGB command cases.
5. SNES-side SGB machine code, objects, and audio cannot be fully executed or
   mixed without adding a host-SNES CPU/PPU/APU implementation, which is outside
   GameYob's current Game Boy emulator architecture.
6. Upstream rare-cartridge gaps remain, notably MMM01/MBC4 and incomplete
   hardware validation of MBC7/HuC behavior.

## 할 일

다음 작업은 `v0.5.5-ko`에서 완료되지 않았습니다.

1. DS↔DS, DSi↔DSi, Nintendo 3DS의 DS 모드↔DS/DSi 조합에서 raw NiFi를
   실기로 검증합니다.
2. 실제 Nintendo 3DS 두 대에서 네이티브 LAN 링크를 검증합니다. 네이티브
   3DS UDP와 `.nds` raw-802.11 사이의 브리지는 별도 설계·구현 작업입니다.
3. 실제 DS/DSi/3DS에서 렌더러, 사용자 보더, 사운드 라우팅, 치트 및 장시간
   성능 시험표를 수행합니다.
4. 주사선 도중 WX를 바꾸는 게임, 희귀 카트리지 하드웨어 및 더 많은 SGB
   명령을 포함하도록 게임별 호환성 목록을 확대합니다.
5. SNES 측 SGB 머신 코드·오브젝트·사운드를 완전히 실행하려면 호스트 SNES
   CPU/PPU/APU가 필요하며, 이는 현재 Game Boy 에뮬레이터 구조의 범위를
   벗어납니다.
6. 원본부터 남아 있는 희귀 카트리지의 공백, 특히 MMM01/MBC4와 MBC7/HuC의
   불완전한 실기 검증이 남아 있습니다.

## More info

See the GBAtemp thread for more information:

http://gbatemp.net/threads/gameyob-a-gameboy-emulator-for-ds.343407/
