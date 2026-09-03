# Repository preflight — 2026-09-03

## Scope and outcome

Maintenance only, after v0.5.9-ko: no new feature, version, tag or release.
The code checks below pass, but this is **not** a guarantee that every emulator
path is correct. Hardware, legacy-state fixtures and inherited EZFlash source
provenance remain open. The development baseline is suitable for further work
with these known limitations; redistribution clearance is not certified.

This record captures evidence before the final documentation commit. The final
commit must pass the same preflight workflow before a fast-forward to master.
Its exact SHA, final CI run and branch cleanup are recorded in the task handoff;
the report does not embed its own impossible self-referential commit hash.

## 1–3. Repository and initial state

- Repository: `GimoXagros/GameYob`, fork of `Stewmath/GameYob`.
- Inspected checkout: `GameYob-Publish`; its parent directory is not a Git repo.
  Neighboring repositories were not modified.
- Origin: `https://github.com/GimoXagros/GameYob.git`.
- Upstream: `https://github.com/Stewmath/GameYob.git` (already configured).
- GitHub default and initial checkout: `master`.
- Initial HEAD: `04f8c83547eeb4c9633813624eb82ff914644f58`.
- No staged, unstaged or untracked changes. Existing ignored artifacts retained.
- No submodules or tracked LFS objects. Fetch/prune performed without rewriting history.
- Origin open PRs: none. Master unprotected; effective branch rules empty.
  An active repository ruleset exists but does not apply to this branch; it was
  not edited. Review/branch rules must be rechecked before integration.
- Maintenance branch: `chore/repository-preflight-20260903`.

Initial tags (all retained): `v0.5.9-ko`, `v0.5.8-ko`, `v0.5.7-ko`, `v0.5.5-ko`,
`v0.5.3-ko`, `v0.5.2-ko.1`, `v0.5.2`, `v0.5.1`, `v0.5`, `v0.4.1`, `v0.4`,
`v0.3`, `v0.2`, `v0.1.2`.
Origin releases: `v0.5.9-ko` (2026-09-01), `v0.5.8-ko` / `v0.5.7-ko`
(2026-08-29), `v0.5.5-ko` (2026-08-09), `v0.5.3-ko` / `v0.5.2-ko.1`
(2026-07-29). Latest published tag is not this maintenance baseline.

## 4–5. Commands and baseline evidence

Exact clean build, tests, generator inputs and packaging commands are in
[BUILDING.md](../../BUILDING.md). Active SDK: digest-pinned BlocksDS 1.22.2
container; observed Wonderful GCC `16.1.1 20260516`, ndstool `1.22.2`.

The Windows host has Python but no native g++/make/Docker/WSL toolchain.
C++ tests and ARM builds ran on GitHub Actions, not locally. Resource and
package checks also ran locally. Native 3DSX is deferred/manual-only; SDL and
old devkitARM DS recipes are legacy, not active release build gates.

