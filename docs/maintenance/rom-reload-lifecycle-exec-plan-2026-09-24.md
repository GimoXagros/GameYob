# ROM reload lifecycle stability execution plan

## Purpose and scope

Investigate the user-reported freeze with a BIOS-like screen after repeated
ROM exit/reload. Reproduce and isolate causes before minimal fixes. Distinguish
related synthetic failures from reproduction of the physical symptom. No new
features, versions, releases, master writes/merges, archive cleanup, or user
ROM/BIOS/save publication. Use synthetic fixtures and isolated writable paths.

## Baseline and delegation

- Repository: `GimoXagros/GameYob`; remote default: `master`.
- Initial local/remote SHA: `1e36db0de67f0b6e4eda732535a422e83804a347`.
- Initial tree clean; one worktree; no open PR or existing named fix branch.
- Work branch: `fix/rom-reload-lifecycle-stability` (created by Sol).
- Desktop package: `26.917.8451.0`, queried via Get-AppxPackage. CLI version
  unavailable: WindowsApps codex.exe execution denied. No bypass attempted.
- Direct collaboration tools support model and reasoning_effort parameters.
  No global or project agent configuration changes are necessary.
- Main `/root`: requested gpt-6-astra/medium; resolved model/effort UNVERIFIED.
  Owns only this plan, scope decisions, coordination and final synthesis.
- `/root/gameyob_sol_engineer`: spawned with gpt-6-sol/high, fork_turns none;
  resolved model/effort UNVERIFIED (tool returns canonical task ID only).
  Sole runtime/header/harness/test/build/CI and Git mutation owner.
- `/root/gameyob_luna_evidence`: spawned with gpt-6-luna/medium, fork_turns none;
  resolved model/effort UNVERIFIED. Initially read-only scenarios/log review;
  after source freeze, owns only the dated maintenance evidence document.
- Reviewer `/root/gameyob_astra_reviewer`: spawned gpt-6-astra/xhigh with
  fork_turns none only after tools confirmed both workers completed.
  Read-only diff/evidence review; resolved model/effort remains UNVERIFIED.
- Tool inventory confirmed Sol and Luna running. At most two active children;
  no child delegation; no Fast mode enabled or requested.

## Phases and acceptance

1. Sol inventories actual lifecycle, build and test boundaries; Luna independently
   defines expected outcomes. Read required installed skills per user roles.
2. Sol runs baseline checks and constructs a production-linked synthetic
   lifecycle harness. Record stubbed boundaries, iterations, resource balance,
   allocation/I/O failure injection, first mismatch and exact revision/tree.
3. Main reviews Sol's evidence and approves bounded fix scope. Sol alone fixes
   production code; preserve save dirty state, teardown ordering and settings.
4. Sol freezes candidate and tests normal lifecycle >=1000 and ASan/LSan >=500,
   supported failpoints, recovery, double unload, BIOS consistency and save
   round trips. Run normal/UBSan/ASan suites, package/repository checks and pinned
   BlocksDS builds. Mark unavailable hardware/IRQ behavior NOT RUN or BLOCKED.
5. Luna independently reconciles evidence and writes the dated report with
   PASS/FAIL/BLOCKED/NOT RUN cells and minimal hardware reproduction procedure.
6. Workers finish; spawn read-only Astra xhigh reviewer against fixed base,
   candidate diff and evidence. Resolve P0/P1 via Sol after reviewer finishes.
7. Sol commits explicit paths and creates/updates a Draft PR. Record exact
   candidate, CI and artifact hashes. No master merge or release.

## Risks and recovery

Do not infer leak from RSS, fragmentation from a single number, or hardware
correctness from helper-only sanitizers. Distinguish cache retention and alias
addresses from live allocations. Never write to real user saves or log from
unsafe IRQ/exception contexts. Existing logs and archived binaries are preserved.
Optimization requires a demonstrated bottleneck after correctness is established.

## Progress

