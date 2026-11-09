# Project Roadmap

## Current milestone: Stage 108 complete

The project now has a standalone offline-testable XZ archive verifier covering checksum, tar paths/links, and version identity; the fetcher delegates to it, and the toolchain doctor checks sdkmanager availability. Android SDK/NDK provisioning and ARM64 liblzma remain pending.

## Next milestone: Stage 109 — Android SDK/NDK Provisioning + ARM64 liblzma Build

Provision Android SDK/NDK and Gradle wrapper, fetch and verify XZ Utils 5.8.4, build `liblzma.a` for `arm64-v8a` API 23, and verify each archive member as ELF64/AArch64 before attempting the application build.

## Remaining major gates

1. Build liblzma for Android ARM64 and complete a real Android NDK build.
2. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
3. Finish `s3eFileRead` partial-item/error behavior using version-matched SDK documentation or controlled original-runtime traces.
4. Activate only individually verified guest imports.
5. Expand guest CPU instruction coverage and runtime diagnostics based on observed execution.
6. Establish graphics/audio/input lifecycle and observe an original game boot.

See `ROADMAP_THROUGH_STAGE100.md`, `STAGE100_REPORT.md`, `ROADMAP_THROUGH_STAGE99.md`, `STAGE99_REPORT.md`, `ROADMAP_THROUGH_STAGE98.md`, `STAGE95_REPORT.md`, `STAGE96_REPORT.md`, `STAGE97_REPORT.md`, and `STAGE98_REPORT.md` for details.


## Stage 101 — Android Asset Packaging + ARM64 Build Readiness

- Added local-only APK asset staging for `main.dz`, `biohazard.s3e`, and `s3e.icf`.
- Added SHA-256 manifest generation and validation for missing/duplicate assets and DTRZ magic.
- Android native probe checks that all required payloads are packaged.
- Host suite: 27 Python tests and all C regression tests pass.
- Android NDK build remains blocked because `ANDROID_NDK_HOME` is not configured.
- Production guest imports remain disabled; original `s3eFileRead` partial/error behavior is unresolved.

Next: Stage 102 — real Android NDK build, ELF64/AArch64 inspection, APK packaging, and ARM64 runtime probe.


## Stage 102 — Android ARM64 Build Orchestration + ELF Verification

- Gradle now requires liblzma-enabled native build and passes its prefix to CMake.
- Added one-command Android build script and ELF64/AArch64/JNI verifier.
- Host suite remains green: 27 Python tests plus all C regression tests.
- Android build remains blocked: no configured NDK, no ARM64 liblzma prefix, and no Gradle wrapper.
- No Android ELF/APK/device test or original game boot is claimed.

Next: Stage 103 — provision Android toolchain/dependency, perform real build, inspect ELF and test packaged assets on ARM64.


## Stage 103 — ARM64 liblzma Dependency Build Automation

- Added reproducible XZ Utils/liblzma cross-build script for Android `arm64-v8a`, API 23, static/PIC.
- Android build script now accepts either repository Gradle wrapper or system Gradle 8.9+.
- NDK preflight checks host-specific LLVM clang.
- Added tests for build configuration contracts.
- Host tests pass; Android cross-build remains blocked because NDK and XZ source are unavailable.
- No Android library, APK, device execution, or game boot is claimed.

Next: Stage 104 — build liblzma with a real NDK, build and inspect the Android shared library, then test APK assets on ARM64.


## Stage 104 — liblzma CMake Integration + ARM64 Archive Verification

- Added host CMake smoke test using the production LZMA decoder and `LibLZMA::LibLZMA`.
- Host liblzma 5.8.1 linked successfully; all 6 decoder scenarios passed.
- Added archive member verifier requiring ELF64/AArch64; negative test rejected x86_64 archive.
- Added 4 configuration contract tests; total Python suite: 35 passed.
- Android cross-build remains blocked by missing NDK and XZ source. Host test is not Android validation.

Next: Stage 105 — obtain NDK/XZ, build and verify Android ARM64 liblzma, then compile and inspect the Android native library.


## Stage 107 — Dependency Acquisition & SDK Provisioning Automation

