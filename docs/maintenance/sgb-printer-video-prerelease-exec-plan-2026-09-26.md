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
- [x] Full final review and targeted correction review; exact-source software and DS builds.
- [ ] Public tag/build/package gates (blocked; no tag created).
- [ ] Published prerelease and public-download verification.

## Outcome

Current controlled USER_REPORTED split: ordinary exact-a084 build shows no white
line at normal speed, but intermittent lines with L fast-forward; diagnostic
exact-a084 can show lines at normal speed. Earlier broad normal-speed regression
interpretation is superseded. Focus implementation on safe host display
publication during fast-forward and independent debug instrumentation overhead,
without slowing guest execution merely to conceal artifacts. No fix yet accepted.

Follow-up in progress: user reports no recurrence of the original whole-screen
black flicker in the diagnostic build and generally working SGB/fast-forward.
A distinct Castlevania Legends white-horizontal-line defect reportedly persists
across ROM changes with Prefer GBC and borders. This is the active investigation;
diagnostic timing versus normal-build behavior remains a validation boundary.
Printer patch incompatibility is a user hypothesis, not an established cause.

Software candidate `a0846aae39fc9999ddda34269de317429dee85d1` corrects the
review's probe-transition save regression and menu trace-retention gap. Exact
source tests/builds passed and the same reviewer accepted the targeted delta.
Earlier 53692f5 artifacts remain superseded and must not be handed off.
Final status: PARTIAL_FIX_DIAGNOSTICS_READY, not a published prerelease.
Previous freeze is USER_REPORTED resolved with exact tested SHA unknown.
Flicker has diagnostics but no validated cause-linked fix. Publication remains
blocked by that missing fix and unresolved inherited provenance. Local diagnostic
packaging and evidence documentation do not imply redistribution clearance.

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
- Integration committed five final docs at e3efa655, then both workers ended.
  Actual `/root/final_reviewer` spawned with Astra/xhigh (resolved metadata
  UNVERIFIED). Read-only review: no P0; P1 mode change/reset while probing can
  clear probingForBorder without ending snapshot, blocking saves and discarding
  real-SGB progress on unload. Existing tests lacked file-backed mode transition.
- P2: trace continues overwriting 256-event history during menu navigation;
  freeze/preserve on entry or immediate trigger required for usable capture.
  Reviewer finished before integration resumed. Main authorized bounded P1/P2
  fixes plus pre-fix reproducer, new exact-source tests/builds and targeted
  reviewer follow-up. No 536 build/private handoff or prerelease in the meantime.
- Correction `a0846aae39fc9999ddda34269de317429dee85d1`: production-linked
  file-backed probe transition reproducer exit 44 before fix, exit 0 afterward;
  Prefer SGB/GBC Off, repeated reset, timeout/cancellation and SRAM/RTC reload
  covered. Debug-only ring freezes before menu display changes and resumes on
  exit. Retention/resume test passes; pre-fix lacked the API (compile failure,
  not a reproduced hardware flicker).
- Exact a084 CI PASS: core 36182509783, ordinary DS 36182509785, full preflight
  36182535946 (normal/UBSan/ASan+LSan and two clean pinned builds), separate
  diagnostic 36182538849. Normal and diagnostic NDS structure checks passed.
- Same `/root/final_reviewer` targeted delta review completed with implementation
  stopped: P1/P2 resolved, no new blocking finding; engineering allows clearly
  labeled private exact-a084 diagnostic handoff, not publication/legal approval.
  Only roughly 0.2 seconds before menu entry is retained; delayed capture is
  inconclusive. No actual hardware trace acquired.
- Latest user reissued the same three-fix/publication gates. Main retained the
  conditional publication stop, resumed Luna for final four-document alignment
  and integration for local notice-bearing diagnostic packaging, without further
  source changes. Next actionable evidence is a prompt trace capture of the
  full-black scaling incident, plus resolution of the inherited provenance gate.
