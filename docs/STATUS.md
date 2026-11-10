# Current Status

**Current implementation milestone: Stage 123G — original ELF32 import-provider coverage audit.** Stage 123F adds a read-only plan for all 238 XE3U type-4 external cells and a rollback-safe apply routine. Bound symbols receive symbol-specific Thumb trampolines; unknown symbols can be routed to the runtime unresolved-import trap. The original shared resolver slot at `0x13B704` is intentionally bypassed by this port strategy. Host CI passed in run `42877554684`, including transactional import planning/apply, direct Thumb dispatch through the original ARM stub, ARM/Thumb interworking, and CPU stack regressions. Android ARM64 NDK build/ELF verification passed in run `42877209331`.

A fresh symbol-table comparison found **22 of the 227 XE3U imports** as defined exports in the exact original ARM32 `libjirobiodg.so`; all 22 exports are Thumb-tagged. This gives us a path to execute those original guest functions through the ELF32 provider loader rather than reimplementing them. The remaining 205 imports are not matched by this one library and still require additional provider/system-library auditing or verified ARM64-side handlers. See [Stage 123G provider coverage](STAGE123G_LIBJIROBIODG_IMPORT_PROVIDER_COVERAGE.md), [Stage 123F import binding](STAGE123F_TRANSACTIONAL_EXTERNAL_IMPORT_BINDING.md), and [Stage 123E thunk execution](STAGE123E_IMPORT_CELL_LOOKUP_AND_THUNK_EXECUTION.md).

**Still unresolved:** actual ELF32 loading/provider validation of `libjirobiodg.so`; the six DT_NEEDED dependencies; production handlers/providers for the remaining imports; allocator callback binding; the two GL cells; internal relocation application and complete XE3U loader integration; physical-device validation. The original game has not been booted or made playable.

**Important target-format correction:** the game's main executable `assets/biohazard.s3e` is an LZMA-alone-compressed XE3U image, not ELF. The Stage 97 evidence identifies `libjirobiodg.so` as a separate ELF32 ARM shared object. Stages 122D–123A build generic ELF32 shared-module infrastructure; they do not load or boot the main XE3U game image. The XE3U path has bounded LZMA-alone decoding plus header/fixup metadata parsing. The uploaded original has now been independently decoded and revalidated against Stage 97. Next, run the native C audit tool against the same asset in a local build environment, then validate the likely internal pointer relocation semantics against code/data disassembly and independently analyze external Thumb thunk semantics before any mutation or guest execution.

**Assets and device validation:** the repository intentionally excludes the original APK and proprietary game assets. `app/src/main/assets/` currently contains only `.gitkeep`; the original APK must be supplied locally and staged with `tools/stage_android_assets.py`. Stage 123D native sources, including the XE3U candidate relocation planner, compiled successfully with the pinned Android NDK in workflow run 42873440349; the ARM64 probe artifact was uploaded. This is a native asset/JNI probe build, not a playable game build. Physical-device validation and original game boot/gameplay remain outstanding.

---

**Current source milestone: Stage 120Z — DT_NEEDED dependency graph resolution implemented. Host source checks passed in GitHub Actions run 42805208188; Android ARM64 native build and ELF verification passed in run 42805208140. The artifact is the native asset/JNI probe; physical-device execution and original game boot/gameplay remain unverified.** Stage 117 integrated a partial ARM32/Thumb guest CPU, AAPCS32 dispatcher, runtime coordinator, and ABI support into the Android ARM64 native target. A prior Android ARM64 workflow passed on commit `b090f90`; this is not validation of the current branch.

Stage 118 adds guest runtime stop diagnostics. Stages 119A–119L introduced guest memory management, checked bus, CPU adapters, checked instruction fetch, CPU data facade, isolated transfer helper, and fault regressions. Stage 119M documented ARMv7-A/R alignment. Stage 119N added memory attributes and strict-by-default SCTLR.A policy; host CI passed on commit `529a7ee`.

**Stage 119O:** added an isolated byte/halfword/word scalar helper using explicit little-endian byte assembly, memory-type checks, and alignment policy. The first host run exposed an incorrect test expectation for an unaligned address while SCTLR.A was clear; the test was corrected. The corrected Stage 119O revision `be24c13` passed host CI (run 42553706025). An Android ARM64 build also passed on the earlier implementation revision, but this does not replace validation of the current Stage 119P branch.