- Pinned XZ Utils 5.8.4 archive SHA-256 and Android NDK r30 Linux package metadata.
- Added a fetch script with archive checksum verification, tar path traversal rejection, and extracted version-header verification.
- Added SDK package installer for platform-tools, Android 35, Build Tools 35.0.0, CMake 3.22.1, and NDK 30.0.16248370.
- Android liblzma build now fetches the pinned XZ source automatically if no source directory is supplied.
- 48 Python tests and host regressions pass.
- Android SDK/NDK and XZ source archive were not provisioned in this environment; no ARM64 library, APK, device boot, or playable state is claimed.
- See `STAGE107_REPORT.md` and `ROADMAP_THROUGH_STAGE107.md`.
\n\n## Stage 108 — XZ Archive Verification Hardening + SDK Diagnostics\n\n- Added standalone offline XZ source archive verifier: SHA-256, safe tar paths/links, unique version header, exact version.\n- Fetch script now delegates to the verifier before extraction.\n- Added five offline security/identity fixture tests.\n- Toolchain doctor now checks sdkmanager discovery paths.\n- No actual XZ source download, Android SDK/NDK provisioning, ARM64 liblzma build, Android APK, or device run is claimed.\n- See `STAGE108_REPORT.md`.\n

## Stage 110 — Android CLI migration\n\nThe SDK installer now prefers Google's official Android CLI (`android sdk install`), with deprecated `sdkmanager` retained only as a compatibility fallback. The doctor accepts either route. See `STAGE110_REPORT.md` and `ROADMAP_THROUGH_STAGE110.md`.\n\nNext: Stage 111 — provisioned ARM64 liblzma build.\n
Stage 110 additionally hardened the ARM64 liblzma archive verifier to inspect every object member, not just the first. Two offline tests validate multi-member success and rejection of a bad second member.


## Stage 111 — Reproducible Android ARM64 CI\n\nAdded a checksum-verified Gradle 8.9 installer and safe ZIP verifier, plus a GitHub Actions workflow that provisions Android CLI/SDK/NDK, builds XZ/liblzma for ARM64, builds the app, and verifies ELF output. The workflow has not yet been run on GitHub. See `STAGE111_REPORT.md` and `ROADMAP_THROUGH_STAGE111.md`.\n\nNext: Stage 112 — first real CI build.\n
Stage 111 validation: 69 Python tests, all host C regression tests, six host LZMA smoke scenarios, and 15 shell syntax checks passed. No Android SDK/NDK or ARM64 build output exists yet; the GitHub Actions workflow has not run.


## Stage 112 — CI artifact validation and provenance

Added a strict artifact collector that requires the ARM64 native library to be packaged in the APK, rejects proprietary original game assets, and emits a build manifest plus SHA-256 list. GitHub Actions now uploads one curated bootstrap/probe artifact for 14 days. This is not a playable game build. The workflow still awaits a real remote run because no matching GitHub repository was connected in this session. See `STAGE112_REPORT.md` and `ROADMAP_THROUGH_STAGE112.md`.

Next: Stage 113 — NDK environment handoff and artifact consistency.

## Stage 113 — NDK environment handoff and artifact consistency

Fixed the cross-step NDK environment handoff by persisting the verified NDK root through GitHub Actions `GITHUB_ENV`, and added a workflow gate for the exact pinned NDK revision. The artifact collector now compares the standalone ARM64 `.so` hash against the library embedded in the APK. See `STAGE115_REPORT.md` and `ROADMAP_THROUGH_STAGE115.md`.

Next: Stage 114 — GitHub repository bootstrap.

Stage 113 validation: 76 Python tests, host C regression tests, six host LZMA smoke scenarios, 15 shell syntax checks, and workflow YAML parsing passed. No remote Android build has yet been executed.

## Stage 114 — GitHub repository bootstrap

Stage 114 adds `tools/publish_github_repository.sh`, a one-command GitHub CLI publisher. It creates a private repository by default, derives the owner from the authenticated `gh` account, commits the source tree when required, and pushes it. At Stage 115, the publisher is tightened to private-only visibility. The script refuses to repoint an existing `origin` and blocks proprietary assets and generated binaries from entering the Git index. Tests cover the security gates and command construction.

