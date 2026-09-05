# GameYob

<p align="center">
  <img src="logo.png" alt="GameYob logo" width="480">
</p>

This unofficial homebrew release completes the next repository milestone and
keeps the emulator core and its redistributable homebrew binaries under their
existing licenses. No ROM image or BIOS is included.

Active development now targets `gameyob.nds` and `gameyob_dsi.nds`. Nintendo
3DS users can run the NDS build in DS mode. Native 3DSX development is paused;
its executable and detailed follow-up list are preserved in
[`backup/3dsx`](backup/3dsx).

현재 개발은 `gameyob.nds`와 `gameyob_dsi.nds`를 대상으로 진행합니다.
Nintendo 3DS에서도 NDS판을 DS 모드로 실행할 수 있습니다. 네이티브 3DSX
개발은 잠시 보류하며 실행 파일과 상세 후속 작업은
[`backup/3dsx`](backup/3dsx)에 보존합니다.

## Completed work

- DS/DSi touch-driven menu and ROM/File Chooser navigation, including
  select-before-activate safety, value adjustment, category navigation,
  scrolling, exit, and temporary Bottom Screen UI routing. The saved Game
  Screen setting, existing configurable Touch binding, and all physical-button
  controls are preserved.
- MBC7 dual-enable register access, accelerometer latching, battery-backed
  serial EEPROM commands and transaction save state; corrected HuC1 RAM/IR and
  independent ROM/RAM bank selection. These paths have portable software
  tests, while physical rare-cartridge validation remains pending.
- Expanded per-line WX hide/resume and SGB packet-header/payload boundary
  regression coverage. The incomplete SGB host CPU/APU/DSP scope is now
  quantified rather than presented as complete.
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
- Independent MMM01 mapper support for cartridge types `0x0B`, `0x0C`, and
  `0x0D`, including the power-on menu mapping, mapping lock, ROM/RAM masks,
  multiplex mode, SRAM/autosave, safe physical-bank normalization, and
  backward-compatible versioned save states
- A separate, SGB-only host runtime foundation for bounded SNES WRAM/PPU/APU
  transfers: `DATA_SND`, `DATA_TRN`, `JUMP`, `SOU_TRN`, `SOUND`, `CHR_TRN`, and
  prototype `OBJ_TRN` now feed host state; unsupported 65C816/SPC700 opcodes
  fault explicitly, and generated host PCM is kept separate from the four GB
  channels and mixed by the DS audio hardware
- Unreleased development keeps state version 8 while preflighting complete
  versioned mapper/SGB tails, expands 65C816 opcode coverage from 51 to 109 of
  256, and hardens DS NiFi fragmented receives. This is partial development,
  not a new release or completed SGB/radio implementation.

## 완료된 작업

- DS/DSi 메뉴와 ROM/파일 선택기의 터치 조작: 오작동을 막는 선택 후 실행,
  값 변경, 분류 이동, 스크롤, 종료 및 실제 아래 화면으로의 임시 UI 배치를
  구현했습니다. 저장된 게임 화면 값, 기존 설정 가능한 Touch 매핑 및 모든
  물리 버튼 조작은 그대로 유지합니다.
- MBC7 이중 활성화, 가속도 래치, 배터리 저장형 직렬 EEPROM 명령과 거래 상태
  저장 및 HuC1 RAM/IR·독립 ROM/RAM 뱅킹을 보강했습니다. 소프트웨어 자동
  시험은 추가했지만 희귀 실물 카트리지 검증은 아직 남아 있습니다.
- 주사선별 WX 숨김/재개와 SGB 패킷 헤더·페이로드 경계 회귀시험을 확대하고,
  미완성 SGB 호스트 CPU/APU/DSP 범위를 수치로 문서화했습니다.
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
- 카트리지 형식 `0x0B`·`0x0C`·`0x0D`의 독립 MMM01 지원: 전원 투입 시
  메뉴 매핑, 매핑 잠금, ROM/RAM 마스크, 멀티플렉스 모드, SRAM·자동 저장,
  실제 ROM 뱅크 경계 처리 및 이전 상태 파일과 호환되는 버전별 상태 저장
