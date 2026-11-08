# Stage 94 — DTRZ archive-backed file adapter

## Goal

Connect the native DTRZ VFS (Stages 92–93) to the generic guest file adapter foundation (Stage 82) without enabling any guest `s3eFile*` import handler or guessing the unresolved `s3eFileRead` return convention.

## Implementation

- `runtime/archive/re_dtrz_file_backend.h/.c` implements the `REFileBackend` callbacks over a parsed `re_dtrz_vfs`.
- `app/src/main/cpp/re_file_adapter.c/.h` is now included in the GitHub source tree, rather than being only present in the earlier Stage 82 package.
- `tests/test_re_dtrz_file_backend.c` drives a guest-memory path and mode through the adapter, then reads from a minimal in-memory DTRZ stored-entry fixture.
- `tests/test_re_file_adapter.c` restores the Stage 82 generic adapter regression suite to the consolidated repository.
- `tools/dtrz_adapter_verify.c` and `tools/run_archive_adapter_validation.sh` validate all entries in a locally supplied `main.dz` through the complete adapter/backend/VFS/LZMA path.
- Android CMake includes the new backend source. LZMA remains opt-in and requires an Android-ABI liblzma build.

## Backend contract

- Read-only `rb` mode only.
- `open` resolves the archive path using the VFS and creates one decoded-entry stream.
- `read` returns bytes transferred; zero means EOF, negative means backend error.
- `get_char` returns 0–255, `-1` for EOF, and `-2` for backend error.
- `close` releases the decoded entry and decrements the active stream count.
- Archive bytes, parsed index, VFS, and decoder context are borrowed. They must remain alive until all streams are closed.
- The adapter's existing `open` callback returns only a pointer or NULL. Consequently it maps backend-open failures to `RE_FILE_E_NOT_FOUND`; `re_dtrz_file_backend_last_open_status()` preserves the exact VFS status for diagnostics.

## Validation

### Synthetic host integration

- 20 assertions passed for VFS -> archive backend -> generic guest file adapter.
- Covered path open, sequential reads, partial read at EOF, `get_char`, EOF, close, stale-handle rejection, missing path diagnostics, and traversal rejection.
- Stage 82 generic file adapter regression passed.
- Native VFS regression: 29 assertions passed.
- Native LZMA regression: 6 scenarios passed.
- Existing guest CPU/import/runtime/GL and file-read contract regressions passed.
- UndefinedBehaviorSanitizer integration run passed.

### Locally supplied original archive

Using a local copy of the original APK's `assets/main.dz` (not included in this repository):

- All 47 entries opened through `REFileAdapter`.
- All 47 entries read sequentially to EOF and closed.
- Total bytes delivered through the adapter: 57,976,257.
- Per-entry size matched the parsed decoded size.
- Per-entry FNV-1a digest from the adapter stream matched the corresponding Stage 93 Python extraction for all 47 paths.
- Active stream count returned to zero.

Reproduce with:

```bash
./tools/run_archive_adapter_validation.sh /path/to/main.dz
```

Keep the proprietary APK, archive, and extracted files outside the repository.

## Limitations and safety gates

- This is a host-validated backend. Android NDK build and physical-device execution have not been performed here.
- The archive is parsed from a memory buffer; the caller owns that buffer and all VFS lifetime dependencies.
- Each open currently decompresses the entire entry into memory. It is not streaming LZMA and does not cache decoded entries. Multiple simultaneous large opens can consume substantial memory.
- The generic `REFileBackend` interface has no seek callback. VFS seek exists, but Stage 94 does not expose seek through the guest file adapter.
- The generic adapter's open callback cannot distinguish VFS decode errors from missing paths in its return value; the backend diagnostic field records the detailed VFS result.
- No `s3eFileOpen`, `s3eFileRead`, `s3eFileGetChar`, or `s3eFileClose` guest import handler is activated by this stage.
- The `s3eFileRead` guest return convention remains unresolved and quarantined.
- No original game boot or gameplay has been observed.