The connected GitHub app currently exposes repository search and Git data operations but no repository-creation action; no online repository has been created or pushed by this session. User-side `gh auth login` and repository-creation permission are required to execute the publisher. Stage 114 does not claim an Android build or playable game.


## Stage 115 — Private publication and first CI dispatch

The publication helper now enforces private visibility (no public override), pushes the source on `main`, and dispatches `.github/workflows/android-arm64.yml` after successful publication. If dispatch fails, the repository remains published and the script prints a safe retry command. Workflow artifact naming and Android versionName advance to Stage 115. This session still cannot create the remote repository directly: the available GitHub connection exposes no repository-create action, and no authenticated `gh` CLI exists in this runtime. No remote run or Android APK is claimed.



## Stage 119M — ARMv7 unaligned access research (complete: documentation only)

- Audited the Stage 119J bus and Stage 119K/L A32 transfer helper against the ARMv7-A/R Architecture Reference Manual.
- Recorded instruction-specific behavior controlled by SCTLR.A, including families that always require alignment.
- Recorded Normal versus Device/Strongly-ordered memory restrictions, PC-load restrictions, and non-atomic / multi-access fault caveats.
- Kept runtime strict alignment unchanged; memory type and guest SCTLR.A are not modeled yet.
- Added `docs/STAGE119M_ARMV7_UNALIGNED_ACCESS_RESEARCH.md`.
- No implementation or test behavior changed. CI for the research branch is pending.

### Next — Stage 119N: guest memory attributes + alignment policy

1. Extend guest mapped regions with explicit memory type attributes (Normal, Device, Strongly-ordered or a deliberately conservative Unknown).
2. Preserve existing mapping call behavior with a safe default; do not silently classify unknown regions as Normal.
3. Add explicit CPU alignment policy representing guest SCTLR.A, defaulting to strict until guest system-control state is evidenced.
4. Test all supported scalar transfer combinations, including cross-region / mixed-attribute rejection and permission faults.
5. Keep production interpreter migration disabled until the policy layer and differential tests pass.

### Later milestones

- Stage 119O: implement instruction-specific ARMv7 scalar transfer semantics through the policy layer.
- Stage 119P+: migrate narrowly scoped A32/Thumb instruction families behind opt-in checked execution and differential regression coverage.
- Then: XE3U loader/relocations, complete guest address-space mapping, import-table activation, renderer/audio/input lifecycle, and real-device boot/gameplay validation.

**Important:** a green host test suite or Android ARM64 APK build does not mean the original game is playable. Physical-device boot and gameplay remain separate acceptance gates.

## Stage 119N — Guest memory attributes + alignment policy

- Extended guest memory regions with UNKNOWN, NORMAL, DEVICE, and STRONGLY_ORDERED attributes.
- Added re_guest_memory_map_ex; the legacy re_guest_memory_map remains source-compatible and defaults to UNKNOWN.
- Added bounded memory-type query.
- Added a conservative SCTLR.A alignment-policy model, strict by default.
- Added instruction-class-aware alignment checks and regression tests for scalar, exclusive, doubleword, multiple, SWP, coprocessor, and memory-type cases.
- Registered the new source in Android CMake and host test runner.
- The policy is decision-only: the memory bus and production CPU remain strict; no unaligned load/store implementation is active.
- CI for this branch is pending.

### Next — Stage 119O: checked scalar transfer semantics

1. Add opt-in byte-assembled unaligned scalar reads/writes for eligible Normal-memory ranges only.
2. Keep strict behavior for UNKNOWN, Device, Strongly-ordered, SCTLR.A=1, and instruction families requiring alignment.
3. Test little-endian behavior, bounds, permissions, unmapped and cross-region accesses, and failure non-mutation.
4. Add isolated A32 LDR/STR and halfword forms and differential tests before production decoder migration.
5. Keep instruction fetch alignment unchanged.

Stage 119P+ will migrate narrow instruction families behind checked execution, followed by XE3U loading/relocations, import-table activation, renderer/audio/input, and physical-device boot/gameplay validation.

## Stage 119O — Checked scalar transfer semantics

