# Stage 96 — Synthetic Guest File Import Dispatch

## Outcome

Stage 96 adds a test-only end-to-end bridge exercise for the four audited file APIs. The test registers synthetic symbol IDs with the existing AAPCS32 import dispatcher, sends guest register words through a trampoline, runs the Stage 95 `Open`/`GetChar`/`Close` capture helpers or Stage 89 `Read` capture helper, and verifies return-register/LR plumbing.

**This does not activate production guest file imports.** Synthetic symbol IDs and synthetic return markers exist only inside the host test. They are not the game's actual symbol slots and do not model Marmalade success, EOF, or error return values.

## New test

- `tests/test_re_s3e_file_import_dispatch.c`
  - Registers four test-only bindings in `REGUESTIMPORTTable`.
  - Exercises AAPCS32 `r0-r3` capture for Open, Read, GetChar, and Close.
  - Verifies `r0/r1` propagation and Thumb LR return through the generic dispatcher.
  - Verifies invalid `Read` arguments cause the synthetic handler to fail rather than manufacturing a guest-visible API error code.
  - Verifies the Stage 89 and Stage 95 production authorization gates remain false.
  - **28 assertions pass.**

The test is compiled and run by `tools/run_tests.sh` alongside the existing host tests.

## Public-reference research note

A public Marmalade tutorial demonstrates the common call pattern `s3eFileOpen(..., "rb")`, `s3eFileRead(buffer, sizeof(element), count, file)`, and `s3eFileClose(file)`. Its sample ignores the `s3eFileRead` return value. This is useful corroboration of API usage, but it does **not** specify the function's return units, short-read behavior, EOF/error behavior, or the exact SDK version used by this game. It is not treated as an authoritative ABI contract:

- https://esolution-inc.com/blog/integrating-the-dolby-audio-api-with-marmalade--cms-21365.html (see the `Sound` constructor example)

No matching official SDK header/reference or controlled original-runtime trace was recovered during Stage 96.

## Validation

- Python tests: 17 passed.
- Host mock-header syntax checks: passed.
- Existing DTRZ backend: 20 assertions passed.
- LZMA: 6 scenarios passed.
- Native DTRZ VFS: 29 assertions passed.
- Guest CPU, GL import, import dispatcher, and runtime tests: passed.
- Stage 95 file API capture: 14 assertions passed.
- Stage 89 `s3eFileRead` argument contract: 10 assertions passed; guest return authorization remains false.
- Stage 96 synthetic file import dispatch: 28 assertions passed.

All checks are host-side. They do not validate Android ABI linkage, original SDK semantics, or game boot/gameplay.

## Evidence and limitations

- Stage 81/84 binary callsite and veneer findings remain carried-forward evidence; the original APK and decompressed `biohazard.s3e` were not present in the Stage 95 repository ZIP.
- The public tutorial is an example program, not a version-matched SDK contract.
- The synthetic test proves dispatcher mechanics and argument-capture integration only.
- No file backend is called by the synthetic handler; no guest path is dereferenced; no host handle is exposed to the guest.
- `s3eFileRead` return units and short-read/error behavior remain unknown.
- Guest-visible handle lifecycle, stale/invalid handle policy, repeated close, and error mapping remain unproven.
- No guest file import is registered or enabled in production.

## Next stage — Stage 97

1. Obtain the original APK or decompressed `biohazard.s3e` to repeat the complete xref/callsite audit and determine whether read results are consumed elsewhere.
2. Recover an SDK header/reference matching the exact Marmalade generation, or capture controlled traces from the original 32-bit runtime.
3. Establish open/read/get-char/close semantics including full read, partial read, EOF, missing path, invalid/stale handle, repeated close, and cleanup.
4. Only then design a real backend-backed guest handler and authorize each import individually.
5. Separately continue Android ARM64 NDK/liblzma build and device validation; the synthetic host test is not a substitute.
