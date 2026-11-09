# Project Roadmap

## Current milestone: Stage 120N — Packed and dynamic relocation metadata parity

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

## Remaining major gates

1. Build liblzma for Android ARM64 and complete a real Android NDK build.
2. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
3. Finish s3eFileRead partial-item/error behavior using version-matched SDK documentation or controlled original-runtime traces.
4. Activate only individually verified guest imports.
5. Expand guest CPU instruction coverage and runtime diagnostics based on observed execution.
6. Establish graphics/audio/input lifecycle.
