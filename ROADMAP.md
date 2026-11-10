# Project Roadmap

## Active milestone update — Stage 123E (import-cell identity and ARM thunk execution)

Stages 122D–122H and 123A–123E have host implementation coverage:
- Transactional multi-ELF image loading, ELF32 dependency/provider/relocation planning, and bounded constructor execution.
- Native XE3U header/fixup parser and bounded LZMA-alone decode adapter.
- Read-only audit of all 36,377 internal relocation entries; candidate-only C planner preflights and plans all 36,377 original records without writing.
- Verification of all 230 nonzero external type-4 ARM import stubs and read-only guest-`r12` cell-to-symbol lookup.
- ARM CPU support for the pre-indexed writeback word transfers used by the original import stub and shared helper.
- Synthetic end-to-end execution of the three-instruction import stub at `0x84` through shared helper `0x70`, resolver slot `0x13B704`, and a test resolver return path.

The re-uploaded original APK and key assets match Stage 97 SHA-256 hashes exactly. A native C harness planned all 36,377 original internal relocations at a test-only runtime base and a second native harness mapped 230/238 external records to verified stubs (the eight remaining cells correctly fail the stub check). Host CI passed in run `42875447333`. Android ARM64 NDK build and ELF verification passed in run `42875193411`. See [Stage 123C original asset validation](docs/STAGE123C_ORIGINAL_APK_REVALIDATION.md), [Stage 123D relocation evidence](docs/STAGE123D_INTERNAL_RELOCATION_EVIDENCE.md), and [Stage 123E import lookup/thunk execution](docs/STAGE123E_IMPORT_CELL_LOOKUP_AND_THUNK_EXECUTION.md).

This remains an engineering prototype: the actual S3E resolver pointer at `0x13B704` is zero in the original image, its initialization path and AAPCS32 contract are not yet known, and the original game has not booted.

### Critical next milestone — Stage 123F: shared resolver initialization and import dispatch

1. Identify who initializes XE3U slot `0x13B704` and what function ABI it expects.
2. Trace the S3E runtime's import-cell resolver path and establish how `r12` cell addresses become symbol-index/function mappings.
3. Define the guest-side resolver entry and return contract, including LR/SP preservation and stack switching, without assigning guessed addresses.
4. Classify the two zero-valued GL cells and define explicit bindings for the six allocator data cells.
5. Build a complete import manifest for all 227 symbols; keep unknown APIs disabled and fail closed.
6. Only after resolver semantics are proven, connect verified ARM64-side handlers through the guest AAPCS32 dispatcher.

### Remaining real-game gates

1. Run `tools/xe3u_native_audit` against the re-uploaded original S3E in the local repository build environment.
2. Complete Stage 123F shared resolver initialization, AAPCS32 call/return contract, and the 227-symbol import manifest.
3. Classify and bind the six allocator data slots and two remaining GL cells; no guessed addresses.
4. Finalize runtime image base, map the complete code/initialized-data/BSS memory extent, and apply the preflighted internal relocation plan transactionally.
5. Determine which runtime pieces from the separate ELF32 ARM `libjirobiodg.so` must be represented or reimplemented on ARM64; do not treat it as the XE3U main executable.
6. Connect archive/VFS/file APIs, runtime services, and ARM32 CPU execution; expand instruction coverage from the real executable's inventory.
7. Run Android NDK build, install on an ARM64 device, record loader/constructor/entry telemetry, then progress through menu, rendering, input, audio, and gameplay.

---



1. Run `tools/xe3u_native_audit` against the re-uploaded original S3E in the local build environment and compare its output with docs/STAGE123C_ORIGINAL_APK_REVALIDATION.md.
2. Validate representative internal pointer cells in code and initialized data against ARM/Thumb disassembly; establish exact data_offset and BSS interpretation.
3. Classify all 238 external Thumb relocation cells and determine exact thunk construction/branch semantics; unknown imports must remain unresolved.
4. Implement a read-only relocation report and a transactional planner; do not write guest memory until every record is validated.
5. Only after complete preflight, implement the proven add-load-bias operation while preserving pointer bit 0 and rejecting overflow/out-of-image results.
6. Inventory and load the actual ELF32 ARM shared object(s), including `libjirobiodg.so`, through the ELF32 pipeline as appropriate.
7. Bind verified S3E imports; unknown imports must fail closed. Connect archive/VFS/file APIs, runtime services, and ARM32 CPU execution.
8. Run the Android NDK build, install on an ARM64 device, record loader/constructor/entry telemetry, then progress through menu, rendering, input, audio, and gameplay.

---

## Historical milestone: Stage 120N — Packed and dynamic relocation metadata parity

GitHub Actions run 42614083636 passed 229 Python tests plus host C/native regressions. See STAGE120N_VALIDATION_LOG.txt.

- Carry bounded symbol size, definition state, section index, and symbol version into APS2 entries.
- Mark DT_SYMTAB namespace unresolved unless dynamic symbol bounds are proven.
- Apply R_ARM_COPY st_size only with complete symbol provenance.
- Correct APS2 decoding to AOSP Bionic semantics: absolute r_info, grouped info reuse, grouped addend applied once per group, per-entry addend deltas, and zero addend when HAS_ADDEND is absent.
- Verify sectionless PT_DYNAMIC/APS2 and ordinary dynamic/packed symbol metadata parity.

See STAGE120N_REPORT.md and docs/STAGE120N_PACKED_DYNAMIC_SYMBOL_PARITY.md.

## Current milestone: Stage 120O — Independent APS2 conformance and malformed-input hardening

