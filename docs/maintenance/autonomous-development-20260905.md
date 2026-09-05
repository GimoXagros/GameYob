# Autonomous development handoff - 2026-09-05

This is an unreleased development record. It does not replace v0.5.9-ko and
does not claim physical-hardware validation.

## Baseline and boundaries

- Baseline `master`: `56005cd96bba83e8dfd40b4ba8248e8ba8ae34f9`
- Work branch: `feature/sgb-host-runtime-next`
- Baseline preflight: Actions run `33938878595`, successful host and DS jobs
- State format: version 8 before and after this work
- Baseline SGB coverage: 65C816 51/256; SPC700 21/256
- Baseline NDS sizes: 710,144 bytes for both `gameyob.nds` and
  `gameyob_dsi.nds`
- Native 3DSX, releases/tags, published archives, `old_releases`,
  `backup/3dsx`, and undocumented cartridge behavior were outside scope.

## A. State-file safety - partial

The validation helpers now describe exact prefix sizes for versions 1-8 and
calculate complete mapper, SGB, and optional host-tail lengths with checked
addition. Synthetic fixtures are truncated at every byte boundary and receive
fixed-seed scalar mutations. Boolean fields, bank values, SGB scalar values,
host magic/version/size, negative versions and size overflow are rejected.

`Gameboy::loadState` performs the complete structural/tail preflight before it
starts writing the large RAM regions and reads the historical prefix length
appropriate for versions 1 and 2. This avoids an additional full RAM copy on
Nintendo DS. Consequently loading is safer but not completely transactional:
actual historical user files, injected seek/read failures at every call, and
an allocation/file-close fault harness remain future work.

## B. SGB 65C816 - partial

`tools/sgb_opcode_coverage.py` derives and checks the inventory. Opcode dispatch
coverage moved from 51/256 to 256/256 cases. Added tested families include:

- immediate ORA, AND, EOR, BIT, CMP, CPX, CPY, ADC and SBC;
- packed-BCD 8-bit ADC/SBC across every valid 00-99 operand pair and carry,
  plus selected 16-bit decimal vectors;
- accumulator ASL, LSR, ROL, ROR, INC and DEC;
- TXY, TYX, TSX, TXS, TCD, TDC, TCS, TSC and XBA, with corrected 16-bit
  TXA/TYA and X-width truncation;
- PHP/PLP, PHB/PLB, PHD/PLD, PHK and PHY/PLY;
- all direct, absolute, long, indirect and stack-relative load/store and
  arithmetic addressing forms, including tested bank and direct-page wrapping;
- memory BIT/TRB/TSB, shifts, rotates, increment and decrement;
- branch, jump, call, return, push-effective-address, BRK/COP, mode-sensitive
  NMI entry, and MVN/MVP block moves.

Every opcode byte now has an explicit implementation and tested dispatch path,
so there are no remaining opcode-byte gaps. This does not complete the CPU:
`run` still budgets instructions rather than real 65C816 cycles, timing
penalties are absent, IRQ is not externally driven, and broad trace/differential
and SGB-program validation remain. Because those stage-B gates are incomplete,
stages C (SPC700) and D (DSP/OBJ) were not started. SPC700 therefore remains
21/256 and the prior DSP/prototype OBJ scope is unchanged.

## E. Rare cartridges - not started

HuC3 MCU mailbox/IR/tone behavior and the MBC7 programming-busy model were not
changed. The existing documented behavior and tests remain intact. Cartridge
types `0x15`-`0x17` remain unknown and are not aliased or guessed.

## F. NiFi portable stress - partial

Portable protocol tests now cover every-byte truncation and corruption, 4,096
fixed-seed mutations, exact/duplicate/stale ACK behavior, 16-bit sequence
wrap, and fragmented duplicate/gap/reorder/invalid-size behavior. A shared
fragment state machine is used by the DS receive path. Duplicate fragments do
not replace the assembly; missing/reordered/malformed sequences release the
buffer and reset bounded state. Existing bounded frame/retry behavior remains.

The portable layer cannot validate raw radio behavior, concurrency in the DS
input ring, or real packet loss. DS-to-DS, DSi-to-DSi and 3DS-in-DS-mode radio
tests therefore remain explicitly pending.

## Verification evidence

- Intermediate core regression run `33939776788` at
  `dbfcdd9fbb9e94271ebcf3f8f529f7fb353dc055`: success.
- Intermediate DS build runs `33939776804` and `33939797570` at the same SHA:
  success, no build-step warnings treated as errors.
- Post-expansion core regression run `33941557822` at
  `3babf6e2438ff7ec2afb545ea2c648ea4f785a52`: success, including all 256
  dispatch cases and emulation/native NMI stack vectors.
- Resulting code-build NDS sizes: 718,336 bytes each (+8,192 bytes from
  baseline). SHA-256: `31aa9b21aa011223fe7ff8b930fd14ab0397345de37bfa78b3270a3204ba78dd`
  for `gameyob.nds`, and
  `b176c1e0952e1c4431c7813ce23506e13d28e70429a18a8d5bf995d6b8583389`
  for `gameyob_dsi.nds`.
- Repository preflight now runs normal, UBSan and AddressSanitizer/leak host
  suites. The final branch preflight and its two clean-build section/map
  comparison are recorded in the Draft PR because this handoff itself precedes
  that final run.
- Local Windows has Python but no compatible `g++`; host C++ results cited here
  come from Ubuntu Actions. Local Python compilation, package validation,
  repository/resource checking and whitespace checks are run separately.

## Next safe starting points

1. Continue stage B with a real cycle budget and per-addressing-mode timing,
   branch/page/direct-page penalties, externally driven IRQ vectors, and
   reference-trace/differential validation. The 256-byte dispatch inventory is
   complete, but it is not a timing-completion claim.
2. Only after the 65C816 timing and validation gates pass, begin isolated
   SPC700 work; only after that begin DSP/prototype OBJ work.
3. Add a file-operation fault-injection harness and licensed/synthetic
   historical state fixtures without duplicating full DS memory.
4. Run the physical NiFi matrix and long-session/state compatibility matrix;
   automated protocol results are not substitutes.
5. Revisit only the documented HuC3 and datasheet-derived MBC7 busy boundaries;
   do not guess undocumented MCU or mapper behavior.
