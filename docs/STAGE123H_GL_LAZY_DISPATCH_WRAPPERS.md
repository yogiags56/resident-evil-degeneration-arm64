# Stage 123H — Original ELF32 GL wrapper dispatch-table audit

## Provenance and scope

This is a read-only audit of the user-supplied original APK, independently re-opened in the current conversation. The embedded `lib/armeabi/libjirobiodg.so` is 431,528 bytes and SHA-256:

`ce6c9f6f7339f226c1f060744ddb89d7b187d01a8b20571cd417896bf4c4fcf0`

The APK and this ELF asset match the Stage 97 recorded provenance. No proprietary binary is copied into this repository.

The cross-audit script is `tools/xe3u_elf32_provider_audit.py`. It now decodes a small, explicitly bounded subset of Thumb instructions to classify candidate GL exports, resolve PC-relative literals, identify BSS dispatch slots, and cross-check the literal pointers against `R_ARM_RELATIVE` records. Synthetic decoder tests are in `tests/test_xe3u_elf32_provider_audit.py`.

## ELF32 provider facts

- ELF32, little-endian, `ET_DYN`, `EM_ARM`, EABI flags `0x05000002`.
- File size: 431,528 bytes.
- Dynamic symbol table: 207 entries.
- Defined externally visible export candidates: 56.
- Named undefined dynamic symbols: 146.
- Unique undefined names referenced by dynamic relocations: 137.
- Dynamic relocations: 4,057 total — 3,920 `R_ARM_RELATIVE`, 2 `R_ARM_ABS32`, 135 `R_ARM_JUMP_SLOT`.
- Six `DT_NEEDED` dependencies: `libc.so`, `libm.so`, `liblog.so`, `libstdc++.so`, `libz.so`, and `libdl.so`.
- No `DT_SONAME` is present; the root module must use the caller-supplied inventory identity.
- No `DT_INIT` or `DT_INIT_ARRAY` constructor entry is present. The ELF does export `JNI_OnLoad` at Thumb address `0x000238ed` and `JNI_OnUnload` at `0x000233bd`; JNI lifecycle integration is separate from ELF constructors.

PT_LOAD layout:
- RX: virtual address `0x00000000`, file size/memory size `0x667a0`, alignment `0x8000`.
- RW: virtual address `0x0006f000`, file size `0x310`, memory size `0x5c80`, alignment `0x8000`.
- Zero-fill tail/BSS: `[0x6f310, 0x74c80)`.

The existing ELF32 batch loader can represent this layout, but it requires a caller-selected load bias aligned to `0x8000`, caller-owned segment buffers, all relocations pre-decoded/resolved, and all reachable DT_NEEDED modules supplied. It does not itself supply the six Android system libraries or execute JNI_OnLoad.

## Correction: the 22 matching GL exports are lazy dispatch wrappers

The previous exact-name intersection found 22 of the 227 XE3U import names in the ELF32 dynamic exports. They are not 22 direct GLES implementations. All 22 are Thumb wrapper functions that share a dispatch table and call the same internal helper when a function pointer slot is empty.

For every one of the 22 wrappers, the original bytes prove:
1. A Thumb `LDR (literal)` loads the same dispatch-table base, `0x000747dc`.
2. A Thumb word `LDR` reads that wrapper's function-pointer slot.
3. A literal in the wrapper resolves to the exact GL symbol name.
4. A Thumb `BL` calls the same internal helper at `0x0003d64c`.
5. A Thumb `STR` stores the helper's returned pointer into the same dispatch-table slot.
6. The table-base literal is covered by an `R_ARM_RELATIVE` relocation.

Aggregate verification: 22/22 wrappers have the same table base, 22/22 name literals match their export names, 22/22 table-base literals have `R_ARM_RELATIVE`, 22/22 cache the returned pointer in their own slot, and 22/22 slots lie in the PT_LOAD zero-fill tail.

The table base and slots are therefore zero after normal ELF BSS materialization. The wrappers lazily populate them through the shared helper; the actual callback source and complete resolver ABI still need reconstruction. Disassembly of helper `0x3d64c` shows it consults a global callback structure based at `0x000737f0`, including pointer fields at offsets `+0x34`, `+0x38`, and `+0x3c`. Their roles and initialization path are not yet assigned.

### Resolver helper is a three-handle dlsym search

Further disassembly resolves the key unknown in the preceding paragraph:
- Helper `0x3d64c` reads the pointer fields at global structure offsets `+0x34`, `+0x38`, and `+0x3c` in sequence.
- For each non-null field, it calls `0x35a0c` with the handle in `r0` and the GL name in `r1`.
- `0x35a0c` forwards to `0x35a54`; the latter calls ELF PLT entry `0x0000a240`, independently identified from `.rel.plt` as `dlsym`.
- A non-null result is passed with the name to helper `0x3d4f4`, then returned to the wrapper for caching in the BSS slot.
- The same library contains a path/handle wrapper at `0x35a7c` that calls PLT `0x0000a198` (`dlopen`) with flags value `2`. Separate wrappers call PLT `dlclose` at `0x0000a078` and `dlerror` at `0x00009ebc`.

