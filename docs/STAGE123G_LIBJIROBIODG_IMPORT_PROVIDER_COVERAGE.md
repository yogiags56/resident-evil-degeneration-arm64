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

The XE3U image contains 227 API symbol names. The original shared object has 56 defined global dynamic exports. An exact-name comparison finds 22 symbols in common (9.7% of the XE3U symbol table). All 22 export values are Thumb-tagged (bit 0 set), which is compatible with the type-4 Thumb import-cell model and the new direct Thumb-trampoline/guest-provider binding path.

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

## Export wrappers are not yet validated direct providers

Disassembly shows that the matching GL exports are not necessarily simple direct calls into GLES:
- `glTexParameterx` reads a function pointer from a shared table at offset `+0x4C`; when it is zero, the wrapper calls `s3eFibreExit_asm+0x6C44` to initialize/obtain the pointer, then invokes it with `BLX r3`.
- `glDisable` uses a different table slot (`+0x38`) and the same lazy initialization helper.
- `glLoadMatrixx` follows the same function-pointer-table pattern.

Therefore the 22 exact symbol matches are **candidate guest providers**, not yet proven callable by simply adding the ELF load bias. The global function-pointer table, `s3eFibreExit_asm` behavior, initialization order, and required runtime stack/fibre state must be reconstructed before these exports are used as direct providers. A host GLES wrapper may ultimately be simpler, but it must preserve the fixed-point/API semantics and runtime context expected by the guest.

The library imports `dlopen`, `dlsym`, `dlclose`, and `dlerror`, which is consistent with dynamic provider initialization; no assumption is made about which library or function each lookup resolves to.

The repository already has verified host GLES handlers for `glTexParameteri` (symbol index 0) and `glTexParameterx` (symbol index 1). Stage 123G adds `re_guest_gl_register_verified_mapped`, which registers these handlers for the mapped runtime without patching individual slots; the Stage 123F external import planner can then patch all four corresponding cells (two data cells and two ARM stubs) transactionally. This does not yet bind the other 20 GL exports, nor does it prove the original library's lazy function-pointer table is initialized.

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

1. Run the repository's ELF32 auditor/provider pipeline against the exact original `libjirobiodg.so` and compare it with GNU `readelf`.
2. Load the shared object into guest memory with a verified load bias and build a provider catalog from its dynamic symbols.
3. Route the 22 matching XE3U imports to those guest function addresses, preserving Thumb bit 0.
4. Audit the six DT_NEEDED dependencies and the remaining 205 S3E imports; bridge only verified APIs.
5. Keep the two zero GL cells and six allocator data cells fail-closed until their providers are confirmed.

## Safety boundary
This is a symbol-table comparison only. No ELF module was loaded or executed, no import cell was patched, and no Android system library ABI was assumed.
