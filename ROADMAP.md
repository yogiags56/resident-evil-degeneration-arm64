# Project Roadmap

## Active milestone update — Stage 123H (original ELF32 provider and GL dispatch audit)

Stages 122D–122H and 123A–123F have host implementation coverage:
- Transactional multi-ELF image loading, ELF32 dependency/provider/relocation planning, and bounded constructor execution.
- Native XE3U header/fixup parser and bounded LZMA-alone decode adapter.
- Read-only audit of all 36,377 internal relocation entries and a candidate-only C planner.
- Verification of all 230 nonzero external ARM import stubs, identity lookup from guest `r12`, and transactional symbol-specific Thumb import planning/apply for all 238 type-4 cells.
- ARM CPU support for original import-stub writeback, ARM/Thumb PC interworking, and Thumb PUSH/POP.
- End-to-end synthetic execution from the original ARM import stub through the existing Thumb import dispatcher, plus rollback/stale-memory tests for external patch application.

Host CI passed in run `42877554684`; Android ARM64 NDK build/ELF verification passed in run `42877209331`. The original APK and key assets match Stage 97 hashes exactly.

A deeper Thumb audit shows those 22 exports are lazy GL dispatch wrappers, not direct GLES implementations. All share the zero-filled BSS dispatch table at `0x747dc`, have exact name literals and `R_ARM_RELATIVE`-relocated table-base literals, and cache the returned function pointer in their slot after calling the common helper at `0x3d64c`. Helper `0x3d64c` is now proven to call `dlsym` through guest PLT wrapper `0x35a0c`, searching three handles at global structure offsets `+0x34`, `+0x38`, and `+0x3c`. The corresponding `dlopen`/`dlclose`/`dlerror` wrappers exist; the handle initialization and ownership path remain unknown. See [Stage 123H GL wrapper audit](docs/STAGE123H_GL_LAZY_DISPATCH_WRAPPERS.md), [Stage 123G provider coverage](docs/STAGE123G_LIBJIROBIODG_IMPORT_PROVIDER_COVERAGE.md), [Stage 123F external import binding](docs/STAGE123F_TRANSACTIONAL_EXTERNAL_IMPORT_BINDING.md), [Stage 123E thunk execution](docs/STAGE123E_IMPORT_CELL_LOOKUP_AND_THUNK_EXECUTION.md), and [Stage 123D relocation evidence](docs/STAGE123D_INTERNAL_RELOCATION_EVIDENCE.md).

### Active milestone — Stage 123H: load original ELF32 provider and bridge dependencies

1. Complete the read-only audit of the original `libjirobiodg.so` program headers, dynamic symbols, relocations, ARM attributes, and JNI lifecycle; current evidence is recorded in `docs/STAGE123H_GL_LAZY_DISPATCH_WRAPPERS.md`.
2. Reconstruct initialization and ownership of the three `dlopen` handles at global structure `0x737f0` offsets `+0x34/+0x38/+0x3c`; trace who opens the libraries, which libraries they are, and how `dlsym` results become ARM32-callable Thumb pointers. Preserve the observed `dlsym` search order.
3. Trace `JNI_OnLoad` and the Java/native startup path to determine whether it initializes the callback state or only registers JNI methods.
4. Build a read-only relocation/provider plan for the original ELF32 module. Select no load bias until its `0x8000` PT_LOAD alignment and guest address-space reservation are accounted for.
5. Audit the six `DT_NEEDED` libraries (`libc.so`, `libm.so`, `liblog.so`, `libstdc++.so`, `libz.so`, `libdl.so`) and map all 137 relocation-referenced undefined names to verified providers or explicit guest bridges.
6. Bridge only verified system APIs to ARM64 handlers; never load ARM32 system libraries through the ARM64 process linker.
7. Keep the remaining XE3U imports fail-closed until matched to a verified guest provider or host handler.

### Remaining real-game gates

1. Complete Stage 123H actual ELF32 provider loading and dependency bridging.
2. Build the production 227-symbol import manifest; resolve file APIs through the audited VFS bridge, allocator cells through explicit allocator callbacks, and GL cells through verified providers.
3. Implement internal relocation application from the candidate plan and zero-fill the memory-only tail; verify the `data_offset` split and code/data/BSS permissions.
4. Connect LZMA decode, XE3U header/fixup parse, image mapping, internal relocation apply, external import plan/apply, and runtime entry into one rollback-safe loader.
5. Run Android NDK build, install on an ARM64 device, record loader/constructor/entry telemetry, then progress through menu, rendering, input, audio, and gameplay.

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
