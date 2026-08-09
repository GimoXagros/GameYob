# GameYob v0.5.5-ko

This unofficial homebrew release completes the next repository milestone and
keeps the emulator core and its redistributable homebrew binaries under their
existing licenses. No ROM image or BIOS is included.

## Completed work

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
  APIs, stable absolute ROM paths, and CSND audio with an NDSP fallback;
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
- Completed-frame native 3DS output using the verified centered 160x144 direct
  framebuffer path; unstable native 3DS scaling is excluded from this release
- DS, DSi, and native 3DS GBC BIOS selection with exact 0x900-byte validation;
  **Save Settings** records the selected absolute path as `biosfile` in
  `gameyobds.ini`
- Independent DS NR50 SO1/SO2 volume/routing behavior and GameShark `0x8x`
  external-SRAM bank writes
- 2026 maintenance pass: table-driven CRC32, allocation-free DS packet sends,
  hashed DS glyph caching, bounded formatting/path handling, strict cheat-code
  validation, and removal of unnecessary 3DS compiler temporary output

## 완료된 작업

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
  안정적인 ROM 절대 경로와 실기 CSND 사운드·NDSP 대체 경로 적용;
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
  출력과 검증된 중앙 160x144 직접 프레임버퍼 경로; 불안정한 네이티브 3DS
  확대 기능은 이번 릴리스에서 제외
- DS·DSi·네이티브 3DS의 GBC BIOS 파일 선택과 정확한 0x900바이트 검사;
  **설정 저장** 시 선택한 절대 경로를 `gameyobds.ini`의 `biosfile`로 기록
- DS NR50의 SO1/SO2 독립 음량·라우팅과 GameShark `0x8x` 외부 SRAM 뱅크 쓰기
- 2026년 유지보수 최적화: 테이블 CRC32, DS 패킷 송신 힙 할당 제거,
  DS 글리프 해시 캐시, 경계가 있는 문자열·경로 처리, 엄격한 치트 코드 검사,
  불필요한 3DS 컴파일 임시 파일 제거

See [language-file documentation](languages/README.md),
[wireless-link design](docs/features/wireless-link.md), and the
[v0.5.5-ko release record](docs/releases/v0.5.5-ko.md).

## User guides / ユーザーガイド / 사용자 가이드

- [English](docs/guides/user-guide.en.md)
- [日本語](docs/guides/user-guide.ja.md)
- [한국어](docs/guides/user-guide.ko.md)

## Release / 릴리스

- [Version 0.5.5-ko release](https://github.com/GimoXagros/GameYob/releases/tag/v0.5.5-ko)
- [Download gameyob.zip](https://github.com/GimoXagros/GameYob/releases/download/v0.5.5-ko/gameyob.zip)
- [Detailed release record](docs/releases/v0.5.5-ko.md)
- SHA-256: `fc89a50290bfd9c5ee6cb7137db2c30402edbd1fcd9afbe15afde85fcd6f8365`

The archive contains `gameyob.nds`, `gameyob_dsi.nds`, `gameyob.3dsx`, the
English/Japanese/Korean guides, editable language examples, checksums, and
required license notices. It does not contain a game ROM or BIOS.

압축 파일에는 `gameyob.nds`, `gameyob_dsi.nds`, `gameyob.3dsx`, 영어·일본어·
한국어 가이드, 편집 가능한 언어 예제, 체크섬 및 필수 라이선스 고지가 들어
있습니다. 게임 ROM과 BIOS는 포함하지 않습니다.

## Known limitations

- The native `gameyob.3dsx` build has been confirmed to run ROMs correctly on
  real hardware with the fixed centered 160x144 display, but occasional frame
  drops can still occur during gameplay.
- Native 3DS aspect/full-screen scaling is intentionally unavailable in this
  release. A stable, hardware-verified implementation is planned for
  `v0.5.6-ko`. DS/DSi scaling remains available.

## 알려진 제한 사항

- 네이티브 `gameyob.3dsx`는 실기에서 중앙 160x144 고정 화면으로 ROM이
  정상 구동되는 것을 확인했지만, 게임 중 간헐적인 프레임 드롭이 남아 있습니다.
- 네이티브 3DS의 비율/전체 화면 확대는 이번 릴리스에서 의도적으로 제공하지
  않습니다. 실기에서 안정적으로 검증된 구현을 `v0.5.6-ko` 목표로 진행하며
  DS/DSi판의 확대 기능은 계속 사용할 수 있습니다.

## TODO

The following work is not completed by `v0.5.5-ko`.

1. Reimplement native 3DS aspect/full-screen scaling for `v0.5.6-ko` with a
   hardware-verified presentation path, stable output, and acceptable speed on
   Old 3DS/2DS. The options are intentionally absent from `v0.5.5-ko`.
2. Validate raw NiFi on physical DS-to-DS, DSi-to-DSi, and Nintendo 3DS in DS
   mode-to-DS/DSi combinations.
3. Validate native LAN link play on two physical Nintendo 3DS systems. A bridge
   between native 3DS UDP and the `.nds` raw-802.11 transport remains a separate
   design and implementation task.
4. Run the renderer, custom-border, sound-routing, cheat, and long-session
   performance matrix on physical DS/DSi/3DS hardware.
5. Expand the game compatibility matrix, especially games that change WX during
   a scanline, rare cartridge hardware, and more SGB command cases.
6. SNES-side SGB machine code, objects, and audio cannot be fully executed or
   mixed without adding a host-SNES CPU/PPU/APU implementation, which is outside
   GameYob's current Game Boy emulator architecture.
7. Upstream rare-cartridge gaps remain, notably MMM01/MBC4 and incomplete
   hardware validation of MBC7/HuC behavior.

## 할 일

다음 작업은 `v0.5.5-ko`에서 완료되지 않았습니다.

1. `v0.5.6-ko` 목표로 네이티브 3DS의 비율/전체 화면 확대를 다시 구현하고
   Old 3DS/2DS 실기에서 화면 안정성과 충분한 속도를 검증합니다. 해당 옵션은
   `v0.5.5-ko`에서 의도적으로 제외했습니다.
2. DS↔DS, DSi↔DSi, Nintendo 3DS의 DS 모드↔DS/DSi 조합에서 raw NiFi를
   실기로 검증합니다.
3. 실제 Nintendo 3DS 두 대에서 네이티브 LAN 링크를 검증합니다. 네이티브
   3DS UDP와 `.nds` raw-802.11 사이의 브리지는 별도 설계·구현 작업입니다.
4. 실제 DS/DSi/3DS에서 렌더러, 사용자 보더, 사운드 라우팅, 치트 및 장시간
   성능 시험표를 수행합니다.
5. 주사선 도중 WX를 바꾸는 게임, 희귀 카트리지 하드웨어 및 더 많은 SGB
   명령을 포함하도록 게임별 호환성 목록을 확대합니다.
6. SNES 측 SGB 머신 코드·오브젝트·사운드를 완전히 실행하려면 호스트 SNES
   CPU/PPU/APU가 필요하며, 이는 현재 Game Boy 에뮬레이터 구조의 범위를
   벗어납니다.
7. 원본부터 남아 있는 희귀 카트리지의 공백, 특히 MMM01/MBC4와 MBC7/HuC의
   불완전한 실기 검증이 남아 있습니다.
