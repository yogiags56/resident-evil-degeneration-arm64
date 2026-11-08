# Stage 92 — Native DTRZ parser and read-only virtual file system

## Implemented

- Native C parser for the observed DTRZ header, filename/folder strings, six-byte attribute records, four-byte location header, and sixteen-byte location records.
- Bounds checks for table windows, monotonic payload offsets, folder/file indices, and 32-bit archive offsets.
- Safe path construction and lookup; absolute paths, drive syntax, empty components, `.` and `..` are rejected.
- Read-only in-memory VFS with open, partial read, EOF, seek, tell, and close.
- Stored method `0x100` is copied internally after exact-size validation.
- Other compression methods use an injected decoder callback. The repository does not silently pretend to implement LZMA; method `0x200` remains unsupported until a real codec is linked and independently tested.

## ABI / ownership

- `re_dtrz_archive_parse` allocates entry/path metadata; it borrows the archive byte buffer. The caller must keep that byte buffer alive until the archive is destroyed and all VFS files are closed.
- `re_dtrz_archive_destroy` releases metadata only.
- `re_dtrz_open` allocates one decoded entry buffer owned by `re_dtrz_file`; it rejects decoded entries larger than the default 64 MiB safety limit. The caller must pass a zeroed/closed output file object.
- `re_dtrz_close` releases the decoded buffer and clears the file object.
- VFS handles are host pointers/objects, not guest ARM32 file handles. A later bridge must wrap them in generation-checked 32-bit tokens, like the existing file adapter.

## Validation

`tests/test_re_dtrz_vfs.c` builds a synthetic archive fixture and checks metadata parsing, root and nested paths, path traversal rejection, stored reads, partial reads, EOF, seek/tell, close behavior, injected decoder behavior, unsupported codec behavior, and corrupt-offset rejection.

`tools/dtrz_native_inspect.c` is a host utility. It was also run against a locally supplied `main.dz` during Stage 92; the binary is not part of this source repository. Expected observed profile: 47 entries, 35 folders including root, payload boundary `0xA6A` (2666), and archive length 14,552,804 bytes.

## Deliberate limitations

- No built-in LZMA-alone decoder is shipped in this stage.
- No asset archive is bundled; game data remains user-supplied and local-only.
- This VFS is not yet wired into the ARM32 `s3eFileOpen` / `s3eFileRead` import handlers.
- `s3eFileRead` guest return-value semantics remain unresolved, so no guest `r0` convention is inferred.
- No Android device build or game boot is claimed.