- Added exact signed 32-bit SLEB128 fifth-byte validation.
- Added complete legal REL/RELA group-flag matrix tests and illegal combinations.
- Added cross-group addend reset/accumulation and offset state tests.
- Added resource-limit, truncation, malformed flag, and overlong encoding tests.
- See STAGE120O_REPORT.md and docs/STAGE120O_APS2_CONFORMANCE.md.

## Completed milestone: Stage 120P — APS2/RELR coexistence and dynamic relocation cross-checks

GitHub Actions run 42619189507 passed 272 Python tests and host C/native regressions. See STAGE120P_VALIDATION_LOG.txt.

- Added ELF32 RELR direct-entry and bitmap decoding.
- Supports standard DT_RELR and Android DT_ANDROID_RELR aliases, including mixed aliases.
- Preserves optional DT_ANDROID_RELRCOUNT as a non-authoritative hint.
- Integrates RELR into ordinary dynamic and APS2 audits.
- Reports cross-format relocation target collisions without silently deduplicating them.
- Adds endian, malformed-tag, alignment, ordering, overflow, resource-bound, and coexistence fixtures.

See STAGE120P_REPORT.md and docs/STAGE120P_APS2_RELR_COEXISTENCE.md.

## Completed milestone: Stage 120Q — RELR differential validation and Bionic metadata parity

GitHub Actions run 42640031158 passed all 273 Python tests and host C/native regression checks. The GNU readelf SHT_RELR differential test passed. See STAGE120Q_VALIDATION_LOG.txt.

- Corrected DT_ANDROID_RELRCOUNT behavior to match Bionic: ignored metadata, not a required RELR tag.
- Exposed the count hint at the top level of both dynamic and APS2 audit reports.
- Added a generated ELF32 ARM DYN fixture containing SHT_RELR.
- Added GNU readelf -rW differential comparison for decoded relocation target addresses.
- Added assertions that both audit pipelines preserve the non-authoritative count hint.

See STAGE120Q_REPORT.md and docs/STAGE120Q_RELR_DIFFERENTIAL_VALIDATION.md.

## Completed milestone: Stage 120R — PT_DYNAMIC RELR differential validation

Stage 120R added a section-header-independent PT_DYNAMIC RELR fixture, a GNU readelf differential test, and unified reconciliation checks. CI validation is tracked in PR #19 and must pass before the stage is marked complete.

## Implemented milestone: Stage 120S — ELF32 ARM PT_LOAD segment mapping and memory layout planning

A read-only PT_LOAD planner reports file-backed, memory, BSS zero-fill, page-rounded ranges, page overlaps, and combined permissions. CI validation remains pending in PR #20.

## Next milestone: Stage 120T — Guest memory mapping transaction

1. Validate PT_LOAD file-backed and memory ranges, including BSS zero-fill boundaries.
2. Calculate page-rounded ranges without allocating guest memory or modifying file contents.
3. Report page overlaps and combined permissions, especially writable/executable page combinations.
4. Keep ET_EXEC fixed-address and ET_DYN load-bias requirements explicit; do not choose a bias.
5. Add malformed-range, overflow, alignment, and entry-point regression fixtures.

Stage 120T introduces rollback-safe multi-region guest mapping. Failed requests remove only mappings created by the transaction; pre-existing regions remain intact. CI validation is pending.

## Stage 120U — ELF segment materialization

Added a preflighted helper to copy file-backed segment bytes and zero-fill BSS in existing writable guest mappings. It rejects invalid source ranges, file_size > memory_size, guest-address overflow, overlapping destination segments, and inaccessible destinations before modifying guest memory. CI validation is pending.

## Stage 120V — Guest permission finalization

Added an atomic all-or-nothing permission metadata update for exact guest mappings, with W+X, duplicate, missing mapping, invalid flags, and address-range rejection. This is software guest permission enforcement only; host mprotect/VirtualProtect and instruction-cache work are out of scope. CI validation is pending.

## Stage 120W — ARM32 relocation application

Added an all-or-nothing relocation subset for R_ARM_RELATIVE, R_ARM_ABS32, R_ARM_GLOB_DAT, and R_ARM_JUMP_SLOT. Symbol values are caller-supplied; no symbol lookup or shared-library loading is performed. CI validation is pending.

## Stage 120X — Dynamic symbol resolution

Added a bounded, version-aware resolver for pre-parsed provider symbols. It filters undefined/local/hidden/internal definitions, prefers a unique GLOBAL definition over WEAK, rejects multiple strong matches, and resolves batches atomically. ELF parsing, GNU version index translation, and dependency loading remain out of scope. CI validation pending.

## Stage 120Y — ELF dynamic symbol table integration

Added bounded DT_NEEDED extraction and complete dynamic-symbol inventory output to the PT_DYNAMIC auditor. A JSON adapter separates external provider definitions from undefined GLOBAL/WEAK requests and preserves version names and SONAME constraints. No library loading, search-path resolution, dependency scope inference, or relocation application. CI validation pending.

## Completed milestone: Stage 120Z — DT_NEEDED dependency graph resolution

Added a deterministic graph builder over caller-supplied adapter inventories. It validates SONAME uniqueness, reports unresolved dependencies, detects cycles, and emits dependency-first order only for complete acyclic graphs. No filesystem search, Android linker namespace emulation, loading, mapping, or relocation. Host source checks passed in GitHub Actions run 42805208188; Android ARM64 native build passed in run 42805208140. Original game boot and device gameplay remain unverified.

## Remaining major gates

1. Build liblzma for Android ARM64 and complete a real Android NDK build.
2. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
3. Finish s3eFileRead partial-item/error behavior using version-matched SDK documentation or controlled original-runtime traces.
4. Activate only individually verified guest imports.
5. Expand guest CPU instruction coverage and runtime diagnostics based on observed execution.
6. Establish graphics/audio/input lifecycle.