**Stage 119P:** added an isolated A32 scalar instruction helper supporting AL condition, immediate offsets, pre-index/no-writeback, non-PC base/destination registers, LDR/STR byte/word, LDRH/STRH, LDRSB, and LDRSH. Signed loads sign-extend. Added regression tests and Android CMake/host runner registration. This helper is not called by `re_guest_cpu_step`; production CPU behavior is unchanged. Stage 119P host CI and Android ARM64 build passed on commit `52d8271` (runs 42584651928 and 42584651926).

**Still blocked / unverified:** guest SCTLR system-register emulation and reset-value evidence; exact guest address-space/MMU mapping and page semantics; full ARM/Thumb instruction coverage; comprehensive differential testing of new scalar instruction helper; XE3U loader and relocation engine; production file-import activation and exact partial-read/error behavior; complete S3E import table; renderer/audio/input; physical-device execution; original game boot/gameplay. The original APK and game executable are not distributed in the repository.

See [Stage 119P report](STAGE119P_A32_SCALAR_TRANSFER_INTEGRATION.md), [Stage 119O report](STAGE119O_CHECKED_SCALAR_TRANSFER.md), [Stage 119N report](STAGE119N_MEMORY_ATTRIBUTES_ALIGNMENT_POLICY.md), and [roadmap](../ROADMAP.md).


## Stage 119M update

Added ARMv7-A/R alignment research covering instruction-specific alignment behavior, SCTLR.A, memory-type restrictions, and multi-access fault caveats.


## Stage 119N update

Added UNKNOWN, NORMAL, DEVICE, and STRONGLY_ORDERED guest memory attributes, a source-compatible legacy map API defaulting to UNKNOWN, bounded memory-type queries, and strict-by-default SCTLR.A policy. Host regression tests passed on commit `529a7ee`.


## Stage 119O update

Added checked scalar data transfers for byte, halfword, and word widths using explicit little-endian byte assembly. The helper consults memory attributes and alignment policy, checks full range and permissions before access, and preserves outputs/backing bytes on failure. The corrected Stage 119O revision `be24c13` passed host CI. The helper remains opt-in and is not connected to the production interpreter.


## Stage 119P update

Added isolated A32 scalar instruction decoding for ordinary immediate pre-indexed/no-writeback byte/word LDR/STR plus LDRH/STRH/LDRSB/LDRSH. Signed loads sign-extend. The helper routes accesses through Stage 119O, preserves destination registers on failed loads, and fails closed for unsupported forms. Host tests and Android CMake registration added. Current branch CI pending.


## Stage 119Q update

Added a deterministic independent byte-array reference model covering 512 A32 scalar transfer cases across word/byte/halfword and signed loads. The test compares all CPU registers and the complete memory region after every operation. It is registered in the host test runner. Host CI passed on commit `c4325e6` (run 42584673737), including all 75 Python checks, native C syntax checks, Stage 119P integration tests, and all 512 differential cases. Android ARM64 build passed on commit `9cb5aee` (run 42584826800). The model is not an external hardware oracle and the helper remains isolated from production CPU dispatch.


## Stage 119R update

Added directed A32 scalar permission, cross-region, address-wrap/underflow, and unsupported-encoding tests. The tests verify no partial backing-memory mutation and preservation of destination registers on failed loads. Host CI passed on code commit `46a8d8a` (run 42584904344): 75 Python checks, native C syntax checks, Stage 119P integration tests, 512 differential cases, and directed fault/decoder tests all passed.


## Stage 119S update

Audited what the repository proves about the guest ARM profile. Confirmed ARM32/Thumb guest code and AAPCS32 argument convention; exact architecture revision/profile, optional extensions, guest endianness control, privilege/MMU model, and SCTLR reset/write behavior remain unproven. The ARMv7-A/R alignment reference is conditional research, not evidence of the target's exact profile. No guessed system-control state is added.


## Stage 119T update

Added a read-only ELF32/ELF64 metadata auditor and synthetic tests for ELF class, byte order, machine, section bounds, and selected AEABI ARM attributes. The original guest executable is not in the repository, so no game-specific metadata is claimed. Host source checks and Android ARM64 native build passed on commit ffd61f9 (runs 42586297642 and 42586297664). This does not establish game boot or playability.