- User supplied current INI, read only. Integration verified enum meanings:
  Printer On, Aspect scaling + filter On, Top screen, VBlank wait Off,
  HBlank/Window On, GBC mode/BIOS On; SGB mode/borders and custom border Off.
  This corroborates the printer/scaling configuration but is not a snapshot
  captured at the earlier incident. Do not retroactively replace the reported
  SGB-On slowdown conditions. Original configuration and storage left untouched.
- User clarified the current border-Off values are a workaround chosen because
  of the slowdown. Preserve SGB-On slowdown/fast-forward failure as USER_REPORTED
  incident conditions, not disproved by the newer INI. The custom-border value
  at the original incident remains unspecified.
- Follow-up user report: diagnostic build has no original whole-screen black
  flicker, reiterated explicitly; SGB/fast-forward work generally. Castlevania
  Legends with Prefer GBC and borders produces white horizontal noise which
  reportedly carries into the next ROM. User's printer/ROM-patch explanation
  remains unverified. New white-line defect is separate from the black flicker.
- Actual follow-up agents `/root/video_sgb_followup` (requested Sol/high) and
  `/root/integration_followup` (requested Sol/medium), fork none, at most two
  active children, no redelegation; resolved model/effort still unexposed.
  Source tip 3bcc1146 initially clean; preserve earlier tested a084 and all
  stabilization. Video owns isolated renderer/SGB edits, integration owns shared
  runtime and all remote mutations. Reassess exact release gates after focused
  regression evidence/review, not by equating a diagnostic non-recurrence report
  with proof of normal-build timing correctness.
- User supplied a still and two local videos. Video specialist inspected frames
  read-only in isolated scratch output: ~3.17s gameplay and ~12.73s boot. Bright
  region bands are visible but cannot be separated conclusively from filmed LCD
  interference; no ROM-switch sequence was captured. Preserve the user's real
  white-line observation without claiming it is only camera interference.
- Rejected investigation: specialist initially claimed refreshGFX dropped queue
  lengths without clearing dirty flags and produced helper test red 2/green 0.
  Full call-chain recheck showed updateTileMap clears each flag during refresh,
  so the test modeled an impossible production boundary. Commit 3050ee6 was
  reverted locally by 1107c2f and NEVER integrated. Main corrected the premature
  commentary; integration stopped before cherry-pick. No new renderer fix or
  candidate validation is established by that test.
- New user photo clearly shows a narrow bright horizontal line immediately below
  KONAMI inside the guest rectangle, not spanning the decorative border. User
  reports the line persists regardless of scaling/filter and with SGB borders
  disabled. The photo itself shows a border, so record the Off tests separately
  as USER_REPORTED, not as visible photo settings. Cold launch with all Off is
  not yet distinguished from changing settings in-session; optional question sent.
  Resumed the same two agents for focused reachable scanline/shared-frame-path
  analysis; no repeated provenance search, no speculative renderer edits.
- User subsequently confirmed cold restart, Wait for VBlank On and SGB Mode
  Off do not remove the line. Supplied CSV contains 256 valid events across host
  frames 13888..13900, all 12 publish events at physical line 192, no mixed
  published generation among sampled host scanlines, scale=0. This only weakens
  the active-swap hypothesis in the captured ~0.2s window; no incident marker or
  BG/window/palette states are available. No source fix inferred.
- User explicitly requested PC-emulator reproduction and logs before fix and
  prerelease. Main read computer-use skill and required API/guidance/confirmation
  references; main alone owns native UI via sky. Integration prepares isolated
  melonDS 1.1 config/BIOS copies/DLDI storage and a hash-verified copied ROM, never
  user saves or the existing GameYobTest image. Video specialist completed a
  bounded checkpoint, no instrumentation until direct emulator evidence or a
  specific missing-debugger need; SDL renderer is not a DS output oracle.
- Latest user clarifies menu entry hides the white line; Castlevania triggers
  persistence into another ROM, and initialization restores normal rendering.
  A prior stable build reportedly did not exhibit it. Exact known-good binary and
  meaning of initialization (menu Reset vs app restart) are requested, not assumed.
  Earlier all-Off/cold-launch reports concern triggering Castlevania; distinguish
  that from recovery in a subsequently loaded ROM. GB/GBC transition is a user
  hypothesis, not a confirmed cause. Same two agents resumed comparing actual
  load/reset/menu lifecycle call chains and BASE-to-candidate differences. Do not
  repeat menu-triggered capture as if it leaves the symptom unchanged.