- Added an isolated CPU-facing byte/halfword/word data-access helper.
- Uses explicit little-endian byte assembly; does not rely on host alignment or host endianness.
- Consults Stage 119N memory-type metadata and SCTLR.A policy.
- Permissive unaligned scalar access is eligible only for complete ranges inside NORMAL memory when SCTLR.A is clear.
- UNKNOWN, DEVICE, STRONGLY_ORDERED, unmapped, cross-region, and strict-mode unaligned accesses fail closed.
- Resolves full range and permissions before reads/writes; failed stores do not mutate backing memory and failed loads do not update output.
- Added host regression test and Android CMake registration.
- Does not modify the checked bus, Stage 119K/L A32 decoder helper, or production interpreter. CI pending.

### Next — Stage 119P: isolated A32 scalar instruction integration

1. Add opt-in ordinary A32 LDR/STR and halfword transfer variants that explicitly use the new scalar helper.
2. Preserve strict legacy path as fallback and keep PC/writeback/condition semantics outside the supported subset.
3. Add differential tests against the legacy interpreter for aligned and unaligned addresses, pre/post indexing only when semantics are fully specified, and fault/commit behavior.
4. Do not connect the new path to production `re_guest_cpu_step` until differential coverage and exception behavior are adequate.

Later milestones: broader ARM/Thumb migration, guest SCTLR/system-control emulation, XE3U loader/relocations, import table, renderer/audio/input, and device boot/gameplay verification.

## Stage 119P — Isolated A32 scalar transfer integration

- Added opt-in A32 helper for AL-condition immediate pre-index/no-writeback scalar transfers.
- Supports ordinary LDR/STR word and byte, plus LDRH/STRH/LDRSB/LDRSH.
- Uses Stage 119O checked scalar memory operations and Stage 119N alignment/memory attributes.
- Signed loads sign-extend; destination registers are committed only after successful loads.
- Added regression tests and Android CMake/host runner registration.
- Not connected to production `re_guest_cpu_step`; current CI pending.

### Next — Stage 119Q: differential and architectural corner-case validation

1. Build broader differential coverage for the supported A32 instruction subset against the legacy interpreter/reference model.
2. Verify positive/negative offsets, register preservation, signed loads, byte/halfword/word data patterns, and all fault cases.
3. Add tests for condition-code behavior and reject unsupported encodings before side effects.
4. Research guest SCTLR reset/write semantics and the exact target architecture profile.
5. Keep production CPU dispatch unchanged until differential and exception behavior evidence is sufficient.

Later milestones: guest system control/MMU semantics, narrow checked-step integration, broader ARM/Thumb migration, XE3U loader/relocations, import table, renderer/audio/input, and physical-device boot/gameplay validation.

## Stage 119Q — A32 scalar differential reference tests

- Added deterministic independent byte-array reference model.
- Runs 512 reproducible cases over word/byte/halfword and signed load/store forms.
- Compares all CPU registers and the full guest-memory fixture after each operation.
- Registered in the host test runner.
- CI pending; no production CPU dispatch changes.

### Next — Stage 119R: directed decoder and fault validation

1. Verify the latest Stage 119P regression vectors and fix instruction-encoding/offset mistakes.
2. Expand directed negative tests for unsupported condition codes, register offsets, post-index, writeback, PC operands, and invalid extra-transfer encodings.
3. Test read-only store rejection, unmapped loads, and destination preservation using the A32 decoder API.
4. Check address wraparound and boundary behavior.
5. Keep the helper isolated until host CI and Android ARM64 build pass on the same source revision.

## Stage 119R — Directed A32 decoder and fault validation

- Added directed read-only memory, cross-region, address-wrap, and subtraction-underflow tests.
- Added rejection tests for unsupported condition, post-index, writeback, PC operands, register-offset, LDRD, STRSB, and STRSH.
- Tests verify failed stores do not mutate backing memory and failed loads preserve destination registers.
- CI pending.

### Next — Stage 119S: guest system-control profile research

1. Identify the target ARM architecture/profile from the game runtime and platform ABI evidence.
2. Research guest SCTLR reset and write semantics from that profile.
3. Determine whether the existing SCTLR.A policy can be tied to actual guest system-control state.
4. Keep unknown control bits conservative and avoid inventing reset values.
5. Do not enable production instruction dispatch until control-state and exception behavior are evidence-backed.

## Stage 119S — Guest ARM architecture-profile evidence audit

