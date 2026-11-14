# Stage 125G — Read-only archive-backed guest stdio

## Goal

Connect the existing guest libc stdio adapter to the existing DTRZ virtual
filesystem, so guest fopen/fread/fseek/ftell/fclose can access packaged game
assets without passing guest pointers to Android host APIs.

## Implementation

Added `re_guest_libc_vfs_backend.h/.c`. The adapter exposes a
`REGUESTLIBCStdioBackend` whose stream handles are pointers to caller-owned,
bounded backend slots. Each active slot owns one `re_dtrz_file`; the DTRZ VFS
continues to own path lookup, archive bounds checking, decompression, and
decoded-file cursor state.

Supported operations:

- `open`: accepts only `r` and `rb`, resolves normalized relative paths
  through the DTRZ VFS, and allocates one of 32 stream slots.
- `read`: copies bytes from the decoded archive entry.
- `seek` / `tell`: support SET/CUR/END with signed-offset overflow checks.
- `close`: closes the decoded entry and releases the slot.
- `write`: an explicit callback returning failure; archive assets are
  intentionally read-only.
- `flush`: absent; the guest stdio provider treats the backend as unbuffered.

The adapter does not accept absolute paths, traversal paths, write/append
modes, or direct host filesystem names. Guest-facing FILE values remain the
opaque generation-tagged tokens created by the existing stdio provider; the
host stream pointer is never returned to guest memory.

## Regression coverage

The host regression constructs a small stored DTRZ entry and exercises open,
read, tell, relative seek, read-to-end, seek-to-end, invalid seek rejection,
write rejection, close, double-close rejection, missing paths, traversal and
absolute-path rejection, and close-all lifecycle.

The new source is registered in the Android ARM64 CMake target and the
regression is added to `tools/run_tests.sh`.

## Limits

This stage is read-only and archive-specific. It does not implement POSIX
`open/read/write/stat` file descriptors, writable app-private storage,
directory enumeration, Android AssetManager path mapping, or complete
filesystem semantics. It also does not install the backend into the
production XE3U/ELF game boot sequence. No original-game boot is claimed.

## Validation

Host and Android ARM64 CI are required for the final source revision.
