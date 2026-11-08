# Stage 99 — Guest Dispatcher + DTRZ End-to-End Integration

## Objective

Exercise the Stage 98 Open/Read/GetChar/Close handlers through the actual synthetic AAPCS32 guest import dispatcher and the DTRZ-backed file backend. This closes the integration gap between direct bridge tests and the guest register/memory boundary without enabling production imports.

## Implementation

- Added `tests/test_re_s3e_file_bridge_dispatch_dtrz.c`.
- Constructed a minimal synthetic DTRZ archive with a stored `readme.txt` entry containing `hello`.
- Mounted the archive through `re_dtrz_archive_parse` -> `re_dtrz_vfs` -> `re_dtrz_file_backend` -> `REFileAdapter` -> `RES3EFileBridge`.
- Bound the four real Stage 98 handler functions to test-only synthetic import symbols.
- Invoked each handler through `re_guest_import_dispatch`, passing guest ABI arguments in r0-r3 and verifying return through guest LR.

## Verified behavior

1. Open reads path/mode strings from guest memory and returns a non-null opaque guest handle in r0.
2. Read requests two items of two bytes; the dispatcher returns `r0 == 2` and guest memory receives `hell`.
3. GetChar returns `o`, then `0xFFFFFFFF` (`-1`) at EOF.
4. Close returns zero and releases the DTRZ backend stream.
5. A stale handle passed through the dispatcher is rejected as a handler error; no invented guest error sentinel is returned.
6. Production-import authorization remains false.

## Validation

- New Stage 99 integration test: **31 assertions passed**.
- Full `bash tools/run_tests.sh`: **PASS** (22 Python tests, host mock syntax checks, and all native host tests including Stages 94–98 and the new Stage 99 integration).
- This is a host-side synthetic archive test. It does not execute the proprietary game binary, validate the exact original Marmalade partial-read/error contract, or constitute an Android NDK/device test.

## Safety boundary / remaining work

Production guest file imports remain disabled. Stage 97 established the `s3eFileRead` return unit as item count, but exact behavior for partial items and backend errors still requires version-matched documentation or controlled traces from the original runtime. Actual imported symbol indices should only be bound after reproducing and checking the original symbol table mapping in the live loader path.

## Next milestone — Stage 100

- Close the original `s3eFileRead` partial-item and error semantics with evidence, or explicitly document the remaining uncertainty and retain the quarantine.
- Add an import-table mapping audit that connects the parsed XE3U symbol table to the four audited APIs without hard-coded guesses.
- Begin Android ARM64 build validation, including a reproducible liblzma dependency build/configuration.
- Keep physical-device validation and original-game boot as separate acceptance gates.
