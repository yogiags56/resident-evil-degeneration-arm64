# Stage 93 — bounded native LZMA-alone decoder

## Scope

Stage 93 adds an optional `liblzma`-backed implementation for the DTRZ VFS decoder callback. The code supports the observed `0x200` LZMA-alone method. It does not claim support for other DTRZ compression methods.

## Implementation

- `runtime/archive/re_dtrz_lzma.h/.c` implements `re_dtrz_lzma_alone_decode`.
- Decoder dictionary/state memory is limited by a caller-configurable `memlimit` (64 MiB default).
- Output is limited by the VFS-provided destination capacity; a one-byte overflow probe detects output beyond that bound.
- Truncated/corrupt streams, memory-limit failures, unsupported methods, and output overflow return distinct VFS status codes.
- `tools/dtrz_native_verify.c` parses a user-supplied archive, decodes every entry through the native VFS, and writes the output under a caller-selected directory.
- The Android CMake integration is opt-in (`RE_ENABLE_LIBLZMA=ON`) and requires a liblzma package built for the selected Android ABI. The project does not silently link a host library into Android.

## Reproduction

Host dependency: `liblzma` development headers/library and `pkg-config`.

```bash
bash tools/run_tests.sh
clang -std=c11 -Wall -Wextra -Werror -Iruntime/archive \
  runtime/archive/re_dtrz_vfs.c runtime/archive/re_dtrz_lzma.c \
  tools/dtrz_native_verify.c $(pkg-config --cflags --libs liblzma) \
  -o /tmp/dtrz_native_verify
/tmp/dtrz_native_verify /path/to/main.dz /path/to/native-output
```

Python oracle:

```bash
python3 tools/dtrz_extract.py /path/to/main.dz --extract /path/to/python-output --verify
```

Compare the two output trees byte-for-byte. Do not commit either output tree or the original archive.

## Stage 93 observed validation

Using the locally supplied original `main.dz` during this engineering run:

- Native C parser opened all 47 entries.
- Native liblzma decoder completed all 47 LZMA-alone streams.
- Python Stage 91 extraction completed all 47 entries.
- All 47 relative paths matched and every native output file was byte-identical to its Python counterpart.
- Combined decoded payload size: 57,976,257 bytes.
- LZMA unit test: 6 scenarios passed, including success, output cap, unsupported method, truncated input, dictionary memory cap, and corrupt stream.
- Existing native VFS regression: 29 assertions passed.
- UBSan: LZMA and VFS tests passed.

The archive and decoded game assets are not included in this repository. The byte-for-byte result is recorded as a local validation result, not as a public fixture.

## Limitations

- The native decoder uses external liblzma; the dependency must be built for Android ARM64 before enabling it in the Android app.
- Android NDK build/device execution was not performed in this environment.
- The VFS is not yet connected to guest `s3eFile*` imports.
- `s3eFileRead` guest return semantics remain unresolved and must remain quarantined.
- No original game boot/gameplay has been observed.
