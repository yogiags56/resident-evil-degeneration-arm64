# Project Roadmap

## Active milestone update — Stage 123S (guest C++ new/delete heap provider)

Stage 123S adds guest ARM32 adapters for the four relocation-backed C++ allocation imports _Znwj, _Znaj, _ZdlPv, and _ZdaPv. It uses a dedicated guest RW arena, bounded host-side allocation records, 8-byte-aligned first-fit allocation, exact-address release, and guest-address-only pointers. Host regression passed; Android ARM64 build validation is pending. It does not implement C malloc/free, C++ exceptions, std::bad_alloc, sized/aligned/nothrow new/delete, or production heap lifecycle integration. See docs/STAGE123S_GUEST_CXX_HEAP_PROVIDER.md.
Stage 123P introduced the callback-driven guest liblog.so provider for __android_log_print and __android_log_write, with bounded guest-string copying, an AAPCS32 variadic formatter subset, exact symbol-index binding, and transactional rollback. See docs/STAGE123P_LIBLOG_GUEST_ABI_PROVIDER.md.

## Completed milestone — Stage 123R (guest libz ABI provider)

Stage 123R adds a bounded guest libz provider for the four relocation-backed imports inflate, inflateEnd, inflateInit2_, and inflateInit_. It models the 56-byte ARM32 z_stream, tracks native host streams behind opaque guest tokens, and copies input/output through validated guest memory before calling native zlib. Host regression and Android ARM64 build both passed (runs `43413431936` and `43413417668`). Device-side zlib calls and original-game boot remain unverified. Guest allocator callbacks, native msg pointers, exact original zlib minor-version parity, and production module-loader integration remain out of scope. See docs/STAGE123R_GUEST_LIBZ_ABI_PROVIDER.md.

## Completed milestone — Stage 123Q (Android NDK liblog callback integration)

Stage 123Q connects the Stage 123P virtual liblog.so provider to Android NDK logging. The native probe supplies a __android_log_write callback and exercises both exact guest Thumb import trampolines through synthetic mapped guest memory. Guest pointers are never forwarded as host pointers. Host CI passed in run `43412748589`; Android ARM64 build passed in run `43412881170`. Physical-device logcat validation and production module-loader lifecycle installation remain future work. See docs/STAGE123Q_ANDROID_LIBLOG_NDK_INTEGRATION.md.

## Active milestone update — Stage 123O (original ELF32 DSO dependency evidence)

A fresh readelf inspection of the uploaded APK's `lib/armeabi/libjirobiodg.so` is recorded with both APK/ELF SHA-256 provenance and a machine-readable manifest. It confirms six direct DT_NEEDED entries, 4,057 dynamic relocations (3,920 RELATIVE, 135 JUMP_SLOT, 2 ABS32), and 137 symbol-backed references across 136 unique undefined names. Runtime GLES fallback SONAMEs are recorded separately from DT_NEEDED. This is static evidence only; no provider ABI compatibility or game boot is claimed. See `docs/STAGE123O_ORIGINAL_DSO_DEPENDENCY_EVIDENCE.md` and `reports/STAGE123O_LIBJIROBIODG_PROVIDER_MANIFEST.json`.

## Active milestone update — Stage 123N (guest dl bridge/provider catalog wiring)

Stage 123N adds a small initializer that installs the Stage 123L exact-SONAME provider catalog callbacks into the Stage 123I guest libdl bridge. A host regression exercises opaque guest handles, per-provider symbol scope, Thumb/data address preservation, reference-counted opens/closes, stale-token rejection, and missing-provider behavior. The new source is registered in the host test runner and Android native target. CI validation is pending; this does not yet load Android system DSOs or supply GLES/EGL ARM32 trampolines. See docs/STAGE123N_GUEST_DL_BRIDGE_PROVIDER_WIRING.md.

## Active milestone update — Stage 123M (ELF32 exports into guest dlopen providers)

Stage 123M adds a bounded adapter that converts already-parsed and already-mapped ELF32 module exports into the Stage 123L exact-SONAME guest DSO catalog. It preserves per-module lookup scope, applies load bias only to non-SHN_ABS symbols, and commits the live catalog only after the complete provider set validates. The host regression covers Thumb/data address preservation, provider-isolated dlsym, insufficient scratch capacity, and duplicate SONAME rollback. CI validation is pending. See docs/STAGE123M_ELF32_DLOPEN_PROVIDER_ADAPTER.md.

## Active milestone update — Stage 123K (ELF32 libdl import-index binding)

Stages 122D–122H and 123A–123F have host implementation coverage:
- Transactional multi-ELF image loading, ELF32 dependency/provider/relocation planning, and bounded constructor execution.
- Native XE3U header/fixup parser and bounded LZMA-alone decode adapter.
- Read-only audit of all 36,377 internal relocation entries and a candidate-only C planner.
- Verification of all 230 nonzero external ARM import stubs, identity lookup from guest `r12`, and transactional symbol-specific Thumb import planning/apply for all 238 type-4 cells.
- ARM CPU support for original import-stub writeback, ARM/Thumb PC interworking, and Thumb PUSH/POP.
- End-to-end synthetic execution from the original ARM import stub through the existing Thumb import dispatcher, plus rollback/stale-memory tests for external patch application.

Host CI passed in run `42877554684`; Android ARM64 NDK build/ELF verification passed in run `42877209331`. The original APK and key assets match Stage 97 hashes exactly.

