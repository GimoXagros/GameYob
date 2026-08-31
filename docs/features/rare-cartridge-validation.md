# Rare cartridge implementation and validation

This record separates specification review, automated software checks, and
physical-cartridge validation. No commercial ROM image or BIOS is stored in
the repository.

## Evidence used

- [Pan Docs cartridge type table](https://github.com/gbdev/pandocs/blob/master/src/The_Cartridge_Header.md)
- [Pan Docs MBC7 description](https://github.com/gbdev/pandocs/blob/master/src/MBC7.md)
- [Pan Docs HuC1 description](https://github.com/gbdev/pandocs/blob/master/src/HuC1.md)
- [Pan Docs HuC3 description](https://github.com/gbdev/pandocs/blob/master/src/HuC3.md)

These public hardware notes are the primary implementation reference. Other
emulators may be used for comparison, but matching one emulator is not treated
as proof of hardware correctness.

## MBC7 (`0x22`)

v0.5.9-ko replaces the empty EEPROM placeholder with a bounded 93LC56-style
serial state machine. It implements the two required enable registers,
seven-bit ROM banking, the documented accelerometer erase/latch sequence,
register reads, EWEN/EWDS, READ, WRITE, ERASE, WRAL, and ERAL. EEPROM contents
use the existing battery-save storage. Save-state version 8 records the serial
transaction, write-enable, access-enable, and latched sensor state explicitly.

`mbc7_eeprom_test.cpp` checks write protection, word read/write/erase,
write-all, erase-all, command boundaries, and state restoration. DS touch
coordinates remain the motion source only while a game is running; Touch Menu
consumes touch only while a menu or chooser owns the UI.

Programming-busy delay and real sensor noise/timing are not modeled. Physical
MBC7 cartridge validation is pending.

## HuC1 (`0xFF`)

HuC1 no longer uses the MBC1 mode register. RAM/IR selection at `0000-1FFF`,
six-bit ROM selection, independent two-bit RAM selection, ignored
`6000-7FFF` writes, and the documented IR register values are represented.
There is no physical IR receiver backend, so reads report no incoming light.
`huc1_rules_test.cpp` checks the selection and masking rules. Save-state
version 8 preserves IR selection/output state.

Physical HuC1 cartridge and two-system IR validation is pending.

## HuC3 (`0xFE`)

Existing ROM/RAM selection, elapsed-host-time RTC rollover, persistence, and
clock rollback handling remain covered by `rtc_test.cpp` and the DS build.
Pan Docs describes a more detailed MCU mailbox, RTC memory window, IR, and tone
generator than the current compatibility implementation. Those MCU/IR/tone
details and physical cartridge behavior remain pending; this release does not
claim complete HuC3 hardware emulation.

## Legacy values `0x15`-`0x17`

The current Pan Docs cartridge type table has no definitions for these values,
and no reproducible register map or licensed physical-cartridge evidence was
found. GameYob therefore keeps them explicitly unknown. They are not aliased
to MBC5 or any other mapper. Implementation remains intentionally blocked on
credible hardware evidence.

## Physical validation checklist

- MBC7: boot, ROM banks including bank 0, EEPROM persistence, every programming
  command, ready/busy polling, X/Y direction and repeated latch behavior.
- HuC1: ROM/RAM banks, battery persistence, IR transmit/receive with two units.
- HuC3: boot, RAM, clock read/write/event behavior, long elapsed time, backward
  host clock, IR, and tone generation.
- Record cartridge revision, platform, GameYob revision, exact steps, and ROM
  SHA-256 without distributing cartridge data.