- User identifies v0.5.10 as free of the white-line problem. Integration will
  resolve the exact release/tag/artifact identity; expand comparison to that
  release through a084 rather than assuming stabilization BASE763 was good for
  this newly reported defect. User has not yet defined Reset vs app restart.
  Automated bisect remains conditional on a genuine reproducible classifier;
  melonDS user's non-recurrence is not a failing predicate.
- Resolved GOOD v0.5.10 source commit: 4d0f29f7e07c26dbaea1006aec310fea7b50828e
  (annotated tag object 30ec90035d498bff07de939a3e3d52087d8f0c34 is not the
  commit). Tracked ZIP hash 18c8eff2f7c7b5d38f302e996c68dad5a63935f70782dd13d45343847e4d846c
  matches GitHub asset digest; released normal NDS hash
  12dcb43eb301bfd14ac0fa56bd91599532de72fcf9ea01b9dc7ba7ce79ae68b4.
  Source comparison found no changed BG/window/palette composition algorithm;
  release-visible graphics delta centers on VBlank queue, with shared startup/
  probe/BIOS changes and debug-only HBlank trace overhead. No culprit proven.
- An important remaining split is exact-a084 ordinary vs VIDEO_TRACE build on
  the same device/ROM/config, because debug IRQ sampling itself may affect timing.
  Both existing artifacts were already built/tested; do not call this a fix or
  rebuild unnecessarily. Reset and ROM loading both refresh guest/map content
  but differ in unload/probe/border/display scheduling. No speculative global
  clears, source rollback or release authorized by these observations alone.
- User completed the exact-source A/B: ordinary a084 normal speed is clean,
  L fast-forward intermittently causes white lines; diagnostic a084 can show
  lines at normal speed. Requested correction and distribution. Same video and
  integration agents resumed with implementation scope, first requiring actual
  production-path evidence and safe frame ownership design; preserve guest timing
  and baseline stabilization, no blanket frame-wait throttling or speculative
  mode reset. No new source/release accepted from the hypothesis alone.
- Video design review found pointer-only publication gating insufficient: live
  tile/map/palette uploads and deferred dirty queues must also retain ownership
  and fit host blanking time. Rather than implement an unmeasured timing patch,
  authorized a low-overhead automated diagnostic to measure actual publication
  and upload intervals, without HBlank sampling or menu-triggered capture.
- Video instrumentation-only commit 04d21599130a659e8867e8311ce431449c163082
  adds UPLOAD_END event and FF/mode/queue metadata; dedicated host ring test
  passes. Integration adds opt-in profile build, 180 ready-frame warmup then
  separately captured/exported 20 normal, 20 fast-forward, 20 normal guest frames.
  Restore previous FF mode before each foreground export including failures;
  retain 256-event ring, report overwrite deltas. No guest timing or ordinary
  renderer fix is claimed; require reviewed exact build before PC profiling.
- Targeted read-only profile review at 2a1b83e found no P0/P1 for isolated,
  hands-off execution. Inputs/reset/menu/lid can invalidate labeled windows;
  inspect actual FF fields and monotonic counters. Foreground CSV export leaves
  a gap, so the final window is not evidence of immediate FF-release behavior.
- Frozen profile source 71917a5f2f603f4f0e637eb54c3576758ad1d9d2 adds only
  explicit profile CI dispatch wiring after review; pinned build run 36232317142
  dispatched. No white-line correction or release accepted yet. Main UI scan
  found no running melonDS; reuse only isolated scratch setup for next capture.
