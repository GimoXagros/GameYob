# GameYob v0.5.2-ko.1 빌드 기록

## 고정한 원본

- GameYob v0.5.2 commit: `c4e9cef971ba88c77ad6010a6517ed5eca35767f`
- devkitARM: `r68-1` (GCC 16.1.0)
- libnds v1.8.1: `62b4973e5005f922da92b0cd05d90604c2346365`
- libfat v1.1.5: `fef8efe371e97b5b2281c6a8f6e4ba6c46f7dd12`
- dswifi v0.4.2: `3b4faeacaee5c3d47037f6862df8735516795e86`
- Galmuri: `71e1cacf1437a11220307120e63e30bc275312d4`

GameYob의 안정판 소스가 현재 libnds 2.x API와 호환되지 않아, 원래 API와 맞는
libnds 1.8.1/libfat 1.1.5를 현재 devkitARM으로 다시 빌드했다.
GCC 16의 newlib syscall 변경에는
`patches/libnds-1.8.1-newlib-syscalls.patch`를 적용했다.
ARM7/ARM9 링크에는 devkitARM에 포함된 레거시
`ds_arm7.specs`/`ds_arm9.specs`를 사용한다.

## 한글 자산 재생성

Python 3에서 다음 명령으로 Galmuri BDF와 Python 표준 CP949 코덱으로부터
바이너리 자산을 생성한다.

```powershell
python tools\generate_korean_assets.py `
  --galmuri-dir toolchain\font-galmuri\dist `
  --output-dir arm9\data
```

생성 결과:

- `hangul_font.bin`: 92,192 bytes
- `cp949_table.bin`: 64,512 bytes

완성형 한글 11,172자, 호환 자모, 현대 한글 조합용 자모를 표시한다.
Galmuri에 없는 역사적 확장 자모 191자는 `?` 대체문자로 표시된다.
일반적인 현대 한국어 파일명과 치트명은 전부 포함된다.

## 빌드

GameYob의 기존 Makefile은 경로에 공백이 있으면 실패하므로 소스와
`toolchain/legacy-sdk`를 공백 없는 임시 경로에 복사한 뒤 빌드한다.

```sh
make clean LIBNDS=/path/to/legacy-sdk
make -j2 gameyob.nds LIBNDS=/path/to/legacy-sdk
make gameyob_dsi.nds LIBNDS=/path/to/legacy-sdk
```

배포 전에 두 NDS 파일을 `ndstool`로 검사하고 SHA-256을 다시 산출한다.
