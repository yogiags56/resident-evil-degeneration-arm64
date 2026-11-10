# Stage 123O — Original ELF32 DSO dependency evidence

## Scope and provenance

This stage records a fresh, reproducible static inspection of the uploaded APK entry
`lib/armeabi/libjirobiodg.so`. The APK itself remains outside Git; only hashes and
derived metadata are committed.

| Property | Verified value |
|---|---|
| APK SHA-256 | `6654b4af083a899b5c75de92a66b485435b3ce6ac65afbe96082d5ede4eb23d2` |
| ELF entry | `lib/armeabi/libjirobiodg.so` |
| ELF size | 431,528 bytes |
| ELF SHA-256 | `ce6c9f6f7339f226c1f060744ddb89d7b187d01a8b20571cd417896bf4c4fcf0` |
| Format | ELF32, little-endian, EM_ARM, ET_DYN, EABI5 |
| Inspection tool | GNU readelf / Binutils 2.44 |

## Direct dynamic dependencies

The original `DT_NEEDED` sequence is:

1. `libc.so`
2. `libm.so`
3. `liblog.so`
4. `libstdc++.so`
5. `libz.so`
6. `libdl.so`

These are the DSO dependencies encoded in the ELF dynamic section. The GL libraries below are different: they are selected and opened at runtime by the audited GL loader path, not listed in this `DT_NEEDED` sequence.

## Relocation inventory

<badge color="success">Reproduced from uploaded ELF</badge>

| Relocation type | Count |
|---|---:|
| `R_ARM_RELATIVE` | 3,920 |
| `R_ARM_JUMP_SLOT` | 135 |
| `R_ARM_ABS32` | 2 |
| **Total** | **4,057** |

There are 137 symbol-bearing relocation references and 136 unique undefined symbol names. `free` is referenced twice. The complete unique-name list is stored in
`reports/STAGE123O_LIBJIROBIODG_PROVIDER_MANIFEST.json`.

The symbol table contains additional undefined names that are not referenced by the
counted dynamic relocation records. The runtime must not treat every `SHN_UNDEF`
dynamic symbol as a required relocation binding without checking actual relocation
usage. The manifest therefore records relocation-backed names separately.

The ELF also contains a `DT_TEXTREL` tag. The guest loader must keep relocation
targets and page permissions explicit; it must not silently make all guest code
writable or apply an unrestricted host-side text-relocation policy.

## Runtime GL loading is a separate path

The Stage 123H Thumb audit identifies the loader routine at guest VA `0x3d818`.
Its fallback rows are:

| GL mode | Runtime DSO candidates (in order) |
|---|---|
| GLES1 | `libGLESv1_CM.so`, `libhgl.so` |
| GLES2 | `libGLESv2.so`, `libEGL.so` |

These names are not additional `DT_NEEDED` entries. Runtime selection depends on
`GLESVersion` and successful opens. Their presence in the fallback arrays is not
proof that any available Android library exports a compatible ARM32 ABI.

## Consequences for the ARM64 port

1. Stage 123M can now transform already-decoded and already-mapped ELF32 module
   symbol tables into the exact-SONAME provider catalog.
2. Stage 123N can install that catalog behind the guest `dlopen` bridge.
3. The six direct dependencies still require a verified guest provider strategy.
   Android's native ARM64 system DSOs cannot be exposed to the ARM32 guest by
   returning raw host pointers.
4. The GLES/EGL candidate libraries require symbol-by-symbol ABI verification and
   ARM32 guest-callable trampolines. A symbol-name match is insufficient.
5. This report does not prove dependency closure, successful relocations, JNI
   initialization, game boot, or playability.

## Reproduction

With the original APK available locally:

```sh
unzip -p "Resident Evil Degeneration by JawaDroidNew.apk" \
  lib/armeabi/libjirobiodg.so > libjirobiodg.so
sha256sum libjirobiodg.so
readelf -h libjirobiodg.so
readelf -d libjirobiodg.so
readelf -rW libjirobiodg.so
```

The machine-readable report includes provenance, the direct dependency order,
dynamic metadata, relocation totals, all 136 unique relocation-backed undefined
names, and the separately audited runtime GL fallback names.
