# Stage 123G — XE3U import provider coverage in libjirobiodg.so

## Input provenance
The user-supplied APK and `lib/armeabi/libjirobiodg.so` match Stage 97 SHA-256 hashes exactly. The shared object is ELF32 little-endian ARM EABI5 (431,528 bytes); it is not an ARM64 library.

## Complete XE3U import manifest

The read-only `tools/s3e_binary_audit.py` now emits `import_manifest` in its JSON report. Each of the 227 symbols has its exact symbol index, external relocation count, and every cell's offset, original value, type, classification, and verified ARM stub offset where applicable. It rejects external cell offsets that are unaligned or outside the file-backed code blob.

Re-running the manifest logic against the re-uploaded original image yields:
- 227 / 227 symbols represented.
- 238 / 238 external records classified.
- 230 verified three-instruction ARM import stubs.
- 8 zero-valued function-pointer data cells.
- No unverified resolver placeholders or other cell forms.

The eight data cells map to five symbol indices: `glTexParameteri` (1), `glTexParameterx` (1), `s3eMallocBase` (2), `s3eReallocBase` (2), and `s3eFreeBase` (2). This is a reproducible metadata inventory, not proof that those APIs have production handlers.

## Exact symbol intersection

The XE3U image contains 227 API symbol names. The original shared object has 56 defined global dynamic exports. An exact-name comparison finds 22 symbols in common (9.7% of the XE3U symbol table). All 22 export values are Thumb-tagged (bit 0 set). A later Stage 123H disassembly audit establishes that these are lazy GL dispatch wrappers, not direct GLES implementations; see [Stage 123H](STAGE123H_GL_LAZY_DISPATCH_WRAPPERS.md).

| Symbol | ELF32 st_value |
|---|---:|
| `glTexParameterx` | `0x0004AB05` |
| `glLoadMatrixx` | `0x0004A7BD` |
| `glDisable` | `0x0004AA45` |
| `glTexImage2D` | `0x0004A765` |
| `glPixelStorei` | `0x0004AB35` |
| `glActiveTexture` | `0x0004AAB5` |
| `glDepthMask` | `0x0004A935` |
| `glDisableClientState` | `0x0004A8A5` |
| `glEnableClientState` | `0x0004A8CD` |
| `glEnable` | `0x0004AA6D` |
| `glDrawElements` | `0x0004A7E5` |
| `glVertexPointer` | `0x0004A825` |
| `glOrthox` | `0x0004A985` |
| `glBindTexture` | `0x0004AB61` |
| `glLoadIdentity` | `0x0004A9D1` |
| `glDeleteTextures` | `0x0004AA19` |
| `glColor4x` | `0x0004A8F5` |
| `glMatrixMode` | `0x0004A9F1` |
| `glGenTextures` | `0x0004AB8D` |
| `glShadeModel` | `0x0004A95D` |
| `glTexCoordPointer` | `0x0004A865` |
| `glClientActiveTexture` | `0x0004AADD` |

These are link-time ELF symbol values, not runtime guest addresses. A loaded provider address must be calculated from the actual ELF32 load bias and symbol semantics, not hardcoded from this table.

## Important negative matches

- `glTexParameteri` is not a defined export in this shared object; it needs a separate provider (likely the GLES system interface, subject to ABI verification).
- The four guest file APIs `s3eFileOpen`, `s3eFileRead`, `s3eFileGetChar`, and `s3eFileClose` are not exports here; retain the independently audited backend-backed file bridge.
- The allocator data symbols `s3eMallocBase`, `s3eReallocBase`, and `s3eFreeBase` are not exports; bind them through explicit allocator callbacks.
- The other 205 XE3U symbols do not match a defined export in this one shared object. This does not prove they are absent from the Android system libraries or other S3E runtime providers.

## Export wrappers use a shared lazy dlsym dispatch path

Stage 123H disassembled all 22 matching Thumb exports and found they share a BSS function-pointer table at `0x747dc`. Each wrapper reads its own slot, passes the exact GL name to shared helper `0x3d64c` when the slot is empty, stores the returned pointer back into that slot, and calls it. All 22 table-base literals have `R_ARM_RELATIVE`; all 22 names match; all 22 slots are in the ELF zero-fill tail.

The helper `0x3d64c` searches up to three dynamic-loader handles at global structure `0x737f0` offsets `+0x34`, `+0x38`, and `+0x3c`, using `dlsym` through PLT `0xa240`. The unlabelled initialization routine at `0x3d818` reads `GLESVersion`, builds `OGLES`/`OGLES2` configuration keys, and loads the configured libraries with `dlopen`; compiled fallback rows are GLES1 `[libGLESv1_CM.so, libhgl.so, null]` and GLES2 `[libGLESv2.so, libEGL.so, null]`. Handles are stored in success order and closed/cleared on teardown.

Consequently, the 22 exports can only serve as guest wrappers after the ELF32 module is mapped, relocated, and its loader-handle initialization path succeeds. They are not final host GLES implementations and must not be bound directly as such. The port needs a guest `dlopen/dlsym/dlclose/dlerror` bridge that maps 32-bit guest handle tokens to host-side handles and returns guest-callable Thumb trampoline addresses; never store 64-bit host pointers in guest memory. The exact Android config override source and callback/handle lifetime remain open. See [Stage 123H GL lazy dispatch audit](STAGE123H_GL_LAZY_DISPATCH_WRAPPERS.md).

The repository already has verified host GLES handlers for `glTexParameteri` (symbol index 0) and `glTexParameterx` (symbol index 1). Stage 123G adds `re_guest_gl_register_verified_mapped`, which registers these handlers for the mapped runtime without patching individual slots; the Stage 123F external import planner can then patch all four corresponding XE3U cells transactionally. This is distinct from the provider's own BSS dispatch table.

## ELF dependency boundary

The shared object declares these DT_NEEDED libraries:
- `libc.so`
- `libm.so`
- `liblog.so`
- `libstdc++.so`
- `libz.so`
- `libdl.so`

The generic ELF32 dependency/provider pipeline can be used to load and inspect this guest ARM32 module, but these Android system dependencies are not supplied as ARM32 binaries in the APK. Their required imports must be audited and bridged to verified ARM64-side handlers or other guest providers. Do not attempt to load this ARM32 ELF directly into the ARM64 process linker.

## Next

1. Run `tools/xe3u_elf32_provider_audit.py` against the exact original APK locally and compare its ELF metadata with GNU `readelf`; its new wrapper decoder is covered by synthetic host tests but has not yet been run against the proprietary APK in CI.
2. Reconstruct Android config overrides and implement a guest-handle `dlopen/dlsym/dlclose/dlerror` bridge that returns only guest-callable 32-bit Thumb trampoline addresses.
3. Audit all six DT_NEEDED dependencies and 137 relocation-referenced undefined ELF names; prepare a complete read-only ELF32 relocation/provider plan.
4. Only after guest address reservation and dependency preflight, load `libjirobiodg.so` into guest memory and initialize its loader handles.
5. Keep all remaining XE3U imports and data cells fail-closed until their providers/semantics are verified.

## Safety boundary
This is a symbol-table comparison only. No ELF module was loaded or executed, no import cell was patched, and no Android system library ABI was assumed.
