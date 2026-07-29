# Super Game Boy borders on Nintendo 3DS

The native 3DS renderer now decodes and displays borders transferred by
Super Game Boy enhanced games.

## Implemented

- `CHR_TRN` low and high 128-tile banks
- SNES 4bpp planar tile decoding
- `PCT_TRN` 32x28 tile map and four 16-color palettes
- Horizontal and vertical tile flipping
- Centered 256x224 border output on either 3DS screen
- Screen-position SGB palettes for the 160x144 Game Boy picture
- `MASK_EN` cancel, freeze, black, and color-zero modes
- Safe handling for `SOUND`, `SOU_TRN`, `ATRC_EN`, `TEST_EN`, `ICON_EN`,
  `DATA_SND`, and `DATA_TRN`

The decoder is platform-independent and covered by
`tests/sgbborder_test.cpp`.

## Current limitations

- Automatic SGB-border probing for games started in Game Boy Color mode
  remains disabled because the existing reset/probe path is unreliable.
- Custom PNG/BMP borders in the old native 3DS port are still unfinished.
- SNES-side SGB audio is parsed and its transfer state is retained, but it is
  not mixed into GameYob audio because the emulator does not emulate the SNES
  APU.
- A real 3DS hardware compatibility pass is still required before this can
  be called release-ready.

## Azahar validation

The 3DSX built from commit `b70ee20` was run with Azahar 2125.1.3. The test
cartridge was `Tales of Phantasia Narikiri Dungeon_[AN6J][J].gbc`
(SHA-1 `EF322F4160CEEBD8DA67758EBD73225190AF6D23`). No cartridge image is stored
in this repository.

Observed result:

- the game-provided 256x224 SGB border was decoded and displayed;
- the 160x144 Game Boy picture was centered at the SGB opening
  (48 pixels from the left and 40 pixels from the top);
- the inner picture continued updating after the initial white boot frame;
- Azahar reported approximately 59-60 application frames per second.

This is emulator validation, not a replacement for testing on a Nintendo 3DS.

## Host-side decoder test

```sh
clang++ -std=c++11 -Wall -Wextra -Werror \
  -Iplatform/common/include \
  tests/sgbborder_test.cpp platform/common/sgbborder.cpp \
  -o sgbborder_test
./sgbborder_test
```