A deeper Thumb audit shows those 22 exports are lazy GL dispatch wrappers, not direct GLES implementations. All share the zero-filled BSS dispatch table at `0x747dc`, have exact name literals and `R_ARM_RELATIVE`-relocated table-base literals, and cache the returned function pointer in their slot after calling the common helper at `0x3d64c`. Helper `0x3d64c` is now proven to call `dlsym` through guest PLT wrapper `0x35a0c`, searching three handles at global structure offsets `+0x34`, `+0x38`, and `+0x3c`. The unlabelled routine at `0x3d818` reads `GLESVersion` (fallback `1`), builds `OGLES`/`OGLES2` keys using `%s_Lib_%d`, and opens them with `dlopen(..., 2)`. Its caller supplies compiled fallback rows: GLES1 `[libGLESv1_CM.so, libhgl.so, null]`, GLES2 `[libGLESv2.so, libEGL.so, null]`. Successful handles are packed into the three global slots in success order and closed/cleared on teardown. The Android override policy/source remains unknown. See [Stage 123H GL wrapper audit](docs/STAGE123H_GL_LAZY_DISPATCH_WRAPPERS.md), [Stage 123G provider coverage](docs/STAGE123G_LIBJIROBIODG_IMPORT_PROVIDER_COVERAGE.md), [Stage 123F external import binding](docs/STAGE123F_TRANSACTIONAL_EXTERNAL_IMPORT_BINDING.md), [Stage 123E thunk execution](docs/STAGE123E_IMPORT_CELL_LOOKUP_AND_THUNK_EXECUTION.md), and [Stage 123D relocation evidence](docs/STAGE123D_INTERNAL_RELOCATION_EVIDENCE.md).

### Active milestone — Stage 123K: bind libdl handlers from original ELF32 symbol indices

The original `libjirobiodg.so` has 137 symbol-based relocation references across 136 unique undefined names. Stage 123J records candidate families only: 101 libc/POSIX/Bionic, 21 ARM EABI compiler-runtime, 4 libdl, 4 libz, 4 C++ ABI/libstdc++, and 2 liblog. These labels are not verified DSO ownership. Host CI passed in run `42885003645` (312 Python tests plus native regressions), and Android ARM64 build passed in run `42884983999`. See `docs/STAGE123J_ELF32_UNDEFINED_SYMBOL_FAMILIES.md`.

1. Stage 123K now derives the four `libdl` import IDs directly from the decoded ELF32 dynamic-symbol table and binds them transactionally. The actual provider callbacks and guest-callable GLES/EGL trampoline registry remain the next integration gate.
2. Add verified `liblog.so` and `libz.so` providers; preserve guest variadic and zlib structure ABIs.
3. Implement the 21 ARM EABI compiler-runtime helpers with differential tests against a known ARM EABI implementation; do not assume they belong to `libm.so`.
4. Audit the 101 libc/POSIX/Bionic candidates for actual use, structure layouts, and API-level availability. Prioritize JNI_OnLoad, file access, threading, and renderer startup.
5. Verify the complete six-library dependency closure and every relocation before reserving a guest address range and loading the ELF32 provider.

### Stage 123K implementation — automatic ELF32 libdl import binding

- Added `re_guest_dl_bridge_bind_elf_symbols()` to identify exactly one undefined, externally visible `dlopen`, `dlsym`, `dlclose`, and `dlerror` symbol in the parsed ELF32 dynamic-symbol array.
- Uses the actual dynamic-symbol indices as import IDs; rejects missing/duplicate names and unsupported symbol types before mutating the import table.
- Synthetic tests cover successful index extraction and transactional missing/duplicate rejection. Validation is pending in CI. See `docs/STAGE123K_ELF32_DL_IMPORT_BINDING.md`.

### Stage 123I foundation — guest dynamic-loader bridge primitive

- Added callback-driven guest `dlopen`/`dlsym`/`dlclose`/`dlerror` handlers, generation-tagged opaque 32-bit guest handles, and a host-only native-handle table.
- Added guest-address-only symbol resolution, transactional mapped-handler registration, and retryable `close_all` shutdown cleanup.
- Added `re_guest_dl_bridge_build_provider_symbols()` to export the four mapped trampolines as `REGUESTSYMBOL` entries for virtual `libdl.so`, directly usable by the single-image ELF32 linked loader's external provider list.
- Host tests passed in run `42885707892`, including provider-symbol resolution; Android ARM64 build including the provider-export addition is running in run `42885707759` (the preceding bridge/cleanup build passed in `42884437640`).
- This is a tested bridge primitive, not yet wired to the actual `libjirobiodg.so` dynamic import indices or real host GLES providers. See `docs/STAGE123I_GUEST_DL_BRIDGE.md`.

### Remaining real-game gates

1. Complete Stage 123J actual ELF32 provider loading and dependency bridging.
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


### Stage 123L — guest DSO provider catalog

- Added a bounded registry for caller-supplied guest DSO export inventories.
- Exact SONAME-only open; no host filesystem search or host `dlopen`.
- Provider-scoped `dlsym` through the existing strong/weak resolver.
- Host-private reference-counted handles; guest results remain 32-bit addresses.
- Added standalone host regression and Android CMake integration.
- Android DSO loading, verified GLES/EGL ARM32 trampolines, and actual game boot remain future work.
- CI validation is pending.
