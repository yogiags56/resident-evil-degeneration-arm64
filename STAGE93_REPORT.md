# Stage 93 report — Native LZMA-alone decoder integration

## Summary

Integrated an optional, bounded `liblzma` LZMA-alone decoder behind the Stage 92 DTRZ VFS callback. The decoder is not enabled by default in the Android target: Android builds must provide a liblzma package cross-compiled for the target ABI.

## New source

- `runtime/archive/re_dtrz_lzma.h`
- `runtime/archive/re_dtrz_lzma.c`
- `tests/test_re_dtrz_lzma.c`
- `tools/dtrz_native_verify.c`
- `docs/STAGE93_NATIVE_LZMA.md`

## Safety properties

- Default decoder dictionary/state memory limit: 64 MiB, caller configurable.
- Decoded output cannot exceed the VFS entry capacity; an overflow probe detects an extra byte.
- Rejects truncated/corrupt streams, unsupported methods and memory-limit failures.
- `written` is reset to zero on every failure path.
- Existing VFS decoded-entry cap remains 64 MiB.

## Results

- LZMA decoder test: 6 scenarios passed.
- Existing DTRZ VFS test: 29 assertions passed.
- Python suite: 17 tests passed.
- Existing guest CPU/import/runtime/GL/file-contract tests passed.
- UBSan LZMA test passed.
- UBSan DTRZ VFS test passed.
- Native C decoded all 47 entries from the locally supplied original `main.dz`.
- Python and native extraction trees contained the same 47 paths and all 47 decoded files were byte-for-byte identical.
- Combined decoded size: 57,976,257 bytes.

## Not completed

- Android ARM64 liblzma cross-build and NDK link.
- Android device test.
- DTRZ VFS to generic file adapter bridge.
- Guest `s3eFileRead` return convention and handler activation.
- Original game boot or gameplay.

## Next stage

Stage 94 should connect the now-tested DTRZ VFS to the platform-neutral file adapter through a separate archive-backed backend. It must preserve the unresolved guest return convention and must not activate the guest import handler yet. Add host tests for path aliases, file handles, seek/read/EOF and archive lifetime.
