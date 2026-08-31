# SGB host runtime coverage

The SGB host runtime is deliberately isolated from ordinary GB/GBC execution
and remains an incomplete, fault-explicit compatibility layer. This inventory
prevents the current foundation from being described as a complete SNES host.

## 65C816

- Implemented opcode cases: 51 of 256; 205 remain.
- Present coverage: selected status/mode operations, immediate loads, selected
  absolute/long loads and stores, direct/long jumps and calls, returns,
  branches, transfers, stack pushes/pulls, WAI, and STP.
- Width handling exists for the implemented accumulator/index operations.
- Unsupported opcodes enter an explicit fault state; they are never silently
  treated as NOP.
- Missing: broad addressing-mode and arithmetic coverage, decimal arithmetic,
  block moves, full interrupt behavior, and a cycle-accurate timing model.

`sgb_host_test.cpp` checks bounded memory, a transferred CPU program, explicit
unsupported-opcode faulting, SPC700 transfer, and prototype object decoding.

## SPC700 and DSP

- Implemented SPC700 opcode cases: 21 of 256; 235 remain.
- Present coverage: a small load/store/branch/transfer subset and explicit
  sleep/stop faults, sufficient only for the current transferred-program test.
- BRR decoding, voice pitch/volume, and separate host PCM output are present.
- Missing: general instruction/addressing/flag coverage, timers and I/O,
  ADSR/GAIN envelopes, accurate KON/KOFF/release, noise/modulation, echo,
  feedback, and FIR filtering.

## PPU and commands

- Existing palette, attribute, border, mask, and multiplayer paths remain.
- `DATA_SND`, `DATA_TRN`, `JUMP`, `SOUND`, `SOU_TRN`, and `CHR_TRN` feed
  bounded host state.
- Packet headers now have portable validation for all 32 command IDs and packet
  counts 0-7; `DATA_SND` remains bounded to its 11-byte payload.
- Prototype OBJ data is decoded in the isolated host PPU, but final DS
  framebuffer composition and priority are not implemented.
- Retail SGB `OBJ_TRN` behavior remains a no-op by policy.

## Save state and completion status

Host state uses explicit field serialization. Save-state version 8 adds MBC7
and HuC1 data without changing the existing host-state payload. A host runtime
cannot be marked complete until CPU, APU, DSP, OBJ composition, timing, state,
and non-SGB regression requirements in the project TODO are all satisfied.