| Baseline check | Result / evidence |
| --- | --- |
| Original 16 host suites | Normal and UBSan pass in [run 33746919948](https://github.com/GimoXagros/GameYob/actions/runs/33746919948) |
| First ARM7/ARM9 + NDS build | Built; supplemental ndstool audit failed due to an incorrect hardcoded SDK path, not a source build failure |
| Corrected baseline harness | [run 33747171151](https://github.com/GimoXagros/GameYob/actions/runs/33747171151), two clean DS builds and binary comparison pass |
| New SGB probes before fixes | JMP, BRA and negative BRR each fail independently in that run |
| Packaged guide local links before fix | Synthetic package regression fails on missing archived-3DS guide target |
| Assets | Regeneration matches; no stale generated asset found |

Baseline ARM comparison uses `a7fd75d567fd52fbb094d7e1db3110f5087a88d0`:
only harness/tests had changed since the initial emulator source. Embedded
Git metadata differs, so this is not a claim to reproduce a historical release
hash or the initial HEAD byte-for-byte.

## 6–9. Findings, changes and regressions

| Finding | Risk | Minimal correction / test |
| --- | --- | --- |
| Existing 65C816 JMP overwrote PC before fetching the high operand byte | High: incorrect host execution | Fetch full target first; isolated JMP regression |
| Existing SPC700 BRA read PC and mutated it during one expression | High: incorrect branch target | Sequence signed operand fetch; isolated BRA regression |
| BRR decoder left-shifted negative values | High: C++ undefined behavior | Bounded multiplication and explicit negative rounding; UBSan BRR regression |
| Empty/negative-version/truncated fixed state data or invalid WRAM/VRAM banks could reach state application | High: unsafe state load / bank indexing | Version, overflow-safe length, common-prefix and bank guards; boundary regression |
| Packaged EN/JA/KO guide relative links pointed outside their ZIP layout | Low: broken documentation | Remap language links; archived 3DS links point to source repository; ZIP regression |
| Guides described forced GBC and NDS/DSi filenames too broadly | Low: misleading usage | Document header-dependent GBC choices and actual DS/TWL header distinction |

State version stays **8**. Earlier four-bank RAM layout and short legacy tails
are preserved. Guards do not validate every mapper/SGB variable-length tail or
provide transactional rollback for all failed reads. The new state test covers
the pure guards, not end-to-end historical `.ys*` fixtures. Such fixtures and
malformed-state fuzzing were **not run** and remain explicitly pending.

No new SGB opcode, DSP/timing model, mapper, UI, platform or launcher workaround
was introduced. This does not complete the SGB host. No speculative upstream
hardware timing changes were imported. Recent source paths and TODO comments
were reviewed; absence of findings is not exhaustive proof against memory bugs.

Changed file inventory (plus this report):

- Runtime: `platform/common/sgb_host.cpp`, `platform/common/gameboy.cpp`,
  `platform/common/include/state_validation.h`.
- Tests: `tests/sgb_host_baseline_test.cpp`, `tests/state_validation_test.cpp`,
  `tests/package_release_test.py`.
- Checks: `tools/run_host_tests.py`, `tools/check_repository.py`,
  `.github/workflows/core-regression-tests.yml`,
  `.github/workflows/repository-preflight.yml`.
- Packaging/build commentary: `tools/package_release.py`, `Makefile.blocksds`.
- Docs: `BUILDING.md`, `README.md`, `CHANGELOG`, `THIRD_PARTY_NOTICES.txt`,
  `docs/features/emulation-compatibility.md`, all three `docs/guides/user-guide.*.md`.

## 10. Generated resources and package integrity

| Resource | Verification |
| --- | --- |
| 12 language files + built-in table | Regenerated; identical text after newline normalization |
| Unicode font | Exact regeneration with pinned Galmuri; GYUF v1, 20,615 unique sorted records, 206,162 bytes |
| CP949 table | Exact regeneration; 64,512 bytes |
| NDS icon/banner | Decoded RGB555 pixels equal committed 32×32 BMP; banner/header CRC pass |
| Embedded font/CP949 | Exact byte sequences found in ARM9 ELF |
| Package | Two synthetic ZIPs identical; all internal checksums and three guides' local links checked; notices present; no 3DSX |
| Markdown | Tracked active relative file links pass; historical/archive links checked separately without edits |

Unicode SHA-256:
`f5976f3c2149bb36d080d28edd464d064cc7ba995fc30a8905fb76e5987f6c92`.
CP949 SHA-256:
`e747cbecc43da02044ba379f08fea89d2376e17b1d832f30d5ae57b20479b5d1`.
External URLs/anchors are not part of the local-link checker. No committed
logo-to-BMP recipe exists: banner equivalence is checked, not logo regeneration.

## 11. Binary / linker comparison

Fixed-code evidence: `49af071484cccf4da05235e8cfe5454c2284bb1a`,
[preflight 33747941248](https://github.com/GimoXagros/GameYob/actions/runs/33747941248).
All sizes below are bytes. Debug metadata can change hashes in later commits.

| Item | Baseline a7fd75d | Fixed 49af071 | Delta |
| --- | ---: | ---: | ---: |
| gameyob.nds | 710144 | 710144 | 0 |
| gameyob_dsi.nds | 710144 | 710144 | 0 |
| ARM9 header payload | 589712 | 589968 | +256 |
| ARM7 header payload | 71848 | 71848 | 0 |
| ARM9 .text | 239912 | 240176 | +264 |
| ARM9 .rodata | 294872 | 294864 | -8 |
| ARM9 .data / .bss | 6032 / 215056 | 6032 / 215056 | 0 |
| ARM7 .text / .rodata | 36792 / 1488 | 36792 / 1488 | 0 |
| ARM7 .data / .bss | 33004 / 6212 | 33004 / 6212 | 0 |

No `warning:` compiler diagnostics found in baseline/fixed build logs. Git's
Windows LF/CRLF conversion notices are separate from compiler diagnostics.
The small ARM9 change is from the corrections/guards and revision metadata;
file padding absorbs it. This is not a measured performance improvement.

| Fixed NDS component | ROM offset | Header load | Entry | Size |
| --- | --- | --- | --- | ---: |
| ARM9 | 0x4000 | 0x02000000 | 0x02000800 | 589968 |
| ARM7 | 0x94200 | 0x02380000 | 0x02380000 | 71848 |
| ARM9i | 0xa6800 | 0x02400000 | — | 1800 |
| ARM7i | 0xa7000 | 0x02e80000 | — | 25620 |

ARM payload ranges do not overlap; ndstool parses both outputs. Header load
addresses are not all ELF runtime VMAs: SDK startup relocates sections.
Maps/readelf evidence is included with the CI artifact. Banner: offset
`0xa5e00`, version 1, 2,112 bytes. DLDI: offset `0x4b00`, reserved 16,384 bytes.
Both files use unit code 2 and identical program payloads. NDS game code
`####` / title ID `0x0003000423232323`; DSi variant `GYOB` /
`0x0003000447594f42`. A filename alone does not force execution mode.

ARM9 `__end__=0x020bd7f0`, `__eheap_end=0x02380000`; this is a linker span,
not measured free heap. Runtime ROM allocations consume it. IRQ/SVC stack
symbols are 256 bytes each; user-stack symbol is zero under SDK allocation.
Runtime heap/stack high-water and launcher-dependent margins: **not run**.

Fixed NDS SHA-256:
`4acc1146d3c3a887939fa9ab9c9b0556b18f68fea94497124d026500b253a776`.
Fixed DSi SHA-256:
`6ff5125fe2c4c6698ce9035a873dd45d40caffd3a4de3dd35647bf2a6a96ab78`.
Each matches its second clean build at that same commit/container. This does
not claim cross-toolchain reproducibility.

## 12–16. Upstream and branch classification

Initial master is **118 ahead, 0 behind** upstream/master. Upstream default
HEAD `45599fbb2f08b2f8042e7f9f47578ef078f2e889` already belongs to its history.
Recent upstream camera settings, DSi camera, ARM7 build and early-memory-access
fixes are therefore not missing. No automatic merge/rebase was appropriate.

Below, ahead/behind and unique counts are against initial master. `M/I/D`
means merged / identical / diverged. Tag means an exact tip tag, not whether
older release tags exist in a branch's ancestry. All listed branches were
unprotected and had no matching open PR head in their owning repository.

| Branch | Tip | Date | Last subject | Ahead/behind; unique | M/I/D | Tip tag | Delete / reason |
| --- | --- | --- | --- | --- | --- | --- | --- |
| local master | 04f8c83547ee | 2026-09-01 | Archive v0.5.9-ko release package | 0/0; 0 | Y/Y/N | none | No: current/default |
| origin/master | 04f8c83547ee | 2026-09-01 | Archive v0.5.9-ko release package | 0/0; 0 | Y/Y/N | none | No: default |
| upstream/master | 45599fbb2f08 | 2023-05-26 | CAMERA: Fixed settings menu | 0/118; 0 | Y/N/N | none | No: external default |
| upstream/asmcore | dec490f430ea | 2013-03-05 | Put assembly code in itcm | 4/670; 4 | N/N/Y | none | No: external/unique experimental core |
| upstream/newgfx | 250fe1e1c1e2 | 2013-04-11 | Implemented sprite priority bit | 12/539; 12 | N/N/Y | none | No: external/unique experimental renderer |
| upstream/nifi | 52bba56e4c5d | 2013-03-10 | FPS and clock work again | 0/629; 0 | Y/N/N | none | No: external historical branch |
| upstream/raspberry-pi | bf13bbf17925 | 2016-02-27 | More link output | 2/126; 2 | N/N/Y | none | No: external/legacy SDL target |
| upstream/v0.5-hotfix | db090e10894c | 2021-12-16 | Added MIT license | 13/229; 13 | N/N/Y | none | No: external historical release line |

`origin/HEAD` and `upstream/HEAD` are symbolic default references, not deletable
feature branches. There were no obsolete user-owned branches to remove.
Only the newly created maintenance branch is eligible after verified integration,
no PR/protection, no unique commits, and switching checkout back to master.

Historical differences are not automatically missing fixes: newgfx's scaling,
palette/flip and priority work overlaps today's DS renderer; asmcore is a
different experimental CPU architecture. Raspberry Pi patches replace SDL
OpenGL output and add debug logging, outside the active target. Hotfix's ini
rename, configurable touch, MIT notice and window-enable gate have current
counterparts (`config.cpp`, touch menu, root LICENSE, `gb_render::nextWindowLine`).
Its combined A+B+START+SELECT mapping is not imported as a new UI feature.
This is a scope/patch review, not a proof of full semantic equivalence of every
experimental branch; those branches are deliberately retained.

Upstream PRs #193 (netplay), #180 (ROM rollover), #169 (DSi touch) are unmerged
external heads, not this fork's PRs. They were not treated as approved fixes.
No GBARunner3/DSpico text-display problem was added to GameYob's TODO.

## 17–18. Documentation and notices

README/TODO separates validation, partial implementation, unknown hardware
specification and deferred native 3DS work. EN/JA/KO guides agree on mode
selection, BIOS, state-guard scope and the Pokemon Silver workaround. Existing
user hardware approval for RTC/reload/borders remains historical evidence,
not a claim that this commit was physically tested.

Root MIT, font OFL and stb notices remain intact. Inherited libfat notices and
file-specific ARM7/camera/build-recipe/legacy SDL attributions were recorded.
Existing GPL-marked Makefiles were not represented as MIT files or relicensed.
No third-party emulator implementation, ROM, BIOS or firmware was added.

**Open provenance item:** inherited `3in1.cpp` / `3in1.h` name aladdin/EZFlash
Group but no separate license grant was identified. Do not describe the entire
repository as license-cleared or publish a new release on this audit alone.
The root license was not used to invent missing permissions. Published ZIPs,
assets, tags, `old_releases`, archive sources and historical release records
were left unchanged.

## 19–20. Commits and post-fix checks

| Commit | Purpose |
| --- | --- |
| 5737ae53a1363bb6cf95e994f6175538e37018d9 | Repeatable preflight runner/workflow |
| 27ad3f42c8350735584a2fa909d1f9bf7f211b8a | Reproduce SGB defects before fixing |
| a7fd75d567fd52fbb094d7e1db3110f5087a88d0 | Independent probes and SDK path correction |
| b4148c3031ffbe1ed476065dfc00952867b8cfba | SGB correctness and packaged links |
| 4035016a2af22ed7024c0923d87cc14a36188ac7 | State header/prefix/bank guards |
| da339e8262d561a669972243d25c5a73a2db8225 | Preserve short historical state tails |
| 49af071484cccf4da05235e8cfe5454c2284bb1a | Resource, header and embedded-asset verification |
| c8e1bbd23e25720e47b51cf02f73f8dabc5f7927 | Trilingual docs and inherited notices |

At 49af071: 18 host suites pass normally and under UBSan; independent SGB
probes, package checks, Python compilation, resource checks, two clean builds
and binary/header checks pass. Separate [core run 33747941299](https://github.com/GimoXagros/GameYob/actions/runs/33747941299)
and [DS run 33747941347](https://github.com/GimoXagros/GameYob/actions/runs/33747941347)
also pass. No configured repository-wide formatter/linter exists; ASan,
coverage measurement and exhaustive fuzz/static analysis were **not run**.

## 21–26. Handoff boundaries

Pending hardware work: raw NiFi device matrix, DSi launchers/memory margins,
long-run/reload/state regression, touch input, rare cartridges and SGB timing.
Pokemon Silver Detect GBA On/Off requires matched ROM hash, BIOS/settings,
build and boot-vs-state reproduction; Off is a user-confirmed workaround,
not a demonstrated root-cause fix. Native 3DS remains deferred as one group.

Final delivery must recheck CI at the report-containing commit, master/rules,
PRs, branch containment, remote refs and `git status`. If green and unprotected,
fast-forward only; then normally delete just the integrated temporary branch.
No new release/tag, old archive mutation, reset/clean/force/rebase is allowed.
Preserve state version 8, existing config keys and file paths, current DS/TWL
payload conventions, localization and all existing license notices.

Development status after those gates: **READY WITH KNOWN LIMITATIONS**;
not hardware-verified, not a new stable release and not a legal clearance.
The next development baseline is the final master SHA supplied in the handoff,
not the v0.5.9-ko release tag.

## 한국어 요약

신규 기능·버전·릴리즈 없이 사전 정비만 수행했습니다. 재현된 SGB 분기/음수 시프트
결함, 상태 파일 공통 구간 및 뱅크 검사, 패키지 가이드 링크를 수정했습니다.
기존 16개 시험에 2개 C++ 회귀시험을 추가했고 정상/UBSan 모두 통과했습니다.
고정 BlocksDS 환경에서 DS·DSi 두 번 클린 빌드와 바이너리 일치, 배너·DLDI·폰트·
문자표 검사를 확인했습니다. 자동 검사는 실기 시험이나 전체 코드 무결성 보증이 아닙니다.

영어·일본어·한국어 가이드와 README/TODO/CHANGELOG를 함께 정리했습니다.
기존 배포물·태그·보류된 3DS 백업은 보존합니다. 실기 무선 링크, 구형 상태 파일,
가변 상태 데이터 검사, 희귀 카트리지/SGB 정확도, 포켓몬 은 GBA 감지 원인 조사는
남아 있습니다. EZFlash 유래 코드의 별도 허가 근거도 확인되지 않았으므로 저장소
전체의 배포 라이선스 검증 완료로 표현하지 않습니다. 다음 개발은 가능하지만 신규
배포 승인과 구분합니다. 최종 SHA·CI 결과·임시 브랜치 정리는 작업 완료 보고를 따릅니다.