This confirms that the GL wrapper resolver is a guest-side search across up to three dynamic-loader handles, not a hardcoded GLES function table. The names and creation/ownership of the three handles and the initialization order of the global structure remain open. A port must bridge guest `dlopen/dlsym/dlclose/dlerror` semantics; it must not place 64-bit host pointers directly into this 32-bit guest structure. The bridge should translate guest handle tokens to host handles and return guest-callable 32-bit Thumb trampolines for resolved functions.

### Verified dispatch slots

All addresses below are link-time guest virtual addresses; apply the eventual ELF load bias when materializing the module.

| GL export | BSS dispatch slot | Table offset |
|---|---:|---:|
| glTexImage2D | `0x747dc` | `0x00` |
| glLoadMatrixx | `0x747e0` | `0x04` |
| glDrawElements | `0x747e4` | `0x08` |
| glVertexPointer | `0x747e8` | `0x0c` |
| glTexCoordPointer | `0x747ec` | `0x10` |
| glDisableClientState | `0x747f0` | `0x14` |
| glEnableClientState | `0x747f4` | `0x18` |
| glColor4x | `0x747f8` | `0x1c` |
| glDepthMask | `0x747fc` | `0x20` |
| glShadeModel | `0x74800` | `0x24` |
| glOrthox | `0x74804` | `0x28` |
| glLoadIdentity | `0x74808` | `0x2c` |
| glMatrixMode | `0x7480c` | `0x30` |
| glDeleteTextures | `0x74810` | `0x34` |
| glDisable | `0x74814` | `0x38` |
| glEnable | `0x74818` | `0x3c` |
| unclaimed slot | `0x7481c` | `0x40` |
| glActiveTexture | `0x74820` | `0x44` |
| glClientActiveTexture | `0x74824` | `0x48` |
| glTexParameterx | `0x74828` | `0x4c` |
| glPixelStorei | `0x7482c` | `0x50` |
| glBindTexture | `0x74830` | `0x54` |
| glGenTextures | `0x74834` | `0x58` |

The unclaimed `0x40` slot is not assigned to `glTexParameteri` merely because that symbol is another XE3U import; the original export/table evidence does not prove that association. The two zero-valued GL cells in the XE3U executable also remain separate from this ELF BSS dispatch table and must not be patched by guesswork.


### Handle initialization routine identified

A further read-only trace found the unlabelled Thumb routine at `0x3d820` that populates and tears down these handles:
- It reads the `GLESVersion` setting using the literal string at `0x613f8`.
- It uses format string `%s_Lib_%d` at `0x61414` with library-family strings `OGLES` at `0x61404` and `OGLES2` at `0x6140c`.
- It calls the wrapper at `0x35a14`, which reaches `dlopen` PLT `0xa198` with flags value `2`.
- Successful handles are stored into the global structure at offsets `+0x34`, `+0x38`, and `+0x3c`; the same routine later calls the `dlclose` wrapper at `0x35a04` and clears those fields during teardown.
- It checks symbol availability using the resolver helper for `glVertexPointer` and `glVertexAttribPointer` before completing initialization.

The routine constructs family/ordinal names using `OGLES` or `OGLES2` and `%s_Lib_%d`; exact branch-dependent name-to-slot assignment and the source of the `GLESVersion` setting still need to be transcribed and tested. The library names are not assumed to be Android system sonames. The likely bridge seam is now much clearer: reproduce the observed family/ordinal lookup and handle lifecycle using 32-bit guest handle tokens, with each `dlsym` result mapped to a guest-callable Thumb trampoline. Do not store 64-bit host handles or function pointers in guest memory.

## Runtime implications

- The 22 ELF exports are callable wrappers only after the ELF32 module is mapped, its relocations are applied, and the shared resolver callback state is initialized.
- Do not bind the 22 export addresses directly as final host GLES function implementations.
- A viable bridge likely needs to initialize the provider's resolver callback fields so the helper can obtain ARM32-callable Thumb function pointers for GLES entry points. The precise callback contract must be recovered first.
- On ARM64 Android, do not pass host 64-bit function pointers into 32-bit guest slots. Guest pointers must refer to 32-bit guest trampolines or guest-callable provider code.
- The six DT_NEEDED libraries and 137 relocation-referenced undefined names remain a separate dependency/bridge task. No ARM32 system library may be handed to the ARM64 process linker.

## Next work

1. Recover the callback structure at `0x737f0`: identify its owner, initialization site, callback prototypes, and lock/lifetime requirements.
2. Trace JNI_OnLoad and the Java/native startup path to establish whether it initializes the callback structure or registers native methods only.
3. Build a read-only relocation/provider plan for the original ELF using the actual module bytes and the existing ELF32 pipeline; select no load bias until the guest address map is reserved.
4. Classify the 137 undefined relocation-referenced symbols against the six DT_NEEDED libraries and required bridge coverage.
5. Only after those checks, load the original provider into guest memory and test one harmless wrapper through a verified resolver callback.

## Safety boundary

This stage only analyzes bytes and instruction encodings. It does not load the original ELF, apply relocations, invoke JNI_OnLoad, call GLES, modify the APK, or execute game code. The original game is still not booted or playable.