- SGB에서만 생성되는 독립 호스트 실행 기반: `DATA_SND`·`DATA_TRN`·`JUMP`·
  `SOU_TRN`·`SOUND`·`CHR_TRN` 및 프로토타입 `OBJ_TRN`을 SNES WRAM·PPU·
  APU 상태에 연결하고, 미지원 65C816/SPC700 명령은 명시적으로 중단하며,
  호스트 PCM은 GB 4채널과 분리해 DS 사운드 하드웨어에서 함께 출력

See [language-file documentation](languages/README.md),
[wireless-link design](docs/features/wireless-link.md),
[SGB host coverage](docs/features/sgb-host-coverage.md),
[rare-cartridge validation](docs/features/rare-cartridge-validation.md), and
the [v0.5.9-ko release record](docs/releases/v0.5.9-ko.md).

## User guides / ユーザーガイド / 사용자 가이드

- [English](docs/guides/user-guide.en.md)
- [日本語](docs/guides/user-guide.ja.md)
- [한국어](docs/guides/user-guide.ko.md)

## Release / 릴리스

- [Version 0.5.9-ko release](https://github.com/GimoXagros/GameYob/releases/tag/v0.5.9-ko)
- [Download gameyob.zip](https://github.com/GimoXagros/GameYob/releases/download/v0.5.9-ko/gameyob.zip)
- [Detailed release record](docs/releases/v0.5.9-ko.md)

The archive contains `gameyob.nds`, `gameyob_dsi.nds`, the English/Japanese/
Korean guides, editable language examples, checksums, and required license
notices. It does not contain a game ROM, BIOS, or native 3DSX executable.

Published release archives through `v0.5.9-ko` are preserved in
[`old_releases`](old_releases). The earlier native 3DSX binary is preserved
separately in [`backup/3dsx`](backup/3dsx). `v0.5.9-ko` is a DS/DSi-focused
release, and its NDS build can also run on Nintendo 3DS in DS mode.

압축 파일에는 `gameyob.nds`, `gameyob_dsi.nds`, 영어·일본어·한국어 가이드,
편집 가능한 언어 예제, 체크섬 및 필수 라이선스 고지가 들어 있습니다. 게임
ROM, BIOS 및 네이티브 3DSX 실행 파일은 포함하지 않습니다.

`v0.5.9-ko`까지 배포한 압축 파일은 [`old_releases`](old_releases)에
보존하고, 이전 네이티브 3DSX 실행 파일은 [`backup/3dsx`](backup/3dsx)에
별도로 보존합니다. `v0.5.9-ko`는 DS/DSi 중심 릴리스이며 NDS판은 Nintendo
3DS의 DS 모드에서도 실행할 수 있습니다.

## Known limitations

- Korean Pokemon Silver: a user reports a model-warning screen with **Detect
  GBA On**, and normal startup with it Off. Exact ROM/build/BIOS/state matching
  and root cause are unverified; do not describe this as fixed or a hardware
  restriction. Detect GBA is an identification flag, not a performance mode.

- Native 3DSX development, performance work, scaling, and LAN validation are
  deferred together. See [`backup/3dsx/TODO.md`](backup/3dsx/TODO.md).

## 알려진 제한 사항

- 한국어판 포켓몬 은: **GBA 감지 On**에서 기종 경고가 나오고 Off에서는 정상
  실행된다는 사용자 보고가 있습니다. 정확한 ROM·빌드·BIOS·상태 파일 대조와
  원인 확인은 아직 없으며 수정 완료나 실제 하드웨어 제한으로 단정하지 않습니다.
  GBA 감지는 성능 향상 모드가 아니라 기종 식별 플래그입니다.

- 네이티브 3DSX 개발, 성능 개선, 화면 확대 및 LAN 검증은 하나의 후속 작업으로
  보류했습니다. 상세 목록은
  [`backup/3dsx/TODO.md`](backup/3dsx/TODO.md)를 참고하십시오.

## TODO

Build/test commands and maintenance boundaries: [BUILDING.md](BUILDING.md).
Evidence and exclusions: [2026-09-03 preflight report](docs/maintenance/repository-preflight-20260903.md).
Items 1-4 are validation/measurement work (failures can require fixes); item 5
is partial implementation, item 6 mixes implementation/physical validation and
unknown specifications, and item 7 is deferred. Existing user-approved RTC,
ROM reload and border behavior is retained as evidence, not revoked by a new
regression checklist. The Detect GBA report above remains an investigation.

The following DS/DSi-focused work remains after `v0.5.9-ko`.

1. Validate raw NiFi on physical DS-to-DS, DSi-to-DSi, and Nintendo 3DS in DS
   mode-to-DS/DSi combinations.
2. Run the touch-menu/chooser, renderer, custom-border, sound-routing, cheat,
   repeated-ROM-load, RTC, MBC7/HuC, and long-session performance matrix on
   physical DS/DSi hardware and Nintendo 3DS in DS mode.
3. Compare DS-mode and DSi-mode performance on supported launchers and document
   when `gameyob_dsi.nds` provides a measurable benefit.
4. Expand the physically observed game compatibility matrix, especially games
   that change WX during a scanline and depend on more SGB command cases.
5. Complete the SGB host runtime introduced in `v0.5.8-ko`: implement the
   remaining 147 65C816 and 235 SPC700 opcode cases and timing coverage,
   complete DSP
   envelopes/echo, and composite prototype host OBJ pixels into the final DS
   output. The current bounded execution and separate PCM path are functional
   foundations, but they are not a complete host-SNES implementation. Retail
   SGB revisions intentionally treat `OBJ_TRN` as a no-op.
6. Legacy cartridge type values `0x15`-`0x17` remain undocumented/unknown and
   are deliberately not guessed or aliased. Validate the new MBC7 EEPROM/tilt
   and HuC1 paths plus existing HuC3 behavior on physical cartridges; MBC7 busy
   timing and the detailed HuC3 MCU/IR/tone model remain incomplete.
7. Native 3DSX work is deferred as one grouped task; see
   [`backup/3dsx/TODO.md`](backup/3dsx/TODO.md).

## 할 일

빌드·검사 명령과 정비 범위는 [BUILDING.md](BUILDING.md)를 참고하십시오.
검사 근거와 미실행 범위는 [2026-09-03 정비 보고서](docs/maintenance/repository-preflight-20260903.md)에 기록했습니다.
1~4번은 검증·측정(실패 시 수정), 5번은 부분 구현, 6번은 구현·실물 검증·사양 불명,
7번은 보류 항목입니다. 기존 사용자 승인 RTC·ROM 재실행·보더 결과를 취소하는
것이 아니라 최신 변경 이후 회귀검증이 필요하다는 뜻입니다. 위 GBA 감지 보고는
원인 조사 단계입니다.

`v0.5.9-ko` 이후 남은 DS/DSi 중심 작업입니다.

1. DS↔DS, DSi↔DSi, Nintendo 3DS의 DS 모드↔DS/DSi 조합에서 raw NiFi를
   실기로 검증합니다.
2. 실제 DS/DSi 및 Nintendo 3DS의 DS 모드에서 터치 메뉴·파일 선택기, 렌더러,
   사용자 보더, 사운드 라우팅, 치트, ROM 반복 실행, RTC, MBC7/HuC 및 장시간
   성능 시험표를 수행합니다.
3. 지원 런처에서 DS 모드와 DSi 모드 성능을 비교하고 `gameyob_dsi.nds`가
   실제 이점을 제공하는 조건을 문서화합니다.
4. 주사선 도중 WX를 바꾸거나 더 많은 SGB 명령에 의존하는 게임을 실제
   기기에서 확인하여 게임별 호환성 목록을 확대합니다.
5. `v0.5.8-ko`에서 추가한 SGB 호스트 실행 계층을 완성합니다. 남은
   65C816 147개·SPC700 235개 opcode case와 타이밍, DSP 엔벌로프·에코 및 프로토타입 호스트 OBJ의
   최종 DS 화면 합성이 필요합니다. 현재 경계가 있는 실행과 독립 PCM 경로는
   실제 기반이지만 완전한 호스트 SNES 구현은 아닙니다. 일반 판매 SGB의
   `OBJ_TRN` 무동작은 하드웨어 사양대로 유지합니다.
6. 기존 카트리지 형식 `0x15`-`0x17`은 동작 사양이 알려지지 않아 다른 매퍼로
   추측하거나 대체하지 않습니다. 새 MBC7 EEPROM·기울기 및 HuC1 경로와 기존
   HuC3를 실물 카트리지로 검증해야 하며, MBC7 busy 시간과 상세 HuC3
   MCU·IR·톤 모델도 남아 있습니다.
7. 네이티브 3DSX 관련 작업은 하나의 보류 항목으로 묶었습니다. 상세 목록은
   [`backup/3dsx/TODO.md`](backup/3dsx/TODO.md)를 참고하십시오.
