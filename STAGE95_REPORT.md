# Stage 95 Report — Guest File API Bridge Readiness Audit

## Outcome

Consolidated the existing guest `s3eFile*` evidence and added typed argument-capture helpers for `s3eFileOpen`, `s3eFileGetChar`, and `s3eFileClose`. These helpers are intentionally non-executing audit utilities. No guest import was activated and no guest return register is synthesized.

## Findings

- `s3eFileOpen`: `(path, mode)` in `r0-r1`; observed mode `rb`; returned value is checked for null and retained as a handle-like token.
- `s3eFileRead`: `(destination, item_size, item_count, handle)` in `r0-r3`; return and short-read semantics remain unknown.
- `s3eFileGetChar`: handle in `r0`; inspected guest helper recognizes `-1` as EOF/error.
- `s3eFileClose`: handle in `r0`; caller clears its stored handle after the call; return is ignored.

The Stage 94 archive contains source and prior analysis reports, but not the original APK or guest executable. Stage 95 therefore carries forward prior callsite evidence rather than claiming a new binary disassembly.

## New/updated source

- `app/src/main/cpp/re_s3e_file_api_audit.h/.c`
- `tests/test_re_s3e_file_api_audit.c`
- Android CMake and host test runner updated to compile the audit module.
- `docs/STAGE95_GUEST_FILE_API_BRIDGE_READINESS.md`

## Validation

- Stage 95 argument audit: 14 assertions passed.
- Stage 89 read contract regression passed; read-return authorization remains false.
- Full host test suite passed.
- Strict host mock-header C compilation passed.

## Still blocked

- Exact SDK return/error contract for `s3eFileRead`.
- Exact original SDK declarations/semantics for all four APIs.
- Android NDK build with Android-ABI liblzma and physical-device validation.
- Guest import activation and original game boot/gameplay.

See `docs/STAGE95_GUEST_FILE_API_BRIDGE_READINESS.md` for the full evidence matrix and Stage 96 plan.
