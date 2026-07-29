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

The decoder is platform-independent and covered by
`tests/sgbborder_test.cpp`.

## Current limitations

- Automatic SGB-border probing for games started in Game Boy Color mode
  remains disabled because the existing reset/probe path is unreliable.
- Custom PNG/BMP borders in the old native 3DS port are still unfinished.
- SGB sound and the less commonly used SNES-side commands are outside this
  change.
- A real 3DS hardware compatibility pass is still required before this can
  be called release-ready.

## Host-side decoder test

```sh
clang++ -std=c++11 -Wall -Wextra -Werror \
  -Iplatform/common/include \
  tests/sgbborder_test.cpp platform/common/sgbborder.cpp \
  -o sgbborder_test
./sgbborder_test
```
