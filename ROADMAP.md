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

## Current milestone: Stage 120P — APS2/RELR coexistence and dynamic relocation cross-checks

- Added ELF32 RELR direct-entry and bitmap decoding.
- Supports standard DT_RELR and Android DT_ANDROID_RELR aliases, including mixed aliases.
- Preserves optional DT_ANDROID_RELRCOUNT as a non-authoritative hint.
- Integrates RELR into ordinary dynamic and APS2 audits.
- Reports cross-format relocation target collisions without silently deduplicating them.
- Adds endian, malformed-tag, alignment, ordering, overflow, resource-bound, and coexistence fixtures.

See STAGE120P_REPORT.md and docs/STAGE120P_APS2_RELR_COEXISTENCE.md.

## Next milestone: Stage 120Q — Independent RELR differential validation and relocation-source reconciliation

1. Compare RELR expansion against independent toolchain/Bionic-produced vectors.
2. Validate DT_ANDROID_RELRCOUNT and mixed alias behavior across Bionic/API revisions.
3. Reconcile standard REL, PLT, APS2, and RELR target sets with provenance.
4. Preserve collision evidence and do not silently discard duplicate target reports.
5. Keep all audits read-only; no relocation application or runtime load-bias calculation.

## Remaining major gates

1. Build liblzma for Android ARM64 and complete a real Android NDK build.
2. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
3. Finish s3eFileRead partial-item/error behavior using version-matched SDK documentation or controlled original-runtime traces.
4. Activate only individually verified guest imports.
5. Expand guest CPU instruction coverage and runtime diagnostics based on observed execution.
6. Establish graphics/audio/input lifecycle.
