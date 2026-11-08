# Stage 94 Report — Archive-backed File Adapter

## Outcome

The native DTRZ VFS and LZMA decoder are now reachable through the existing generic guest file adapter. This is a host-tested integration milestone; it does not activate original game imports or claim Android runtime success.

## New/updated source

- `runtime/archive/re_dtrz_file_backend.h`
- `runtime/archive/re_dtrz_file_backend.c`
- `app/src/main/cpp/re_file_adapter.h` (consolidated Stage 82 source)
- `app/src/main/cpp/re_file_adapter.c` (consolidated Stage 82 source)
- `tests/test_re_dtrz_file_backend.c`
- `tests/test_re_file_adapter.c`
- `tools/dtrz_adapter_verify.c`
- `tools/run_archive_adapter_validation.sh`

Android CMake now compiles the archive backend alongside the existing VFS. LZMA remains an opt-in dependency requiring a liblzma build for Android ARM64.

## Test results

- Full `bash tools/run_tests.sh`: passed.
- DTRZ adapter integration: 20 assertions passed.
- Stage 82 generic adapter regression: passed.
- DTRZ VFS regression: 29 assertions passed.
- LZMA regression: 6 scenarios passed.
- Guest CPU, import dispatcher, runtime coordinator, GL adapter, and Stage 89 file-read contract regressions: passed.
- UBSan on DTRZ adapter integration: passed.
- Real local `main.dz` adapter walk: 47/47 entries opened, read to EOF, and closed; 57,976,257 bytes delivered.
- All 47 per-entry FNV-1a digests and decoded sizes matched the existing Stage 93 Python extraction.
- Active stream count returned to zero.

## Important caveats

The adapter's generic open callback collapses backend open failures to `RE_FILE_E_NOT_FOUND`; a diagnostic accessor preserves the exact VFS status. Each open currently decodes a complete entry into memory. Seek is not exposed through `REFileBackend`. The original `s3eFileRead` return convention is still unknown, so no guest import handler was activated.

Android NDK build/device validation remains outstanding. The game has not booted and is not playable yet.
