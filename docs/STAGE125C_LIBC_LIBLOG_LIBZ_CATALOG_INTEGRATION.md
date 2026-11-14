# Stage 125C — Atomic libc.so + liblog.so + libz.so catalog integration

## Goal

Extend the combined Stage 125B DSO catalog with the existing guest zlib
provider, making the original game's four identified inflate imports available
through exact SONAME lookup and the shared guest import dispatcher.

## Implementation

Added `re_guest_dl_libc_log_zlib_catalog` and its public initializer. The
caller initializes `REGUESTZLIBProvider` with a guest-only token range before
catalog creation. The initializer binds:

- The implemented 36-symbol libc subset under `libc.so`.
- `__android_log_print` and `__android_log_write` under `liblog.so`.
- `inflate`, `inflateEnd`, `inflateInit2_`, and `inflateInit_` under
  `libz.so`.

All providers use the consumer ELF32 dynamic-symbol inventory and one shared
import table. The combined catalog, three export arrays, and import table are
published transactionally. Failure in a later provider restores the import
table and leaves catalog/export storage untouched.

## Regression coverage

Added `test_re_guest_dl_libc_log_zlib_catalog.c`. It checks all four libz
exports through exact `libz.so` open/symbol/close callbacks, confirms the
three providers coexist with a pre-existing module, and forces a missing
`inflateInit_` import after libc and liblog have bound to verify rollback.

The integration test is included in host test execution. The module is part of
the Android ARM64 native CMake target; the target already links Android zlib.

## Scope and limitations

The guest zlib adapter currently exposes only the four inflate-related
functions listed above. It is not a complete zlib ABI and does not yet expose
compression, checksum, one-shot, or convenience APIs. Provider state lifetime
and guest token-range allocation remain caller responsibilities.

This milestone does not complete `libm.so`, `libstdc++.so`, or `libdl.so`,
does not provide GLES/EGL callable adapters, and does not wire the complete
XE3U-to-ELF-to-entry production boot sequence. No original-game boot or
physical-device execution is claimed.

## Validation

Host regression and Android ARM64 CI are pending for the final commit.
