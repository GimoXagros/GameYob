# SGB, printer and video prerelease execution plan

## Purpose and scope

Investigate and minimally fix SGB-border/GBC fast-forward slowdown, Game Boy
Printer emulation/output, and intermittent video flicker. Preserve the previous
ROM-reload stabilization. Publish a NEW beta prerelease only after all three
have cause-linked fixes and meaningful regression evidence, integrated checks,
read-only review and exact-commit packaging verification.

No master merge, existing PR closure, tag movement, release/asset replacement,
archive cleanup, user ROM/BIOS/save/output mutation or publication. No new SGB
CPU/DSP features or native 3DSX work. Synthetic writable fixtures only.

## Confirmed baseline

- BASE: `763f6bfab8d7f3bed164976277516954141c1618` (stable branch/PR #6).
- Remote master `1e36db0de67f0b6e4eda732535a422e83804a347` lacks six prior
  stabilization commits. Existing PR #6 remains OPEN/DRAFT and unchanged.
- Source worktree clean/synced; no AGENTS.md found in repository/parent.
- Exact-BASE preflight 35911092936, core 35911086494 and DS build 35911086437
  PASS. Preflight includes normal/UBSan/ASan+LSan, lifecycle 1,000 cycles,
  resources/package and identical clean BlocksDS 1.22.2 builds. Reuse these
  baseline results rather than repeat an unchanged whole suite.
- User now reports the prior freeze resolved (USER_REPORTED); personal tested
  binary SHA is unknown. Do not retroactively hardware-certify BASE.
- Latest stable v0.5.10; no v0.5.11 tags when inspected. Proposed new version
  v0.5.11-beta.1, subject to a fresh collision check before tagging.
- gh 2.96.0 authenticated repository push/admin permissions. Current workflows
  have no tag/release triggers; publication will be explicit and gated.

## Delegation and ownership

At most two active child tasks, no redelegation, no Fast mode. Requested model
and effort are passed via actual collaboration tool schema; canonical task IDs
are tool-confirmed, resolved model/effort UNVERIFIED unless runtime exposes it.
No global settings/auth/security changes. Model usage savings unmeasured.

- Main `/root`, requested Astra/medium: coordination and this plan only.
- `/root/integration_release_specialist`, requested Sol/medium: baseline,
  worktrees, shared code/build/CI, integration index/branch and all remote Git,
  PR/tag/release operations. Stage 1 complete; resume after specialist handoffs.
- `/root/video_sgb_specialist` (Sol/high): DS gbgfx and SGB-dedicated code/tests. Separate
  reproduction evidence and commits for border/fast-forward and flicker.
- `/root/printer_specialist` (Sol/high): gbprinter and printer-dedicated headers/tests.
- `/root/evidence_specialist` (Luna/medium): independent expected results/log review and
  explicitly assigned EN/JA/KO documents; no runtime/test/build edits.
- Final reviewer (Astra/xhigh): once after freeze; targeted follow-up only if
  changed. Read-only, no builds duplicated or edits.

Worktrees (all initially BASE):

- `../GameYob-integration-prerelease`, `fix/sgb-printer-video-prerelease`.
- `../GameYob-video-sgb`, `work/video-sgb-prerelease`.
- `../GameYob-printer`, `work/printer-prerelease`.

Shared gameboy/gbmanager/mmu/menu/io/sound/shared headers/CI/Makefiles belong only
to integration. Specialists hand off precise proposed shared diffs and expected
behavior; no simultaneous edits. Each specialist may commit only its worktree;
only integration pushes/remotes/releases. Preserve all old worktrees/logs.

## Phases and acceptance

1. Confirm baseline/permissions, isolate worktrees, finish integration checkpoint.
2. Run video and printer specialists in parallel. Read assigned installed skills
   themselves. Reproduce first divergences with production-linked synthetic
   inputs; separate hardware observations, stubs and source hypotheses. Read
   official printer protocol with revision/source; preserve save/output safety.
3. As a slot frees, evidence specialist independently checks expectations and
   drafts EN/JA/KO documentation. Do not equate helper-only tests with features.
4. Resume integration to apply specialist commits sequentially and shared changes.
   Define border/fast-forward performance tolerance before measurements; separate
   guest frames/cycles from displayed frames/waits. No invented hardware speed.
5. Freeze candidate/version/docs; run integrated cross-cases, legacy lifecycle,
   normal/UBSan/ASan+LSan, package/resources and pinned DS/DSi clean builds.
6. Read-only Astra review; resolve P0/P1 and reproduced new regressions via owners.
   If sources change, targeted review and affected tests; final C must be tested.
7. Verify fresh tag collision check and exact C/tag/version. Two clean builds
   under final tag/version conditions. Inspect NDS/banner/DLDI/TWL/assets and
   package licensing/provenance. Create draft prerelease, latest=false; upload
   versioned ZIP, SHA256SUMS, notes and manifest, then verify all before publish.
8. Re-query published prerelease and tag/commit/assets; publicly re-download and
   verify hashes/package. Confirm stable Latest retained. No needless additional
   approval if all authorized gates pass; otherwise preserve exact blockers/Draft.

## Risks and unknowns

User supplied setup: 3DS running DS mode with DSpico. General affected ROM:
Tales of Phantasia Narikiri Dungeon Korean v1.0 (team Mupung, 260911); specific
SGB-border affected ROM: Castlevania Legends Korean (260102). User supplied both
original paths in the parent workspace. Video owns read-only hash/header inventory
and shares facts; no duplicated inventory or writing adjacent user saves. No
incident video yet. Do not call intended blinking/filter
effects/camera interference a reproduced renderer fault without evidence.
Host renderer rules do not establish actual DS VRAM/DMA/IRQ timing. No IRQ
allocation/I/O, unbounded event queues, blanket IRQ removal or hidden frame loss.
Printer checks require production parser/output plus serial connection; no
parser-only completion claim. Preserve existing output on write/flush/rename
failure and bound RLE/packet sizes. No real user printouts in fixtures/assets.

## Progress

- [x] Read specification and exec-plan skill.
- [x] Actual integration agent prepared verified BASE, worktrees and permissions.
- [x] Specialist partial fixes/diagnostics and independent evidence review.
- [x] Shared integration and frozen candidate software tests.
- [ ] Final review, exact-C/tag build and package gates.
- [ ] Published prerelease and public-download verification.

## Outcome

Software candidate 53692f5 includes the printer decoded-length correction and
passes exact-SHA CI and artifact verification; final read-only review pending. No new
release/tag published. Previous freeze USER_REPORTED resolved with exact tested
SHA unknown. Flicker has diagnostics but no validated fix. Publication remains
blocked, with PARTIAL_FIX_DIAGNOSTICS_READY the provisional outcome.

## Decisions and discoveries

- Read-only supplied ROM inventory: Castlevania 1,048,576 bytes,
  SHA256 `3ed5f6aae59a7ff770c687eee593edb2d5faae172775d76d7786a4820d9a6a2a`,
  CGB flag 0x20, SGB flag 0x03, old licensee 0x33, cart 0x13. Tales 4,194,304
  bytes, SHA256 `c004b5ade81fd35e441dc74b133bf2dc57bd2bad7079c8f041ea217d6c9f063e`,
  CGB 0x80, SGB 0x03, old licensee 0x33, cart 0x1b. Distinguish Castlevania's
  DMG/SGB path from Tales' dual-mode GBC/SGB path. Originals untouched.
- Printer specialist identified source-level pre-checksum DATA/PRINT mutation,
  unchecked in-place BMP header/append writes, and missing serial SC bit7 clear.
  Main approved owned packet-validation/staged-output work after failing tests;
  SC completion shared change deferred to integration. No real-game printer
  reproduction claimed. Require bounded memory and preservation on failures.
- Video found separate probe SRAM/RTC mutation risk despite skipped persistence;
  needs production-linked proof and bounded shared-owner isolation. Default
  Prefer-GBC policy still selects steady SGB for Castlevania, unlike dual-mode
  Tales. Do not conflate them or disable required SGB host work without evidence.
- Main extended video ownership to ARM7 main.c scaling-transfer path ONLY,
  paired with ARM9 gbgfx. Candidate cross-core VBlank ordering problem: ARM7
  copies VRAM without testing ARM9 ready handoff. Require reorder/ownership tests
  and explicit real-DMA/hardware limits; unrelated sound/shared headers remain
  integration-owned. Source concern alone is not a flicker reproduction.
- Video rejected the initial READY/BUSY/DONE cross-core transfer design before
  committing: keeping bank C ARM7-owned across active scanout causes another
  display failure. Investigating a bounded single-core alternative; no gate pass.
- Printer completed local commit `d94df58b3072497a7568f46739c333cd61583b74`
  (gbprinter.cpp and dedicated test only). Synthetic Windows parser/output test
  PASS; sanitizer/toolchain and real FF01/FF02 integration pending. Reported
  limitations include separate BMP per print, one copy, margins/exposure not
  rendered. Independent protocol/output review requested before integration.
- After printer finished, actual evidence specialist spawned with Luna/medium,
  initially read-only. Video remains active; maximum two active children kept.
- Video production idle-host test reproduces CPUs executing zero-filled memory
  after reset without uploaded code. Owned fix gates execution until program
  start; require wake/resume regressions, measured work counts and clear lack
  of DS wall-clock performance evidence. Probe SRAM isolation still shared.
- Evidence checkpoint completed read-only: printer CI wiring/production serial
  integration absent; RLE 0xff manual attribution needs verification, margins/
  exposure/copies subset must be explicit. Real FAT filename behavior untested.
- Video commits: 6ded813 idle host/test; 31e0e01 bounded VBlank queue/test;
  7d42437 intentionally failing probe save isolation test (do not label green).
  No paired DMA patch. Queue removes direct vector allocation only; dispatched
  callbacks still may reach border file I/O/console allocation in IRQ.
- Flicker publication hypothesis remains unproven: guest-frame buffer swap may
  occur mid physical scanout; delayed swap needs safe producer ownership.
  Main authorized only debug-only fixed event ring/foreground drain plus host
  trace tests, no speculative buffering or IRQ-wide callback rewrite. If no
  validated flicker fix is obtained, publication gate fails and preserve partial
  fixes/diagnostic build rather than publish a beta claiming all three addressed.
- User clarified flicker: entire screen briefly black with scaling enabled.
  Video notified to prioritize scaling VRAM/display/capture ownership evidence.
  No off-scaling comparison/video trace yet; do not equate the report with
  proof of a particular DMA race or hardware validation of a fix.
- Video completed diagnostic commit 650db73 and shared-code proposal; both
  specialists finished. Integration resumed for sequential cherry-picks,
  shared probe isolation/fast-forward and production printer serial tests.
  No validated flicker fix is currently present; diagnostic path is fallback.
- User clarified printer incident: Tales main-menu Printer submenu -> Print
  makes no progress. BMP presence not specified. Prioritize serial completion
  and printer response/status waits; synthetic reproduction is not actual game
  menu validation. Integration notified; original ROM/save remains untouched.
- Integration draft probe snapshot passes local production-linked 0/8/128KiB
  success/timeout/interrupted-unload cases (baseline SRAM 5a->a5 exit4, draft
  exit0). Save/state/import/export guarded while probe active. Allocation
  failure fallback test and actual DS memory headroom remain pending.
- Production FF01/FF02 -> runEmul -> printer INIT/STATUS local test PASS with
  SC clear and no duplicate IRQ; full DATA/PRINT/BMP serial path still requested.
- Independent Nintendo Game Boy Programming Manual chapter 9 printed p244
  defines RLE 0xff as repeating the following byte 0x81 times. Integration
  corrected the printer specialist's mistaken rejection and its test expected
  value. Preserve this correction; do not present the earlier subset as correct.
  Reference: https://thissideout.wordpress.com/wp-content/uploads/2014/02/gameboyprogrammingmanual.pdf
- Integration local scoped Clang tests PASS: probe RAM sizes 0/8/128KiB,
  success/450-VBlank timeout/unload and forced allocation failure; probe
  hold/toggle/menu controls; GB/CGB FF01/FF02 -> production runEmul/parser ->
  DATA/PRINT -> BMP header/pixels plus SC completion/one IRQ; corrected RLE;
  idle host/queue/trace; package/structure/diff checks. Evidence retained at
  `.codex-tmp/integration-tests/evidence.md` in integration worktree.
- Main authorized explicit code/test/build commits and Draft PR (dependent on
  PR #6), then frozen-code Linux sanitizers and normal/trace DS/DSi builds.
  No tag/release: flicker fix gate still unmet. Luna resumed four owned docs
  for final evidence alignment. Diagnostic must provide usable foreground dump,
  distinct filenames and no normal-build trace overhead.
- User confirmed GameYob printer option was ON during Tales' Print/no-progress
  incident. Do not dismiss it as printer disabled; candidate hardware behavior
  still untested. Integration and evidence agents notified.
- Integrated code 6e6cfda passed host sanitizers/resources/package and normal/
  diagnostic DS builds (preflight 36179418191; trace 36179421822), Draft PR #7
  created and attached. HOWEVER superseded before review: integration found
  SC bit7 clear also affected pending local-link branch. Main approved narrowing
  to printer/disconnected and adding explicit peer-pending regression, then
  new exact-SHA verification. Do not call 6e6cfda the accepted final candidate.
  Preserve prior successful logs and this discovered test-coverage gap.
- Corrected code `d73d7067285842f132771eed52dffef493bfa0b0`: new peer-pending
  serial regression PASS; core 36179850524, normal DS 36179850446, preflight
  36179855988 and VIDEO_TRACE build 36179859506 PASS. Preflight 27 tests each
  normal/UBSan/ASan+LSan, legacy 1,000-cycle lifecycle, resources/package and
  two identical clean pinned BlocksDS builds. Normal/trace NDS structural
  checks PASS; separately named diagnostic ZIP extracted/hash-verified locally.
- Dependency audit reports no new runtime dependencies/assets and existing
  LICENSE/OFL/STB/third-party notices preserved. Inherited provenance concern
  remains in repository-preflight-20260903.md (3in1 aladdin/EZFlash code without
  separately resolved grant). Do not claim repository redistribution cleared;
  preserve as an additional publication-gate caveat, not a legal conclusion.
- Before documentation freeze, Luna found encoded and decoded printer packet
  bounds incorrectly both capped at 640. Integration agreed this rejects valid
  compressed expansion fitting remaining 8,000-byte image storage. Main approved
  two-pass complete validation then direct decode to image tail, without large
  stack/double buffering; require >640 valid expansion, total capacity boundary,
  overflow/truncation and prior-image preservation tests. New code/CI identity
  required; d73 test passes do not certify the newly exposed missing case.
- Same new 12-byte encoded -> 774-byte output assertion fails against isolated
  d73 code (exit -1073740791) and passes the corrected implementation. New
  53692f5 preflight 36180797959, trace 36180800999, core 36180780686/36180775281
  and DS 36180780454/36180775148 all PASS. Final artifact hashes/notice-bearing
  diagnostic archive and docs are being aligned before read-only review.
- Final 53692f5 normal and trace NDS structural checks PASS. Notice-bearing
  diagnostic ZIP `diagnostics-53692f5-with-notices.zip` extracted/hash-verified:
  SHA256 `8635778e0af057dba8787dea9986411c4bc101a3c6877c42c52867ba8d6a0dd8`.
  Earlier candidate/build notes above are preserved historical checkpoints,
  not current acceptance. No valid fix for full-black scaling incident yet;
  no publication authorized while that and provenance gate remain unresolved.