- [x] Read user specification and exec-plan skill; inspect initial Git state.
- [x] Spawn real Sol and Luna children with explicit requested settings.
- [x] Reproduction and independent evidence of related software defects.
- [x] Minimal approved fix and frozen-candidate software validation.
- [x] Evidence report and sequential read-only review, including bounded delta.
- [x] Draft PR and classified outcome; final documentation-tip CI recorded in PR.

## Current outcome

Related defects fixed with source candidate 7c64f2c validated and reviewed;
Draft PR [#6](https://github.com/GimoXagros/GameYob/pull/6) submitted against
master. Original hardware symptom unconfirmed. Final documentation-tip CI and
build artifacts are to be recorded in the PR without further runtime changes.
Classification: RELATED_DEFECT_FIXED_ORIGINAL_SYMPTOM_UNCONFIRMED.
Delegation: PARTIALLY_VERIFIED (real child tasks and requested settings verified;
resolved model/effort UNVERIFIED). No release or master merge authorized.

## Decisions and discoveries

- Baseline production-linked RomFile test (Windows clang++, synthetic ROM/BIOS,
  FileHandle/platform stubs) fails at injected short BIOS read: expected invalid,
  actual loaded=true/biosExists=true; handles return to zero. This is a related
  reproduced defect, not reproduction of the user's physical freeze.
- Luna independently confirmed Gameboy::init chooses PC from previous biosOn
  before initMMU recomputes mapping. Existing helper tests do not cover lifecycle.
- Main approved Sol's bounded change to decide current BIOS policy before PC,
  use that same decision in MMU, and validate initial/final BIOS read positions.
  Preserve saved settings, later state restore, save formats and IRQ behavior.
  Account for the separate gbsLoadSong -> initMMU caller and GBS exclusions.
  BIOS-PC runtime harness and full lifecycle coverage remain pending.
- Production Gameboy/MMU baseline reproduction now fails with expected PC=0000,
  actual PC=0100 and BIOS mapped (exit 3). Candidate passes the same condition.
  The initial harness executes Gameboy init only at session zero; its 1,000
  RomFile cycles are not yet 1,000 manager/Gameboy lifecycle cycles.
- Luna found a source-level NiFi ordering risk (stop after unload) and VBlank
  queue push/IRQ access concern. Sol is investigating execution context and
  ownership; no race or original hardware cause has been established.
- Main approved moving nifiStop to the beginning of mgr_unloadRom (load,
  chooser and exit main-thread paths), removing its later load call. Require
  stop-before-first-destruction order assertions and empty/double unload checks.
  This is lifecycle ordering hardening, not a reproduced hardware race.
- Production manager harness with a stubbed NiFi boundary fails the old unload
  ordering (exit 6; teardown observed before nifiStop). Main also approved the
  adjacent DS nifiStop reorder: existing disableNifi before fragment buffer
  release, with no added IRQ waits. Stub order checks do not verify actual DS
  callback completion or prove the physical symptom's root cause.
- Sol reports 1,000 candidate production-manager cycles PASS under Windows
  Clang normal and ASan. Luna independently confirms assertion coverage:
  GB/GBC synthetic cartridge alternation, BIOS policy/short reads, RAM byte
  roundtrip, double unload, file-handle balance, ten shared-ROM focus swaps,
  one successful chooser switch, and stubbed NiFi stop ordering.
  No actual DS FAT/radio/IRQ/graphics/first frame is tested by these stubs.
  Windows LSan is unsupported; Linux sanitizers and DS builds remain pending.
- A further injected synthetic save-read failure reproduces data loss: loadSave
  accepts unread zero RAM, then saveGame overwrites the original 0x5a sentinel
  (exit 19 on valid retry). Main approved cursor checks around RAM/stored-clock
  reads and closing persistence on load failure, with unchanged file paths and
  formats. Assert the original synthetic file survives failure/save/unload and
  a valid retry restores it. This is another related defect, not the original
  physical freeze. Save write/flush and allocation failure behavior remain
  separate investigation cells.
- Frozen pushed source candidate: `2a40d92c1a5a02b6f42db402bf5b4d7a352cfd7a`;
  runtime commit `e78dc4a`, tests commit `2a40d92`. Sol reports normal and Windows
  ASan manager 1,000 cycles each PASS, including save RAM/clock read failure
  preservation and retry. Core CI run 35907941765 and DS build 35907941930 PASS;
  comprehensive preflight 35908168656 pending. Source remains frozen while
  Luna is authorized to write only the dated stability evidence report.
- Preflight at 2a40d92: Linux normal and ASan/LSan PASS; UBSan FAIL in the test
  FileHandle fwrite adapter on a zero-length/null write. Production io.cpp
  already guards this case. Main authorized test adapter contract parity only
  (no assertion weakening), a new candidate, and complete preflight rerun.
  Keep the original failure as harness evidence, not a production UB claim.
- Final source candidate `43276b32eac64e7f50d99185ac21b8bf034968be`, preflight
  run 35908408519 PASS: Linux normal/UBSan/ASan+LSan each 20/20, including 1,000
  manager cycles; pinned fonts/package checks; two identical clean BlocksDS
  1.22.2 builds. Sol finished implementation handoff. Hardware, allocation and
  persistent write/flush failure, real callbacks/IRQs, fragmentation and first
  frame remain unverified. No measured bottleneck or optimization claimed.
  Durable local evidence: `.codex-tmp/rom-reload-diagnostics/sol-evidence.md`.
- Luna completed the dated evidence report; Sol and Luna completed their turns.
  Main verified both completed statuses, then spawned actual read-only Astra
  reviewer against base/candidate and preserved evidence. No source changes
  or duplicate whole-suite runs are authorized during this review.
- Astra reviewer completed read-only review at 43276b3: no P0/P1 found;
  recommended two P2 initialization fixes. Main resumed Sol only after review
  completed: deterministically initialize autosave metadata on first-save
  failure and BIOS-entry CPU registers, with fresh-object autosave failure
  and synthetic production CPU instruction tests. These are bounded follow-up
  runtime changes; require new source candidate/tests/build identity.
  Reviewer also requested explicit GBS stub/policy1/FF50 limitations and report
  corrections: canonical task IDs verified, resolved model/effort UNVERIFIED;
  delegation PARTIALLY_VERIFIED; fixed binary/device/launcher/mode/cycle count
  in physical reproduction procedure. Luna will amend after next source freeze.
- Follow-up source frozen at `7c64f2cd001c4eb43c52ef30fa569c2c185d1ea9`.
  Constructor autosave metadata and per-instance CPU registers initialized;
  synthetic CPU test mirrors production runEmul context-copy at its boundary.
  Main rejected an unnecessary BIOS-branch global register copy; Sol removed
  it rather than changing runtime to satisfy direct-opcode harness setup.
  Core run 35910111608 PASS; preflight 35910112812 host sanitizers PASS, DS
  build pending. Luna resumed only dated report updates after source freeze.
- Final delta candidate 7c64f2c preflight 35910112812 PASS at exact SHA:
  normal/UBSan/ASan+LSan each 20/20 with 1,000 manager cycles; two identical
  pinned clean DS/DSi builds and local artifact structural check PASS.
  Sol completed handoff; waiting for Luna then targeted reviewer delta check.
- After both workers finished again, the same reviewer completed a bounded
  43276b3 -> 7c64f2c delta review: both P2 addressed, no new P0/P1 found, no
  production allocation/object-size increase. Draft PR approved. Poisoned
  storage test exercises autosave failure but does not directly assert its
  metadata values; pre-fix exit 29 proves BIOS register mismatch, not a separate
  reproduced autosave metadata failure. Sol resumed solely for submission;
  Luna updating only final review wording in her report.
- Draft PR #6 created by Sol; Luna finished final report amendments. Main
  authorized explicit staging of this plan and the dated evidence report only,
  then final documentation-tip preflight and PR evidence update. No master
  merge, release, user-data changes or optimization are part of the handoff.
