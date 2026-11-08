# Stage 92 report — Native DTRZ Index and Virtual File System

## Summary

Stage 92 moves the observed DTRZ metadata parser into native C and adds a read-only in-memory VFS. The implementation is intentionally codec-pluggable: stored entries are supported directly; LZMA-alone method `0x200` requires an injected decoder callback and is not claimed as implemented by this stage.

## Source additions

- `runtime/archive/re_dtrz_vfs.h`
- `runtime/archive/re_dtrz_vfs.c`
- `tests/test_re_dtrz_vfs.c`
- `tools/dtrz_native_inspect.c`
- `docs/STAGE92_NATIVE_DTRZ_VFS.md`

The Android CMake target includes the new native module. Host CI builds and tests it independently.

## Results

- Synthetic archive parser/VFS test: 29 assertions.
- Native parser run against local Stage 91 `main.dz`: 47 entries parsed; 35 folders including root; payload offset 2666; archive length 14,552,804 bytes. All 47 native metadata records (path, offset, packed size, decoded size, method) matched the Stage 91 Python JSON profile exactly.
- Existing Python and native regression suites retained.
- UBSan run for the native parser/VFS test: passed.
- No proprietary game assets included in the repository or release ZIP.

## Not completed

- Built-in LZMA decoder.
- Android NDK build/device execution.
- Bridge to guest `s3eFile*` imports.
- Resolution of `s3eFileRead` return semantics.
- Original game boot or gameplay.

## Next

Stage 93 should integrate a vetted LZMA-alone decoder behind the callback, with bounded output and dictionary-memory limits; compare C-decoded outputs byte-for-byte with Stage 91 Python extraction. Only after that should the archive VFS be connected to the generic file adapter.