- Confirmed ARM32/Thumb guest code and AAPCS32 import argument convention from existing repository analysis.
- Distinguished Android ARM64 host ABI from the emulated ARM32 guest.
- Identified unproven target attributes: exact ARM revision/profile, optional ISA extensions, VFP/NEON, endianness control, privilege/MMU model, and SCTLR reset/write semantics.
- Decided not to invent a guest CPU profile or SCTLR reset state.
- Documentation-only stage; original guest executable/metadata is not present as a reproducible repository input.

### Next — Stage 119T: guest binary metadata and instruction inventory

1. When a permitted original guest executable or sample is available, extract ELF class, endianness, machine, ABI flags, program headers and ARM attributes.
2. Inventory A32/T32 instruction features and optional VFP/NEON use.
3. Trace runtime startup/system-control initialization where evidence exists.
4. Separate binary ABI evidence from runtime register reset behavior.
5. Do not add production SCTLR state based on guesses.


## Stage 119T — Guest ELF and ARM ABI metadata audit

- Added a read-only ELF32/ELF64 metadata auditor.
- Reports class, endianness, machine, flags, section names, and selected AEABI ARM attributes.
- Added synthetic little-/big-endian ELF fixtures and malformed-input tests.
- Does not execute or modify the supplied file; original game executable remains absent.
- Does not disassemble code or prove runtime SCTLR/MMU state.
- Host source checks and Android ARM64 native build passed on commit ffd61f9.

## Stage 119U — ARM attribute scopes and external evidence capture

- Extended public AEABI parsing to file, section, and symbol scope sub-subsections.
- Added special Tag_compatibility and argument-free Tag_nodefaults encodings.
- Preserved private vendor payloads as opaque data.
- Added readelf/llvm-readobj capture with tool version, source hash, timeout, exit status, and raw output.
- Synthetic tests added; CI pending.

### Next — Stage 119V: instruction inventory and executable metadata cross-check

1. Add optional pinned disassembler support with explicit ARM/Thumb mode selection.
2. Distinguish decoded instruction counts from raw byte coverage and decode failures.
3. Cross-check ELF attributes against observed instruction classes without treating attributes as proof of instruction use.
4. Record tool versions and hashes; do not store proprietary binaries or full symbol dumps in the repository.
5. Use evidence to prioritize missing guest CPU instructions before enabling production dispatch.


## Stage 119V — ARM/Thumb instruction inventory

- Added optional Capstone-based instruction inventory for ELF32 EM_ARM executable sections.
- Requires explicit ARM or Thumb mode; no guessed automatic mode.
- Reports raw executable bytes, decoded bytes, undecoded ranges, mnemonic counts, capped samples, decoder version, and source SHA-256.
- Added mocked decoder tests for mode, byte accounting, gaps, and section filtering.
- Mixed-mode mapping symbols and real guest instruction inventory remain pending; original binary is absent.

### Next — Stage 119W: ARM mapping symbols and mode-aware section decoding

1. Parse .symtab/.strtab and local mapping symbols $a, $t, $d (including suffixed forms).
2. Split executable sections into ARM, Thumb, and data ranges.
3. Decode each range with the correct mode and preserve undecoded gaps.
4. Add symbol-table bounds, relocation, and malformed mapping-symbol tests.
5. Only then run the inventory on the original guest ELF when a permitted copy is available.


## Stage 119W — ELF ARM mapping symbols

- Parse ELF32 symbol/string tables and local mapping symbols $a, $t, $d, including suffixed forms.
- Split executable sections into mode-aware code, data, and unknown ranges.
- Decode ARM and Thumb ranges independently; do not send data ranges to Capstone.
- Distinguish decoder-undecoded bytes, data bytes, and unmapped bytes.
- Add synthetic mixed-mode ELF tests and fail-closed handling for conflicting symbols and malformed tables.
- Original game executable remains absent; no game-specific inventory claimed.

### Next — Stage 119X: relocation-aware disassembly and symbol cross-check

1. Parse REL/RELA relocation tables and validate links, entry sizes, and symbol indices.
2. Annotate decoded branch/call and literal references where relocation evidence exists.
3. Cross-check mapping symbols, function symbols, and executable ranges.
4. Preserve unknown relocations without guessing target addresses.
5. Add synthetic relocation fixtures before analyzing the original binary.