- Exact 71917 profile DS/DSi CI and header checks passed; main booted profile in
  isolated melonDS without intentional guest input, then closed it. Three unique
  CSVs were exported under scratch sdroot/gb with monotonic counters, 20 guest
  frames each, FF 0/1/0 and no overwritten events. Integration found startup
  gfxMask=1 suppressing every publish/upload in the first two windows; therefore
  these logs validate the harness but NOT the scanout hypothesis. Authorized a
  diagnostic-only stable-unmasked readiness gate and bounded retry on masking.
  No thin-line reproduction or production correction established by this run.
- User clarified ordinary NDS behavior: while L is held, white lines appear and
  disappear intermittently; releasing L removes them. Treat this as transient
  FF-only behavior, distinct from prior diagnostic-build persistence reports.
  Investigate scanout/update timing without assuming persistent GB/GBC state.
- Resumed after user stopped UI with Escape. Exact dbe8f203 profile (SHA256 NDS
  c585a5b7bdb65aa4b6269287d0d7de122d9cbd96535a02cfd2235ac1c5abe782)
  passed targeted review/build, then ran in isolated melonDS. User closed the
  emulator after UI input conflict and reconfirmed no visible white line on PC.
  All three current CSVs have 20 complete/publish/upload events, mask=0,
  FF=0/1/0, monotonic counters, zero overwrite. FF has 11/20 visible-line
  publications and 10/20 visible upload ends; normal-before has 192->203 for all
  pairs. Queue counts are zero, so this is not a worst-case transfer budget.
  Timing overlap is established in PC logs, physical symptom causality is not.
- User now requests the build for hardware testing. Integration packages exact
  dbe8f203 as a private automatic diagnostic, explicitly not a renderer fix or
  public prerelease. Current CSVs must be copied outside folder-sync before any
  rerun: earlier 71917 CSVs are no longer visible after melonDS synchronization,
  so no-clobber inside the app does not establish preservation across sync.
- New hardware manual logs on Desktop (18:55/18:56) replace earlier supplied
  filenames: trace_00 SHA256 a5187824ba5d307e347dac914f6ad30df6b7dea4c51def229eaf0cb871492277
  contains 256 events, FF=0 throughout; 64 publish events all at VCOUNT192,
  uploads end192..200. trace_01 has only headers. These do not capture the
  reported FF-only line, despite nonzero tile/map queues in normal-speed data.
- Integration identified a diagnostic routing flaw: automatic profile starts
  only when autoloadrom is nonempty, not after ordinary file-browser ROM choice.
  This explains a plausible missing-log path, not a proven user config value.
  Authorized a narrowly scoped, separately identified FF-release diagnostic:
  real held/toggle FF release triggers foreground capture of prehistory and
  eight normal frames, independent of autoload; guard mode/menu/probe/reset
  contamination, preserve files, test/review before hardware handoff. This is
  not a renderer correction and does not satisfy prerelease publication gates.
- FF-release diagnostic frozen at adeab302319348fdd748b873ef1bed1b259400c8.
  Targeted reviewer found no P0/P1 for private hardware capture; exact pinned
  DS/DSi workflow36235047396 and core/normal builds passed. Structural/resource
  checks passed. DS artifact SHA256
  92d41a873303b48a1e42b5889d49fdc423d6ccbe7c1c0cc34f294a4d9b5b62ee.
  Manual ROM selection supported; effective FF release preserves eight guest
  frames then foreground export, no menu/forced FF. Raw CSV validation is not
  proof of paired complete transition; overwritten is lifetime ring total.
  Hardware execution remains pending. Handoff uses plain NDS and README under
  .codex-tmp/integration-tests/diagnostic-ff-release-adeab30. No new public tag
  or release; white-line cause and renderer correction remain unverified.
- 2026-09-27 resumed implementation after incident-associated hardware capture.
  FF-release trace SHA256866f43c51305d3579cfa687623422569d24ecb5d81683bafc44df33d23b27b8c
  contains 66 FF publishes (35 visible), 67 FF upload ends (36 visible), eight
  normal publishes192 and upload ends198. Leading unmatched upload is the ring
  head, not evidence of a missing in-window publish. User explicitly released L
  upon seeing the line. Last FF guest2151 commits114->120, release VCOUNT122.
  This strongly associates mid-scanout commits with the incident, without a
  pixel-event marker. Video specialist resumed concrete transactional-commit
  implementation design, preserving live VRAM and scanline ownership together;
  integration performs bounded repository checkpoint only until patch handoff.
