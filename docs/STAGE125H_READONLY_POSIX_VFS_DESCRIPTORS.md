# Stage 125H — Read-only POSIX descriptors over DTRZ VFS

## Goal

Expose archive-backed assets through a small POSIX-like descriptor API, sharing
the same DTRZ VFS semantics as Stage 125G guest stdio without creating a
second archive reader or exposing host pointers.

## API

Added `re_guest_vfs_posix.h/.c` with a caller-owned 32-slot descriptor table.
Descriptors begin at 3 and are reused after close.

- `open(path, O_RDONLY)`: resolves a relative archive entry and creates a
  descriptor. All write/create/truncate/append modes are rejected.
- `read(fd, dst, count)`: reads decoded entry bytes.
- `lseek(fd, offset, whence)`: supports SET/CUR/END and signed offsets with
  overflow and bounds checks.
- `close(fd)`: closes the decoded file and releases the descriptor.
- `access(path, F_OK/R_OK)`: checks entry presence/readability; W_OK/X_OK
  are denied.
- `stat(path)`: returns regular-file mode 0444 and decoded size.
- `close_all()`: releases all active descriptors.

The API is a portable backend surface, not yet a guest AAPCS32 import
dispatcher. The returned descriptor values are integers; guest memory
validation and symbol binding belong to the later import adapter.

## Tests

Host tests cover descriptor allocation/reuse, read, all seek origins,
negative relative/end offsets, EOF, out-of-range seek, invalid descriptor,
double close, read-only open policy, access checks, stat metadata, traversal
rejection and close-all.

The source is registered in Android ARM64 CMake and `tools/run_tests.sh`.

## Limitations

This is an archive-backed read-only subset, not full POSIX/Bionic. It does not
implement write descriptors, create/unlink/rename, directories, directory
enumeration, timestamps, ownership, permissions beyond read-only regular
files, errno mapping, or guest AAPCS32 symbol dispatch. It does not yet wire
into the production XE3U/ELF boot pipeline. No original-game boot is claimed.

## Validation

Host and Android ARM64 CI are required for the final revision.
