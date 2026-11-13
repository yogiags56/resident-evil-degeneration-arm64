# Current Status

**Current implementation milestone: Stage 124A — guest libc bounded scanf parser.** Stage 123F adds a read-only plan for all 238 XE3U type-4 external cells and a rollback-safe apply routine. Bound symbols receive symbol-specific Thumb trampolines; unknown symbols can be routed to the runtime unresolved-import trap. The original shared resolver slot at `0x13B704` is intentionally bypassed by this port strategy. Host CI passed in run `42877554684`, including transactional import planning/apply, direct Thumb dispatch through the original ARM stub, ARM/Thumb interworking, and CPU stack regressions. Android ARM64 NDK build/ELF verification passed in run `42877209331`.

Stage 123L adds a bounded guest DSO provider catalog for the Stage 123I/123K dynamic-loader bridge. It validates unique basename-only SONAMEs, resolves symbols only within the opened provider, tracks open references, and returns caller-supplied guest addresses without exposing host pointers. The standalone regression is wired into host tests and the Android CMake target. CI validation is pending; earlier failed GitHub Actions attempts did not expose downloadable logs or artifacts, so their failure cannot yet be attributed to source code. See [Stage 123L](STAGE123L_GUEST_DSO_PROVIDER_REGISTRY.md).

The original ELF32 audit found 22 of 227 XE3U names as defined exports in `libjirobiodg.so`, but a deeper Thumb audit shows these are **lazy GL dispatch wrappers**, not direct GLES implementations. All 22 wrappers share BSS table base `0x747dc`, read and cache a function pointer in their own BSS slot, and call helper `0x3d64c` with their GL name when the slot is empty. All 22 table-base literals have `R_ARM_RELATIVE`; all 22 names match; all 22 cache into zero-fill BSS. Helper `0x3d64c` is now confirmed as a three-handle `dlsym` search: it tries guest handles at global structure offsets `+0x34`, `+0x38`, and `+0x3c`, then caches a resolved Thumb pointer in the wrapper's BSS slot. The ELF contains `dlopen`, `dlsym`, `dlclose`, and `dlerror` wrappers. The unlabelled routine at `0x3d818` reads `GLESVersion` (fallback `1`), builds `OGLES`/`OGLES2` keys for ordinals 1–3, and opens the configured paths with `dlopen(..., 2)`. Its caller at `0x505e0` supplies compiled fallback rows: GLES1 `[libGLESv1_CM.so, libhgl.so, null]`, GLES2 `[libGLESv2.so, libEGL.so, null]`. Successful handles are packed into `0x737f0+0x34/+0x38/+0x3c` in success order and closed/cleared on teardown. The actual Android override policy and source of `GLESVersion` remain unresolved; the APK's `s3e.icf` has only WinMobile examples for those keys. See the expanded [Stage 123H GL wrapper audit](STAGE123H_GL_LAZY_DISPATCH_WRAPPERS.md) and [Stage 123G provider coverage](STAGE123G_LIBJIROBIODG_IMPORT_PROVIDER_COVERAGE.md).

Stage 123I adds `re_guest_dl_bridge.h/.c`: callback-driven `dlopen`/`dlsym`/`dlclose`/`dlerror` mapped handlers, up to eight generation-tagged opaque 32-bit guest handles, host-only `uintptr_t` handles, and a resolver contract that returns guest-callable 32-bit addresses rather than host pointers. The bridge binds transactionally into the existing import table and is included in the Android CMake target. It now also exports its four guest trampolines as `REGUESTSYMBOL` entries tagged `libdl.so`, directly consumable by `re_guest_elf32_load_linked_image()`'s provider list. Synthetic host runtime tests passed in run `42885707892` (312 Python tests plus native regressions, including `Guest dlopen/dlsym/dlclose/dlerror bridge tests: PASS`), covering eight-handle capacity, retryable close-all, stale tokens, and symbol resolution through the generated libdl provider. Android ARM64 build including provider export is running in run `42885707759`; the preceding close-all bridge build passed in `42884437640`. Stage 123H also traced `JNI_OnLoad` at `0x34056` directly to the GL initializer at `0x3d3b0`; the bridge/provider registry must be installed before invoking guest JNI_OnLoad. See [Stage 123I guest dynamic-loader bridge](STAGE123I_GUEST_DL_BRIDGE.md) and [Stage 123H GL wrapper audit](STAGE123H_GL_LAZY_DISPATCH_WRAPPERS.md).

Stage 123J adds name-based candidate-family triage to `tools/xe3u_elf32_provider_audit.py` and records the full original ELF32 import inventory in [Stage 123J undefined-symbol manifest](STAGE123J_ELF32_UNDEFINED_SYMBOL_FAMILIES.md). The exact binary has 137 symbol-based relocation references across 136 unique non-empty undefined names: 101 libc/POSIX/Bionic candidates, 21 ARM EABI compiler-runtime candidates, 4 libdl, 4 libz, 4 C++ ABI/libstdc++, and 2 liblog. These are triage labels only, not verified DSO ownership. Host CI passed in run `42885003645`; Android ARM64 build for the same revision passed in run `42884983999`.