## Stage 119U update

Expanded public AEABI attribute parsing to file, section, and symbol scopes; implemented Tag_compatibility (ULEB128 flag plus vendor string) and argument-free Tag_nodefaults; retained private vendor payloads as opaque data. Added a readelf/llvm-readobj evidence-capture utility recording tool version and source SHA-256 without executing the input. CI pending.


## Stage 119V update

Added tools/guest_arm_instruction_inventory.py, an optional Capstone-based ARM/Thumb instruction inventory for ELF32 EM_ARM executable sections. Mode is explicit; mixed-mode mapping symbols are not yet parsed. Reports decoded/undecoded byte coverage, mnemonic counts, sample instructions, Capstone version, and source SHA-256. Added mocked decoder tests. CI pending. The original game executable is not present; no game-specific instruction inventory is claimed.


## Stage 119W update

Added mapping-symbol-aware ARM/Thumb inventory. ELF32 local $a/$t/$d symbols (including suffixed forms) split executable sections into ARM code, Thumb code, data, and unknown ranges. ET_REL offsets and ET_EXEC/ET_DYN virtual addresses are handled separately. Unknown prefixes are not guessed; sections without mapping symbols require explicit fallback mode. CI pending. No original game executable is present.


## Stage 119X update

Added tools/guest_arm_relocation_audit.py for read-only ELF32 EM_ARM SHT_REL/SHT_RELA analysis. It resolves symbol names through linked conventional symbol/string tables, validates relocation table links, target sections, symbol indices and offsets, and preserves unknown relocation types. REL addends are not inferred; RELA signed addends are recorded. CI pending. No original game executable is available.


## Stage 119Y update

Added tools/guest_arm_relocation_crosscheck.py to correlate ELF32 ARM relocation sites with local mapping-symbol ranges. Reports ARM/Thumb/data/unknown/non-executable site classes, flags CALL/JUMP sites outside mapped code, and distinguishes undefined/external, absolute, and ordinary section-defined symbols. No relocations are applied and no S3E import identity is inferred. CI pending; no original game executable is available.


## Stage 119Z update

Added tools/guest_arm_function_inventory.py to inventory ELF32 ARM STT_FUNC symbols, classify function extents against mapping-symbol ranges, record Thumb state bits, and join CALL/JUMP relocation references by exact symbol-table and symbol indices. Relocation reports now expose symbol-table section identity. Synthetic tests cover mixed-mode extents, data, zero-size, out-of-section, undefined, and malformed links. CI pending; original game executable not available.


## Stage 120A update

Added tools/guest_arm_dynamic_import_inventory.py and synthetic tests for ELF32 EM_ARM dynamic symbol tables, named undefined symbols, DT_NEEDED dependencies, SHT_DYNAMIC tags, and dynamic REL/RELA correlation. Unknown tags are preserved. PT_DYNAMIC-only files, Android APS2, RELR, and symbol version decoding are deferred. No game binary was analyzed; CI pending.


## Stage 120B update

Added tools/guest_arm_plt_got_audit.py to identify section-backed R_ARM_JUMP_SLOT records and correlate them with dynamic symbol identity. PLT tag values are preserved when available; absent values remain null. Synthetic tests cover symbol correlation and unknown/empty PLT inventories. No PLT instruction decoding, GOT resolution, or relocation application is claimed. CI pending.


## Stage 120C update

Added tools/guest_arm_pt_dynamic_audit.py for ELF32 EM_ARM program-header parsing without requiring section headers, PT_LOAD file-backed virtual address mapping, and PT_DYNAMIC/DT_NEEDED decoding. Synthetic tests cover stripped-style inputs, BSS/file bounds, missing DT_NULL, invalid segment sizes, and both endian modes. CI pending; no original game executable was analyzed.


## Stage 120D update

Added tools/guest_arm_dynamic_relocation_segment_audit.py to parse ELF32 EM_ARM DT_REL, DT_RELA, and DT_JMPREL through PT_DYNAMIC/PT_LOAD without section headers. Symbol names are correlated only when SysV DT_HASH safely bounds the dynamic symbol table; otherwise identity remains unresolved. Tests and CI pending. No relocation is applied.
