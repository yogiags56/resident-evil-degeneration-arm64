# Stage 125B — Atomic libc.so + liblog.so catalog integration

## Goal

Extend the Stage 125A exact-SONAME provider catalog so both the implemented
libc subset and Android logging imports can be opened and resolved through the
same guest DSO catalog and dynamic-link bridge.

## Implementation

Added `re_guest_dl_libc_log_catalog` with a combined initializer that:

1. Copies the caller's ordered DSO sources into a private pending source list.
2. Binds the Stage 124E libc provider set against the consumer ELF32 dynamic
   symbol inventory.
3. Binds the two relocation-backed logging imports,
   `__android_log_print` and `__android_log_write`, through the guest import
   dispatcher.
4. Validates the combined catalog with exact `libc.so` and `liblog.so`
   entries.
5. Publishes the catalog and both caller-owned export arrays only after all
   steps succeed.

If either provider bind or catalog validation fails, the import table is
restored and caller catalog/export storage remains unchanged. The provider
objects and export arrays must outlive all guest calls and open catalog
handles.

## Regression coverage

Added `test_re_guest_dl_libc_log_catalog.c` and wired it into host tests.
It checks:

- Existing DSO, `libc.so`, and `liblog.so` coexist in one catalog.
- Exact SONAME open and symbol lookup for `malloc`,
  `__android_log_print`, and `__android_log_write`.
- A missing second logging import fails after libc binding and rolls back the
  import table, catalog, and both export arrays.

The new integration module is included in the Android ARM64 native CMake
target.

## Scope and limitations

The log provider exposes only the two verified relocation-backed imports
implemented by the current adapter. This does not implement the complete
Android logging API, the complete Bionic ABI, or all imports required by the
original game. The zlib provider remains a separate adapter and is not yet
added by this combined initializer. `libm.so`, `libstdc++.so`, `libz.so`,
`libdl.so` coverage, GLES/EGL callable adapters, production XE3U boot
integration, and original-game boot remain outstanding.

No original game execution or physical-device runtime test is claimed.

## Validation

Host regression and Android ARM64 build are pending for the final commit.