**Still unresolved:** actual ELF32 loading/provider validation of `libjirobiodg.so`; wiring Stage 123I callbacks to the exact ELF import symbols; host provider mappings for `libGLESv1_CM.so`, `libhgl.so`, `libGLESv2.so`, and `libEGL.so`; the six `DT_NEEDED` dependencies and 137 symbol-based relocation references across 136 unique undefined names; Android config override/source for `GLESVersion` and `OGLES*_Lib_N`; production handlers/providers for remaining XE3U imports; allocator callback binding; the two zero-valued GL cells; internal relocation application and complete XE3U loader integration; physical-device validation. The original game has not been booted or made playable.

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


## Stage 123Q — Android NDK liblog callback integration

The native Android probe now supplies a callback backed by NDK
`__android_log_write` to the Stage 123P guest `liblog.so` provider. Its
synthetic guest runtime path dispatches both exact relocation-indexed
`__android_log_write` and `__android_log_print` trampolines and checks return
flow. Guest strings are copied through the permission-checked memory bus;
guest pointers are never forwarded as host pointers. The host mock Android
logging header keeps source syntax checks portable. Host suite and Android
ARM64 CI validation are pending for this branch. Physical-device logcat
verification, installation into the real module-load lifecycle, the remaining
Bionic dependency providers, and original game boot remain outstanding. See
[Stage 123Q](STAGE123Q_ANDROID_LIBLOG_NDK_INTEGRATION.md).


## Stage 123Q validation update — 2026-11-13

The host validation pipeline now passes on the Stage 123Q branch: 312 Python
tests, native C translation-unit mock-header syntax checks, and the full
compiled host-test executable suite, including the guest liblog provider
regression. The Android ARM64 NDK build has been triggered for this stage and
is still running. No Android APK build or device logcat result is claimed yet.


## Stage 123R — guest libz ABI provider

Added the bounded guest provider for the four relocation-backed libz imports.
The adapter tracks host zlib streams behind guest-only opaque tokens, uses the
56-byte ARM32 z_stream layout, and copies buffers through validated guest
memory. The host regression passed, including decompression output and guest
stream bookkeeping. Android ARM64 build validation is running. Exact original
SDK layout/version parity, guest custom allocator callbacks, and production
module-loader integration remain open. See
[Stage 123R](STAGE123R_GUEST_LIBZ_ABI_PROVIDER.md).


## Stage 123R validation update — 2026-11-13

Host CI passed in run `43413431936`: 312 Python tests, native C
mock-header syntax checks, and all compiled host regressions, including
`Guest libz provider tests: PASS`. Android ARM64 NDK build and ELF
verification passed in run `43413417668`, including native zlib linkage and
artifact upload. The provider has not yet been exercised by the original
game on a physical device. The original game's full DSO dependency closure
and boot remain unverified.


## Stage 123S — guest C++ new/delete heap provider

Added exact ELF32 symbol-index bindings for `_Znwj`, `_Znaj`, `_ZdlPv`,
and `_ZdaPv`. The provider allocates 8-byte-aligned blocks from a dedicated
guest RW arena using bounded host-side metadata; returned values are guest
addresses, never host pointers. The host regression passed, including
allocation, non-overlap, free/reuse, and out-of-space behavior. Android ARM64
build and ELF verification also passed in run `43413952565`. C++ exception semantics, C malloc/free, and
production heap lifecycle integration remain open. See
[Stage 123S](STAGE123S_GUEST_CXX_HEAP_PROVIDER.md).


## Stage 123T — ARM EABI compiler-helper provider

Added a bounded guest provider for the 21 relocation-backed `__aeabi_*`
symbols in the original ELF32 import manifest. It implements the audited
base-standard/core-register calling convention for double/float arithmetic,
comparisons, conversions, signed/unsigned 32-bit division and divmod, 64-bit
multiplication, and 64-bit signed divmod. The shared import result/dispatcher
now supports r0-r3 return values; this is necessary because RTABI32
`__aeabi_ldivmod` returns quotient in r0/r1 and remainder in r2/r3. The
provider leaves SONAME/version-library identity unspecified rather than
guessing `libgcc.so`. Host CI passed in run `43415885622` with 312 Python
tests and all native regressions, including the new EABI provider test.
Android ARM64 build and ELF verification passed in run `43415890966`,
including artifact upload.
Invalid float-to-int operands use a deterministic saturating policy; exact
original-runtime behavior remains unverified. See
[Stage 123T](STAGE123T_ARM_EABI_COMPILER_HELPERS.md).


## Stage 123U — Shared guest heap and libc allocation provider

Introduced a bounded shared guest heap and moved the Stage 123S C++ new/delete
provider onto it. Added exact ELF32 symbol-index adapters for libc
`malloc`, `free`, `realloc`, and `valloc` from the direct `libc.so`
dependency. Both families now use the same host-side allocation records and
guest RW arena; cross-family free/reuse is tested. The allocator supports 512
records, 8-byte normal alignment, page-aligned valloc, safe realloc data
preservation, and no host-pointer exposure. Host CI passed in run
`43416640694`; Android ARM64 build and ELF verification passed in run
`43416621992`, including artifact upload. Production heap reservation and
lifetime integration remain open. See
[Stage 123U](STAGE123U_SHARED_GUEST_HEAP_LIBC_ALLOC.md).