- Architecture decision: a full VRAM-bank shadow conflicts with custom-border
  storage and the printer OBJ icon. Approved compact RAM staging instead:
  four 16KiB guest BG tile blocks, 16KiB guest OBJ tiles, six 2KiB maps (92KiB),
  plus a third scanline buffer for displayed/ready/producer ownership. This
  capacity estimate is not a measured hardware timing guarantee.
- Implementation phases: (1) production conversion target abstraction and
  byte-equality/dirty-coverage tests, (2) queued complete-frame ownership and
  coalescing tests, (3) bounded safe-window commit with reviewed DMA/channel
  ownership and frequent foreground service hook, (4) exact builds, targeted
  review and before/after hardware evidence. DMA activation remains gated until
  budget and ownership are justified; partial staging is not a released fix.
  Video owns graphics/helpers/tests, integration only shared scheduling hook.
- Phase1 conversion extraction integrated at 9c16a9d8e42ad33cf3793cf4eb4b45df085f853d
  (video commit c62f6cc plus test registration). Production still targets live
  VRAM, so no visible fix. Exhaustive host conversion tests and pinned DS builds
  passed; preflight36276373420 DS job passed two deterministic clean builds,
  host job failed in run_host_tests.py parsing an existing multi-command
  publication-profile CI step. Authorized narrow runner repair, no skipped tests.
- Video phase2a0575f436 adds tested ownership helper/reserved third scanline
  buffer, still not integrated or enabled as FF rendering. Complete staging
  and safe commit service remain in progress; do not release preparatory code
  as a white-line fix or repeat full preflights before a complete candidate.
- Runner repair c27ed6f passed auto preflight36276600118; targeted phase1 review
  found no P0/P1. Later measurement integration frozen at 1ddd192 includes
  guarded inactive stage code and pre-ROM16-trial copy/export. Normal builds
  retain two scanline buffers; EXPERIMENT-only extra RAM, ACTIVE not enabled.
- Targeted 1ddd192 review found no P0/P1 for inactive measurement only. Pinned
  diagnostic run36277251778, normal/core/preflight and structural checks passed.
  DS diagnostic SHA2567853ee9bbcb14db6685ae1c86a0acf99f538cd6e7c9fcf09ccb7f0acd96bb1ad;
  plain artifact under .codex-tmp/integration-tests/stage-copy-diagnostic-1ddd192.
  Normal ARM9 BSS216688; diagnostic ELF absent from artifact, so its exact BSS
  was not measured. No release/tag created.
- Hardware handoff: launch diagnostic without ROM/input, collect unique
  gameyob_stage_copy report from startup FAT folder. Trials copy identical
  94208-byte guest assets while graphics disabled and restore initGFX. This is
  a baseline copy measurement, NOT loaded-game worst-case timing or liveness
  proof. Host-frame/VCOUNT snapshots non-atomic; interpret boundaries cautiously.
  Active stage path still needs budget/margin, ownership/liveness validation,
  targeted review and incident-matched hardware before/after testing before use.
- Hardware stage-copy report D:/gameyob_stage_copy_00.csv SHA256
  ac28a6a14c88b1c85aca85ace597318aafe82366a0f94be26ee24e11a8a495d7:
  16/16 trials94208bytes, same host frame, VCOUNT192->240 (48 lines), no malformed
  records. ACTIVE=0 explains unchanged FF symptom. VBlank-start full copy misses
  line0-pre-render235; 48 observed lines is not a loaded worst-case guarantee.
- Resumed implementation: video develops earlier foreground commit plus dirty
  copy and complete-generation ownership; integration owns frequent guest
  scanline/timeslice service hook. Explicitly audit VBlank callbacks which could
  allocate or perform I/O during a copy spanning192; do not substitute a long
  IRQ mask, unbounded callback, or an after-the-fact deadline assertion for a
  safe transaction. Existing active path remains gated pending this work.
