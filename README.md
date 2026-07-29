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

- [Native 3DS Super Game Boy borders](docs/features/sgb-borders.md) (experimental; hardware validation pending)

## More info

See the GBAtemp thread for more information:

http://gbatemp.net/threads/gameyob-a-gameboy-emulator-for-ds.343407/