## Stage 123W — Guest libc formatted output

Added bounded guest `libc.so` adapters for the exact relocation-backed
`snprintf` and `sprintf` imports. The formatter reads guest strings through
the checked memory bus, follows AAPCS32 variadic register/stack ordering,
aligns 64-bit arguments to even word indices, and uses a fixed 2 KiB scratch
buffer. `snprintf` preserves standard truncation and full-length return
semantics; unsupported conversions, invalid pointers, unterminated inputs,
and output overflow fail closed. The provider is included in the Android
ARM64 CMake target and host regression runner. Tests cover register/stack
arguments, truncation, zero-size output, 64-bit alignment, dynamic width,
and missing/duplicate ELF symbol handling. Host CI passed in run `43418862945`, including the formatted-output
provider and shared liblog regressions. Android ARM64 build and ELF
verification passed in run `43418672203`. Production module-loader
installation and original-game boot remain outstanding. See
[Stage 123W](STAGE123W_GUEST_LIBC_FORMATTED_OUTPUT.md).


## Stage 123V — Guest libc memory and string provider

Added 15 exact relocation-backed `libc.so` adapters covering bulk
`memcpy/memmove/memset`, string length/comparison/copy/concatenation/search,
`atoi`, and stateful `strtok`. Bulk operations validate guest permissions
and contiguous mappings; byte-oriented operations use the guest memory bus
with checked 32-bit addresses and a 64 KiB scan cap. ASCII case folding,
bounded integer parsing, and one continuation slot per provider define the
compatibility policy. Host CI passed in run `43417968172` with 312 Python
tests and all native regressions, including the new libc string provider
test. Android ARM64 build and ELF verification passed in run
`43417396929`. The production module loader does not yet install this
provider. See [Stage 123V](STAGE123V_GUEST_LIBC_MEMORY_STRING.md).


## Stage 123X — Guest libc printf output sink

Extended the Stage 123W formatted-output provider with the exact
relocation-backed `libc.so` symbol `printf`. The guest format pointer is
read from r0 and variadic words begin at AAPCS32 index 1. Formatted bytes are
sent through a caller-supplied output callback; short writes and missing
sinks fail closed. This avoids exposing host stdio or native callback
pointers to the ARM32 guest. The host regression now checks exact output
bytes and the return count. Host regression CI passed in run `43419274585`. Android ARM64 build and ELF verification passed in run `43419241484`, including artifact upload.
Production module-loader installation and a real Android stdout/log sink
remain open. See
[Stage 123X](STAGE123X_GUEST_LIBC_PRINTF_OUTPUT_SINK.md).


## Stage 123Y — Guest libc stdio and file I/O foundation

Added a guest-owned opaque FILE-token provider for seven relocation-backed
`libc.so` imports: `fopen`, `fclose`, `fread`, `fwrite`, `fseek`,
`ftell`, and `fgets`. A bounded 32-slot stream table maps guest tokens
to backend-owned opaque streams; host FILE pointers are never exposed.
Guest path normalization rejects absolute paths and parent traversal.
The backend is an explicit read/write/seek/tell/close vtable. A host
regression exercises the provider through the guest import dispatcher and
is registered in the host test runner; the provider is included in the
Android ARM64 CMake target. Host CI passed in run `43420175068`. Android ARM64 app build, ELF
verification, artifact collection, and upload passed in run `43420138210`. Production module-loader wiring, a writable Android backend,
`fprintf`, and original-game file access remain outstanding. See
[Stage 123Y](STAGE123Y_GUEST_LIBC_STDIO_FILE_IO.md).


## Stage 123Z — Guest libc formatted file output and stream status

Extended the guest stdio provider from seven to eleven exact ELF imports.
Added `fprintf` through the bounded shared AAPCS32 formatter, plus
`feof`, `ferror`, and `fflush`. Stream state tracks EOF and errors;
successful seek clears EOF, backend failures set error, and writable close
flushes when the backend supports it. The host regression now exercises
formatted file output and stream state transitions through the real guest
import dispatcher. Host and Android ARM64 CI for this revision are pending.
`fscanf` and `sscanf` are explicitly deferred for a dedicated bounded
scanf parser. See [Stage 123Z](STAGE123Z_GUEST_LIBC_FORMATTED_FILE_IO.md).


## Stage 124A — Guest libc bounded scanf parser

Added a standalone bounded parser for integer, floating-point, string,
character, scanset, suppression, width, and `%n` conversions. The stdio
provider now binds `sscanf` and seek-restored `fscanf` with ARM32 guest
destination validation. A dedicated parser regression and dispatcher
coverage are included. CI validation is pending. See
[Stage 124A](STAGE124A_GUEST_LIBC_SCANF_PARSER.md).