- Agreed frequent service hook: DS experimental ACTIVE only, after
  updateLCD(cycles) inside Gameboy::runEmul, with no guest-cycle change or normal
  build overhead. Renderer owns early168 readiness, empty-VBlank-task admission,
  short metadata critical section and callback-only deferral during transfer;
  fixed capture/line-completion IRQ work stays in place.
- Candidate goal revised to same-boot early168 calibration plus fail-closed
  runtime activation, avoiding another measurement-only hardware roundtrip.
  No arbitrary compile-time MAX_COPY_LINES/LIVENESS_VERIFIED declaration.
  Requires bounded trials, conservative margin, ownership/liveness tests and
  safe fallback design before enabling; measured success is still experimental
  evidence, not proof of every hardware load or public-release approval.
- Renderer5046f1e review caught P1 stale per-line coalescing and normal-wait
  starvation; corrected by f03331e (integrated fe55722): preserve current-frame
  event stream, service safe window during normal waits, fresh admission and
  pre/post-publication timing checks plus two-line publication reserve.
- Integrated4f6441f builds/preflight passed, but review caught P1 menu/pause
  watchdog false fault. Fixed at54399baaf879e7dbc38fc538f43aebf3779dc380 with
  suspended-interval age rebase, no duplicate paused staging, and unconditional
  pause on newly latched faults. Targeted reviewer accepted this delta with no
  new P0/P1 for private experimental hardware testing, subject to exact builds.
  Foreground hook is ACTIVE-only after updateLCD; error opens visible menu once.
  Reset/Reload recovers ordinary graphics; app restart required for staged retry.
- Candidate remains experimental: calibration measures early168 full transfer
  and includes16-line margin; fresh deadline admission and callback reservation
  constrain copies. Calibration is not proof of universal hardware timing; an
  observed overrun can only be detected after some live writes. No stable/public
  release or white-line-fix verification has been claimed.
- 2026-09-27 hardware calibration for54399baaf879: Desktop
  gameyob_stage_calibration_00.csv SHA256
  ffbf826165099577c87da5e5501eace10aa4ca423877a1b9501ff62e0c7e6db1.
  All16 trials copied94208bytes; thirteen took49scanlines, three48. Result
  ineligible:49+16margin+2publication reaches235 and fails strict deadline.
  ACTIVE compiled does not mean activated; the reported reduced frequency,
  working menu resume and clean post-ROM-switch display are baseline-path
  observations, not staged-renderer success. User still sees intermittent
  lines in Castlevania FF and during switching, only this title reported.
- Next scoped experiment: guarded cache-flushed ARM9 DMA3 through one shared
  calibration/runtime transfer function, retaining existing admission margins.
  Verify pinned BlocksDS API, cache alignment and competing ARM9 DMA users;
  ARM7 scaling DMA is a separate engine. Video owns graphics delta; integration
  owns shared files/builds. Targeted review and exact-source normal/experimental
  checks precede another private hardware build. No public release gate passed.
- DMA candidate frozen at019b81d7eb180018c7a25af70b6287202cce8740 (video8c45ebf
  integrated asa315c13 plus shared status reporting). Pinned BlocksDS1.22.2
  libnds5788d216 confirms cache flush/drain and ITCM synchronous dmaCopyWords;
  calibration/runtime share validated11-block transfer backend. Existing
  margins/deadline unchanged. Startup report labels dma3_words; paused-menu-only
  exclusive status export records calibrationEligible, stagedEntries and
  presentedFrames separately. No active gameplay file I/O or ordinary-path change.
- Targeted reviewer found no newP0/P1 in54399baa->019b81d7; current foreground
  DMA ownership and callback exclusion accepted for private hardware trial.
  Host DMA tests simulate copies and do not prove hardware cache/bus behavior.
  deferredForCallbacks also counts DMA-busy deferrals (nonblocking naming caveat).