## Stage 119X — ARM relocation audit

- Add ELF32 EM_ARM SHT_REL and SHT_RELA parser.
- Resolve relocation symbol references through conventional SHT_SYMTAB/SHT_STRTAB.
- Validate sh_link, sh_info, entry sizes, symbol indices, target offsets, and file bounds.
- Report common ARM relocation names; retain unknown numeric types without inferred meaning.
- Keep REL implicit addends untouched; report explicit RELA addends.
- Dynamic relocations, APS2/RELR, extended symbol indices, and ELF64 remain future work.

### Next — Stage 119Y: Relocation/mapping-symbol cross-check and control-flow references

1. Correlate relocation target offsets with $a/$t/$d ranges.
2. Flag relocations landing in data or unknown ranges.
3. Annotate only symbol-backed call/jump relocations; do not resolve final addresses without link context.
4. Add ET_EXEC/ET_DYN virtual-address fixtures and malformed table cases.


## Stage 119Y — Relocation/mapping cross-check

- Correlate relocation target offsets with ARM, Thumb, data, unknown, and non-executable ranges.
- Flag CALL/JUMP relocations outside mapped ARM/Thumb code.
- Distinguish undefined/external symbols, absolute symbols, and ordinary defined-section symbols.
- Do not infer final branch targets, PLT/GOT resolution, or Marmalade import identity.
- Add deterministic synthetic cross-check tests.

### Next — Stage 119Z: Function-symbol and relocation target inventory

1. Parse STT_FUNC and mapping-symbol ranges together.
2. Identify function symbols whose ranges overlap data or unmapped bytes.
3. Annotate call/jump relocation symbol references and report unresolved/undefined symbols.
4. Add ET_EXEC/ET_DYN target-address validation and malformed ELF tests.
5. Preserve symbol provenance; do not infer absent names.


## Stage 119Z — Function-symbol inventory

- Parse ELF32 ARM STT_FUNC records from conventional SYMTAB/DYNSYM.
- Preserve binding, visibility, section index, symbol value, size, and Thumb state bit.
- Compare declared function extents against $a/$t/$d ranges.
- Report mixed-mode, data-overlap, unknown, zero-size, and out-of-section cases without repairing metadata.
- Correlate CALL/JUMP relocation references using symbol-table section index plus symbol index, not symbol names alone.
- Never apply relocations or infer final control-flow targets.

### Next — Stage 120A: Runtime import symbol inventory

1. Inventory undefined dynamic symbols and associated symbol versions.
2. Inspect .dynsym, .dynstr, .dynamic, PLT/GOT and dynamic relocation sections.
3. Identify import names only when explicitly present in binary metadata.
4. Keep S3E/Marmalade import identities provisional until matching known ABI/API evidence exists.


## Stage 120A — Dynamic import inventory

- Inventory ELF32 EM_ARM SHT_DYNSYM and named undefined symbols.
- Parse SHT_DYNAMIC entries and DT_NEEDED / SONAME / RPATH / RUNPATH strings.
- Parse dynamic REL/RELA sections linked to DYNSYM and correlate by symbol index.
- Preserve unknown tags and relocation numbers.
- Do not infer S3E/Marmalade API identity or apply relocations.

### Next — Stage 120B: Dynamic address tables and PLT/GOT cross-reference

1. Parse DT_PLTGOT, DT_JMPREL, DT_PLTRELSZ, DT_PLTREL and dynamic address/size pairs.
2. Correlate PLT/JUMP_SLOT entries with undefined dynamic symbols.
3. Add PT_DYNAMIC/PT_LOAD mapping so stripped ELF files can be audited without section headers.
4. Record unresolved and unsupported packed-relocation formats explicitly.


## Stage 120B — PLT/GOT cross-reference

- Report DT_PLTGOT, DT_JMPREL, DT_PLTRELSZ, DT_PLTREL.
- Correlate section-backed R_ARM_JUMP_SLOT entries to exact dynamic symbols.
- Preserve unresolved/unknown PLT metadata.
- Do not decode PLT stubs, infer GOT runtime contents, or apply relocations.

### Next — Stage 120C: PT_DYNAMIC / PT_LOAD program-header fallback

