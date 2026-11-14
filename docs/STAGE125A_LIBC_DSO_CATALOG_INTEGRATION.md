# Stage 125A — libc.so DSO catalog integration

## Goal

Connect the Stage 124E aggregate libc binder to the existing guest DSO provider
catalog, so the dynamic-link bridge can open an exact `libc.so` SONAME and
resolve the implemented guest-callable exports through the same bounded
catalog used for other providers.

## Implementation

Added `re_guest_dl_libc_catalog`:

- Accepts the existing ordered provider sources and the consumer ELF32
  dynamic-symbol table.
- Binds the four implemented libc provider families through the Stage 124E
  transactional binder.
- Appends one exact `libc.so` provider entry to a staged catalog.
- Publishes the catalog, libc export storage, and import table only after
  catalog validation succeeds.
- Restores the import table if libc binding or catalog validation fails.
- Rejects a duplicate `libc.so` entry instead of silently shadowing it.

The libc export array is caller-owned and must outlive the catalog and any
open handles. Provider contexts and their owning provider instances must also
remain alive while import trampolines can dispatch them.

## Regression coverage

Added `test_re_guest_dl_libc_catalog.c` and wired it into host tests. It
verifies:

1. A normal DSO source and the synthesized `libc.so` entry coexist.
2. The catalog opens `libc.so` by exact SONAME.
3. `dlsym`-style catalog lookup resolves `malloc` to its guest trampoline.
4. A missing final stdio symbol fails after earlier provider families have
   bound, while the prior catalog, import table, and caller export buffer
   remain unchanged.

The Android ARM64 native target includes the new integration module.

## Scope and limitations

This is the first catalog connection for the implemented libc subset. It does
not mean the full Bionic ABI is available, nor does it resolve all imports of
the original game. Other required DSO providers (including libm, libstdc++,
liblog, libz and libdl) still need verified export coverage and compatible
guest-callable implementations. The production XE3U-to-ELF loader flow,
GLES/EGL callable adapters, and original-game boot remain outstanding.

The input symbol table is the consumer ELF32 dynamic-symbol table whose
undefined imports are converted to trampolines; it is not a host Android
symbol table and no host function pointers are exposed to guest code.

## Validation

Host regression and Android ARM64 native build are pending for the final
commit.