- Exact019b81d CI passed: experimental36283795566; normalDS push36283791192,
  PR36283795329; core36283791137/36283795231; preflight36283795186.
  NDS structural/hash checks passed. Experimental DS SHA256
  85f3bcffc68889f94984032dc322d8c07bf27fbd0df94e162451f45f133b3463;
  DSi274708b81d90747dd47685c0d6f9df6398fd0076b40423192253b16eb50379e5.
  Both739840bytes. Hardware eligibility/activation/white-line outcome pending;
  private handoff authorized, no new public release/tag or master merge.
- Hardware019b81d reports supplied2026-09-27: calibration SHA256
  b763403f8e10d315c03310b45a3d7bbbe52000b2e5dd39af988a8504694eb568,
  status95c49bffab95caeb3e207c30d94725c3f26f482792bd353004c1c22ed17774e0.
  StrictUTF8 reads27/11lines, zero malformed records. All16 full94208byte DMA
  trials168->196 (28lines), eligible1, admitted bound44. Paused-menu status:
  staged_entries1, presented_frames442, deferred0, last_copy_end_vcount169,
  fault0. This establishes activation/publication, not all transfers taking1line
  or universally clean video. Previous CPU calibration49lines was ineligible.
- User explicitly reports white lines still occur similarly to before. Faster
  transfer and actual staged publication did NOT resolve the reported symptom.
  Do not publish as fixed or merely loosen timing gates. Video specialist now
  audits remaining live palette/OAM/scanline publication and FF scheduling
  bypasses to propose a discriminating next test before further implementation.
- Next diagnostic (not a rendering fix): sparse64-event HBlank anomaly ring,
  capturing previous-line retry, visible non-HBlank entry and line-cross event,
  frame generation/line state plus selected BG/window/palette/OAM registers.
  Video delta1c81cf3; ordinary nonflag handler unchanged. Bounded instrumentation
  still perturbs IRQ timing, so symptom disappearance or no events is inconclusive.
  Shared integration will arm on new L press, freeze on genuine gameplay L
  release, and export only from paused menu; no active FAT writes. Same-source
  ACTIVE control and ACTIVE+anomaly builds requested for matched comparison.
- Scoped history check found no production HBlank renderer change from v0.5.10
  to stable BASE; later pre-stage changes include fixed VBlank queue and
  diagnostics, not evidence by themselves of the cause. Exact user-known-good
  binary correspondence remains unverified; no deterministic bisect claimed.
- 2026-09-29 resume: frozen e201c3a4994fb5581c004aa7aebd99f587e824ca remains
  intact; only this main-owned plan is dirty. Prior integration/review agents
  stopped on usage limits before final handoff; partial review is NOT approval.
  Replaced unavailable sessions with /root/integration_resume (requested
  gpt-6-sol/medium) and /root/hblank_delta_review (gpt-6-astra/xhigh), max2
  workers/no delegation. Resolved runtime settings remain unverified.
  Retrieve already dispatched exact-source CI using explicit GimoXagros/GameYob
  repository (gh default upstream Stewmath produces misleading404). Finish
  targeted diagnostic review and artifact checks before private handoff; no
  public release, proven visual fix or new source changes assumed.
- Resume verification: all7 exacte201c3a runs succeeded (ACTIVE36286087882,
  anomaly36286089241, ordinaryDS36286088498/36286091468,
  core36286088550/36286091515, preflight36286091440). Integration downloaded
  artifacts under.codex-tmp/private-hblank-e201c3a, verified manifests and
  structural checks for all3 modes. HBlank diagnostic strings absent in
  normal/control and present in anomaly; not full machine-code absence proof.
- Targeted review completed2026-09-29: no newP0/P1, private diagnostic handoff
  allowed. Caveats: releaseHost/VCOUNT pair non-atomic; exit_vcount measures
  post-doHBlank not full ISR;64event capacity does not bound event frequency;
  effectiveFF edge includes toggle, so user procedure must use Lhold/release
  only. Paused-menu-only output and immutable snapshot ownership accepted.
  No renderer-fix or public-release approval. Hardware HBlank evidence pending.
