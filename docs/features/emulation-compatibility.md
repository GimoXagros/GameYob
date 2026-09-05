# Emulation regression and compatibility status

## Automated coverage

| Area | Implemented behavior | Automated check |
| --- | --- | --- |
| SGB border | CHR/PCT decoding, flips, palettes and masks | `sgbborder_test.cpp` |
| Window | WX clipping (0-6 and 166), hidden WX 167, per-line hide/resume, WY and enable gating | `gb_render_rules_test.cpp` |
| Sprites | first-ten selection support and DMG X/OAM priority order | `gb_render_rules_test.cpp` |
| CGB priority | LCDC bit 0 cancels BG/window priority | `gb_render_rules_test.cpp` |
| LCD off | native 3DS game area is cleared to white | 3DS build plus manual test required |
| Noise channel | hardware-form 15-bit and 7-bit LFSR, periods 32767/127 | `gb_noise_test.cpp` |
| DS NiFi | packet length/CRC, every-byte truncation/corruption, fixed-seed mutation, ACK/sequence wrap and ordered fragment assembly | `nifi_protocol_test.cpp` |
| RTC | MBC3 rollover/carry/halt and HuC3 minute/day/year rollover | `rtc_test.cpp` |
| Patched ROM layout | physical/non-power-of-two banks, 8 MiB boundary, RAM headers through 128 KiB | `rom_layout_test.cpp` |
| MMM01 | power-on menu mapping, ROM/RAM masks, banking, multiplex and locks | `mmm01_test.cpp` |
| SGB host foundation | bounded WRAM access, CPU faults, APU program transfer and prototype OBJ decode | `sgb_host_test.cpp` |
| Save states | exact version 1-8 prefix/tail sizing, every-byte truncation and fixed-seed scalar mutation | `state_validation_test.cpp` |
| SGB packet protocol | all command IDs, packet-count validity and bounded DATA_SND payload | `sgb_protocol_test.cpp` |
| MBC7 | serial EEPROM protection/read/write/erase/all commands and transaction-state restore | `mbc7_eeprom_test.cpp` |
| HuC1 | RAM/IR selection and ROM/RAM bank masks | `huc1_rules_test.cpp` |
| Localization | INI/JSON/XML/YAML, UTF-8 Korean/Japanese and English fallback | `localization_test.cpp` |
| Unicode UI | UTF-8 column count and boundary-safe clipping | `text_test.cpp` |
| DS audio | independent NR50 SO1/SO2 volume/routing model | build plus hardware test required |
| Cheats | GameShark `0x8x` external-SRAM bank writes and hexadecimal validation | build plus game test required |
| 3DS LAN | shared packet protocol, discovery/link backend | protocol test plus two-system test required |

## Game-level checks

| Game / test | Platform | Result | Notes |
| --- | --- | --- | --- |
| Tales of Phantasia Narikiri Dungeon (AN6J) | Azahar 2125.1.3 | Pass | SGB border and centered picture displayed; 59-60 FPS |
| Tales of Phantasia Narikiri Dungeon (AN6J) | physical Nintendo 3DS, NDS build in DS mode | Pass (user-reported) | SGB border reset, repeated ROM loading and RTC accepted in the v0.5.8-ko development line; v0.5.9-ko touch/mapper changes need a new physical pass |

Cartridge images and commercial game data are never committed. Hashes may be
recorded so a lawful local copy can be matched to the test result.

## Still pending

- User-reported Korean Pokemon Silver model-warning screen with Detect GBA On;
  Off boots normally. Exact ROM/build/BIOS/suspend-state combination and root
  cause are not established. This is not attributed to DSpico or GBARunner3.
- Real legacy `.ys*` fixtures remain untested. Synthetic versions 1-8 and
  variable tails are preflighted before large-array mutation, but loading does
  not duplicate all emulator RAM and is not a fully transactional operation.

- real DS/DSi/3DS-in-DS-mode wireless-link tests;
- deferred native Nintendo 3DS regression pass for renderer/sound changes;
- deferred native 3DS-to-3DS LAN link testing and a native-3DS/`.nds` bridge;
- a larger per-game compatibility matrix, especially commercial games that change WX
  during a scanline or depend on SNES-side SGB audio.
- complete SGB host 65C816/SPC700/DSP timing and final prototype OBJ composition;
- legacy cartridge types 0x15-0x17, whose hardware behavior remains undocumented/unknown;
- complete physical-hardware validation of MBC7/HuC behavior; see
  [`rare-cartridge-validation.md`](rare-cartridge-validation.md).
