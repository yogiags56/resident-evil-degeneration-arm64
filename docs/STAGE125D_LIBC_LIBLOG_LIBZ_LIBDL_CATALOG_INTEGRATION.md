# Stage 125D — Atomic libc.so + liblog.so + libz.so + libdl.so catalog integration

## Goal

Complete the four directly identified runtime-provider adapters in one exact
SONAME catalog: the implemented libc subset, Android logging, zlib inflate,
and the guest dynamic-loader bridge. This specifically covers all six direct
DT_NEEDED libraries only partially; `libm.so` and `libstdc++.so` remain
separate work items.

## Implementation

Added `re_guest_dl_libc_log_zlib_dl_catalog` and its public initializer.
It extends the previous Stage 125C initializer with the four relocation-backed
libdl imports:

- `dlopen`
- `dlsym`
- `dlclose`
- `dlerror`

The initializer binds those imports through the existing guest DL bridge,
builds four guest Thumb export records under exact SONAME `libdl.so`, and
commits them alongside:

- 36 implemented libc imports under `libc.so`
- 2 Android logging imports under `liblog.so`
- 4 inflate-related imports under `libz.so`

The pending bridge is initialized with callbacks whose context points to the
caller-owned catalog object. This is intentional: after the pending catalog
is committed into that object, future guest `dlopen/dlsym/dlclose` callbacks
observe the committed entries. The bridge object and all providers/export
arrays must remain alive for the full guest execution lifetime. The caller
must quiesce/close any old bridge handles before replacing an existing bridge.

The combined initializer rolls back the import table on any provider bind or
catalog validation failure and leaves the published catalog and export arrays
untouched.

## Regression coverage

Added `test_re_guest_dl_libc_log_zlib_dl_catalog.c`. It verifies:

- All four provider SONAMEs coexist with a pre-existing sample module.
- All four `libdl.so` exports have nonzero guest Thumb trampoline addresses.
- The installed bridge opens the sample DSO, resolves a guest symbol, and
  closes its opaque guest handle.
- Exact `libdl.so` catalog lookup returns the four generated guest addresses.

The regression is wired into host tests. The new combined module is included
in the Android ARM64 CMake target.

## Dependency closure status

The original ELF32 manifest has six direct DT_NEEDED entries:
`libc.so`, `libm.so`, `liblog.so`, `libstdc++.so`, `libz.so`, and
`libdl.so`. This stage integrates four provider families in the shared
catalog, but does not claim a complete runtime closure:

- `libm.so`: still no direct relocation-backed math symbol names in the
  audited manifest; its declared dependency and initialization behavior
  still require explicit handling/evidence.
- `libstdc++.so`: four relocation-backed C++ allocation operators are
  separately handled by the C++ heap provider, but that provider is not yet
  integrated into this combined initializer.
- ARM EABI helpers are implemented as a separate family and are not a
  SONAME inference for `libm.so`.
- The 101 libc/POSIX/Bionic candidate imports remain only partially provided.
- GLES/EGL runtime handles and callable adapters remain separate.

This stage does not wire the complete XE3U-to-ELF production boot sequence,
does not prove all 137 symbol relocation references resolve, and does not
claim original-game boot or physical-device execution.

## Validation

Host regression and Android ARM64 CI are pending for the final source commit.