1. Parse ELF32 program headers independently of section headers.
2. Map dynamic virtual addresses through PT_LOAD file-backed ranges.
3. Parse PT_DYNAMIC tags for stripped ELF inputs.
4. Cross-check section-backed and segment-backed dynamic metadata when both exist.
5. Keep BSS-only virtual ranges distinct from file-backed bytes.


## Stage 120C — PT_DYNAMIC / PT_LOAD parser

- Parse ELF32 EM_ARM program headers independently of section headers.
- Map virtual addresses only through unique PT_LOAD file-backed ranges.
- Parse PT_DYNAMIC through DT_NULL.
- Decode DT_NEEDED and related strings through DT_STRTAB/DT_STRSZ.
- Reject BSS-only reads and ambiguous mappings.

### Next — Stage 120D: Dynamic relocation segment mapping

1. Map DT_REL / DT_RELA and DT_JMPREL address ranges through PT_LOAD.
2. Validate DT_RELSZ/DT_RELENT and DT_RELASZ/DT_RELAENT.
3. Cross-check section-backed relocation entries when section headers exist.
4. Preserve unknown relocation types and reject malformed entry ranges.
5. Defer Android APS2/RELR decoding to a separate evidence-backed substage.


## Stage 120D — Dynamic relocation segment mapping

- Parse DT_REL/DT_RELA table metadata and entries.
- Parse DT_JMPREL and its REL/RELA selector.
- Map relocation ranges through unique PT_LOAD file-backed regions.
- Correlate symbol names when bounded SysV DT_HASH or GNU DT_GNU_HASH metadata bounds DT_SYMTAB.
- Do not apply relocations or infer runtime providers.

## Stage 120E — Android APS2 packed relocations

- Added an ELF32 EM_ARM parser for DT_ANDROID_REL/RELSZ and DT_ANDROID_RELA/RELASZ streams.
- Decodes bounded SLEB128, relocation counts, group flags, offset/info deltas, and cumulative RELA addends.
- Maps packed streams through file-backed PT_LOAD ranges and reuses bounded SysV DT_HASH or GNU DT_GNU_HASH symbol resolution.
- Added synthetic tests for REL/RELA, grouped/ungrouped records, malformed streams, overflow, and mapping bounds.
- Does not apply relocations, dereference targets, infer providers, or decode RELR.

## Stage 120F — Dynamic relocation reconciliation

- Unified section-backed REL/RELA, PT_DYNAMIC REL/RELA/JMPREL, and Android APS2 REL/RELA inventories.
- Normalizes target domain/address, relocation type, symbol index, addend, source table, and symbol namespace.
- Reports exact duplicates only within the same symbol-table namespace.
- Reports cross-namespace tuple matches as candidates, not proven duplicates.
- Reports exact and partial file-range overlaps independently from semantic matches.
- Preserves all source records and raw counts; never silently deduplicates.
- Supports stripped ELF through the section-header-independent PT_DYNAMIC/PT_LOAD path.
- GitHub Actions Source checks passed: 147 Python tests plus host C/native regression tests (run 42598567251).

## Stage 120G — Dynamic symbol namespace and GNU hash bounds

- Implemented DT_GNU_HASH parsing for ELF32 EM_ARM.
- Validates header dimensions, bloom filter, buckets, chain bounds, and terminators through PT_LOAD file-backed ranges.
- Derives safe dynamic symbol count for GNU-hash-only binaries.
- Validates GNU chain hashes and bloom membership against symbol names.
- Cross-checks GNU-derived bounds against SysV DT_HASH nchain when both exist.
- Shares the resolver with ordinary dynamic and APS2 relocation audits.
- GitHub Actions run 42599248679 passed 154 Python tests and host C/native regression checks.

### Next — Stage 120H: Dynamic version metadata and symbol identity

1. Parse DT_VERSYM through bounded PT_LOAD reads.
2. Parse DT_VERDEF and DT_VERNEED headers and auxiliary chains.
3. Correlate symbols using version indices and names, not numeric symbol index alone.
4. Preserve hidden, local, weak, and unresolved version states.
5. Reject malformed linked lists, cyclic chains, and invalid string/index offsets.
6. Add little-endian and big-endian fixtures.
7. Keep provider lookup and relocation application disabled.
