# Emulation regression and compatibility status

## Automated coverage

| Area | Implemented behavior | Automated check |
| --- | --- | --- |
| SGB border | CHR/PCT decoding, flips, palettes and masks | `sgbborder_test.cpp` |
| Window | WX clipping, including WX 0-6 and right-edge WX 166 | `gb_render_rules_test.cpp` |
| Sprites | first-ten selection support and DMG X/OAM priority order | `gb_render_rules_test.cpp` |
| CGB priority | LCDC bit 0 cancels BG/window priority | `gb_render_rules_test.cpp` |
| LCD off | native 3DS game area is cleared to white | 3DS build plus manual test required |
| Noise channel | hardware-form 15-bit and 7-bit LFSR, periods 32767/127 | `gb_noise_test.cpp` |
| DS NiFi | packet length, CRC, identity bounds and corruption rejection | `nifi_protocol_test.cpp` |

## Game-level checks

| Game / test | Platform | Result | Notes |
| --- | --- | --- | --- |
| Tales of Phantasia Narikiri Dungeon (AN6J) | Azahar 2125.1.3 | Pass | SGB border and centered picture displayed; 59-60 FPS |

Cartridge images and commercial game data are never committed. Hashes may be
recorded so a lawful local copy can be matched to the test result.

## Still pending

- real DS/DSi/3DS-in-DS-mode wireless-link tests;
- real Nintendo 3DS regression pass for the renderer and sound changes;
- native 3DS UDS/LAN link backend;
- a larger per-game compatibility matrix, especially games that change WX
  during a scanline or depend on SNES-side SGB audio.
