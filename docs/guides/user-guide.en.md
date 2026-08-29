# GameYob v0.5.7-ko user guide (English)

## 1. Choose a build

- `gameyob.nds`: recommended for DS, DS Lite, flashcards, and Nintendo 3DS DS mode.
- `gameyob_dsi.nds`: use only with a launcher that explicitly starts DSi mode. It uses the same portable emulator payload and does not guarantee higher emulation speed.
- Nintendo 3DS users can run `gameyob.nds` in DS mode. The experimental native 3DSX build is not included in this release and is preserved only in [`backup/3dsx`](../../backup/3dsx).

No ROM or BIOS is included. Copy legally obtained `.gb`, `.gbc`, or `.gbs` files to the SD card and select one in GameYob. Keep `gameyobds.ini` writable; it is created at the SD root by the console builds.

## 2. Controls and menu

The default DS-family mapping uses the D-pad, A/B, Start, and Select as expected. R opens the menu and L fast-forwards on the default DS mapping. Use **Settings → Button Mapping** to change any binding. Available actions include Menu, Menu/Pause, Save, autofire, fast-forward, reset, and local-link focus swap. The scale action is currently effective only in the DS/DSi builds.

## 3. ROM menu, saves, and suspend

- **State Slot** chooses slot 0–9. **Save State**, **Load State**, and **Delete State** act on that slot.
- **Reset** reboots the current cartridge. **Suspend** writes a resumable state.
- **Exit** saves normally. **Exit without saving** deliberately skips the final save. **Quit to Launcher** returns to the DS/3DS launcher.
- Battery saves use `.sav`; the second local Game Boy uses `.sa2`. State files use `.ys0`–`.ys9`, while suspend uses `.yss`.
- **Autosaving** controls background SRAM writes on DS builds. Always exit normally before removing power or the SD card.

## 4. Settings and language

- **Language** selects embedded English, Japanese, Korean, or Custom. The change is immediate.
- **Select Language File** loads a UTF-8 INI, JSON, XML, YAML, or YML file after Custom is selected.
- **Save Settings** writes `gameyobds.ini` and creates an English `gameyob_language.ini` template beside it if one does not exist. Existing custom templates are never overwritten.
- **Console Output** selects Off, clock, FPS+clock, or debug logging.
- **GB Printer** enables printer emulation. Printed output is written beside the ROM.
- DS-only options include **Rumble Pak**, **GB Camera** (Inner/Outer on supported DSi hardware), and **Autosaving**.

External language details and examples are in [the language-file guide](../../languages/README.md).

## 5. Cheats

Place a cheat file beside the ROM with the same base name: `Game.gbc` uses `Game.cht`. UTF-8 is recommended; CP949 is also accepted for Korean legacy lists. Each line contains a Game Genie code (`AAA-BBB` or `AAA-BBB-CCC`) or an eight-digit GameShark code, its enabled flag, and its display name. Open **Settings → Manage Cheats** to toggle entries, then use **Save Settings** to retain the selection.

## 6. Display, borders, and Game Boy modes

- **Game Screen** chooses the top or bottom screen; **Single Screen** hides the unused console screen.
- DS and DSi builds provide **Scaling** and **Scale Filter**.
- **SGB Borders** enables borders supplied by compatible cartridges. **Custom Border** and **Select Border** load a BMP.
- **Select GBC BIOS** accepts an exact 0x900-byte `.bin` on DS and DSi. Reset or reload the game after selecting it, then choose the desired **GBC Bios** mode. **Save Settings** stores its absolute path as `biosfile` in `gameyobds.ini`. With no selected path, the legacy `gbc_bios.bin` lookup in GameYob's current working directory remains available. The BIOS is optional and not distributed.
- **GBC Mode** chooses GB, automatic, or forced GBC behavior. **SGB Mode** chooses Off, Prefer GBC, or Prefer SGB. **Detect GBA** exposes the GBA-detection flag used by a small number of games.

## 7. Sound and debug options

**Sound Channels** independently enables pulse 1, pulse 2, wave, and noise. **Debug → Sound** is the master switch. DS-only timing switches (**Wait for Vblank**, **Hblank**, **Window**, and **Sound Timing Fix**) should normally remain at their defaults; they are compatibility diagnostics. **ROM Info** shows mapper/ROM/RAM data and **Version Info** shows the exact build revision.

Native 3DS-specific audio diagnostics are documented with the archived build and are outside the active DS/DSi release.

## 8. Local and wireless link

### Local Link

**Linking → Local Link** runs two Game Boy instances on one system. Use **Swap Focus** to control the other instance. Starting local link closes an active wireless session and initializes the second save exactly once. Local link currently uses the same loaded ROM for both instances.

### DS/DSi wireless

**Wireless Link** uses raw local NiFi and does not require Internet access. Choose Host or Client and the same link type on both systems. Keep both devices nearby. DS, DSi, and 3DS DS-mode builds use this backend.

Native 3DS LAN work is deferred with the archived 3DSX build. It cannot join an `.nds` raw-NiFi room directly; see [`backup/3dsx/TODO.md`](../../backup/3dsx/TODO.md).

## 9. RTC and patched ROMs

MBC3 and HuC3 clocks use elapsed host time, preserve the MBC3 halt/day-carry bits, and save clock data after SRAM. RTC-only cartridges with no external RAM are supported. Changing the system clock backwards resets the elapsed-time baseline rather than producing a huge jump.

Patched ROMs are sized from their physical file, not only the often-stale header byte. Non-power-of-two layouts are mirrored safely, standard SRAM headers through 128 KiB are supported, and partial banks read as `0xFF`. The hardware MBC5 limit is 8 MiB; larger or unsupported custom mappers require mapper-specific implementation.

## 10. Troubleshooting

- If a language does not change, select English once, reselect the target language, and save settings. For Custom, verify UTF-8 encoding and unchanged English keys.
- If an emulator replaces Unicode SD filenames with `?` before passing a directory entry to homebrew, GameYob cannot reconstruct those lost characters. Update the emulator or set `autoloadrom=/gb/your Korean filename.gbc` in `gameyobds.ini`; the value is UTF-8 and supports an absolute SD path.
- `gameyob_dsi.nds` requests DSi mode, but the launcher decides whether that mode is actually granted. If it starts in DS mode, performance can be the same as `gameyob.nds`.
- DS raw NiFi does not use the Internet connection. Keep devices nearby and use matching Host/Client and link-type settings.
- If a hack fails, check its mapper, physical size, and RAM header under **Debug → ROM Info**. MMM01/MBC4 and parts of MBC7/HuC hardware remain compatibility limits.
- Do not share ROMs or BIOS files in bug reports. Record the ROM SHA-256, mapper, platform, build revision, and reproduction steps instead.