- New e201c3a hardware logs: user reports lower frequency but intermittent
  white lines persist. StrictUTF8 HBlank/calibration/status reads73/27/11lines,
  zero malformed records. SHA256 respectively:
  f60b6b13e0fea4653da0a7481719dd6997198187cc6d64e71a444eaae627455d;
  bba461b684f9efcdbd3033adbcc074396f9f8ed8ab340fd8e173564ebe16221c;
  4a4e67232551acaf6ead35b31fa8e52b4da5989ebefcc88bf4d0a7ad8bdc42c8.
  Calibration16x28lines eligible/bound44; status staged_entries1,
  presented_frames1744, fault0, deferred0. HBlank ring retains64 events,
  overwritten2344; allflags4 (+1VCOUNT across doHBlank), allFF1. Sixty events
  at guest128/physical151->152; four at guest24/28/43/119. Retained host
  range4236..4295, releasehost4296. No pixel marker; cross-line events do not
  establish visible fault count or first-ever divergence. Specialist checking
  actual HBlank phase semantics and source work at that boundary before repair.
- User banner request supersedes renderer implementation for this turn:
  exact lines GameYob Custom / A Gameboy Emulator for DS / GimoXagros.
  Supplied32pxRGBA saved unchanged as platform/ds/icon.png; high-res root logo
  and upstream license attribution retained. Binary alpha threshold128 and
  <=15visible colors are necessary DS format adaptation, not artwork redraw.
- Actual pinned ndstoolv1.22.2-blocks4d8ef3e loadsBMP opaque: candidates55da3c0
  andde6691a rejected after finalNDS transparency failure.2250d26 switched
  input to generatedRGBAicon_banner.png; actual430transparent/594visible mask
  passed, but obsolete invisibleRGB assertion failed. Finalab9d1b63d92025c3720675ff53289c28612891b5
  compares RGB only for visiblepixels and transparency for everypixel.
- Exactfinal normalDS36589023300 andpreflight36588978713 passed; downloaded
  DS/DSi checked all6 bannerlanguages exacttext,430index0transparentpixels,
  594visibleRGB555 pixels, ARM/header/banner/DLDI/TWL. Private normalfiles in
  .codex-tmp/private-banner-ab9d1b6; DSsha256
  fab1bea05bbffe23f876a785b189a039074ae0c6475bad0773e63fc45b86514d,
  DSisha25679c3000698ba5555aecd82eaffae6393627fea6d07c392f6d73b18a39e493b48.
  No runtimechanges/publicrelease/tag/mastermerge; white-lineissue unresolved.
- NEW USER AUTHORIZATION2026-09-30: integrate compatible openPRs, merge to
  master and publish stableGameYobv0.5.11; user reports hardware stabilization
  and accepts Castlevania FF white-line limitation as separate openissue.
  This supersedes previous no-master/prerelease-only/white-line release gates,
  not license or failing-test gates. Current openPR6+7 bothmergeable/green;
  stableLatestv0.5.10, v0.5.11notlisted. Normal/nonexperimentalbuild intended.
  Integration sole remoteowner handles dependencyorderedmerges/version/docs/
  knownissue/reprocollection/package/release. Reviewer independently resolves
  prior3in1provenanceaudit disposition; do notsilentlyremovefeatures or assert
  legalitywithoutbasis. Preserve this dirtyplan until explicit selectivecommit.
- Release prep: issue#8 records Castlevania FF limitation and structured
  follow-up evidence. PR7 containsPR6tip763f6bf; dependencyorder6then7.
  Provenance reviewer found no new integrationblocker, but existing3in1
  permissionbasis remains unknown despite retained attribution/notices.
  Not an established legal prohibition; no licenseclearance claim possible.
  User asked to select retain+disclose / approve exclusion / holdpublication.
  Integration may prepare/mergeauthorizedPRs, but awaits answer beforepublic
  tag/release. Main authorizes selective inclusion of this plan in finaldocs.
- USER DISPOSITION: explicitly retain3in1feature and disclose unknown
  permissionbasis while publishingv0.5.11. This resolves the maintainer choice,
  NOT underlying provenance verification or legalclearance. Preserve notices,
  clearly list unresolveditem in release materials and continue stableworkflow.
