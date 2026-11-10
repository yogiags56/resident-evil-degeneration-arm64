# Stage 123C — Bounded LZMA-alone to XE3U decode integration

## Implemented
- Added `re_guest_xe3u_decode.h/.c`.
- Reuses the existing bounded liblzma-alone decoder rather than introducing a second decompression implementation.
- Accepts caller-owned source and destination buffers plus an explicit decoder memory limit.
- Requires a complete LZMA-alone stream and rejects trailing input according to the shared decoder contract.
- Validates the decoded output with the native XE3U header parser before publishing the decoded byte count/header.
- Maps decoder memory-limit, output-capacity, malformed-stream, and XE3U-header failures to distinct XE3U decode statuses.
- Keeps the output header and written count unchanged/zero on failure; caller must discard partially written destination bytes after any decode failure.
- Added a compressed synthetic XE3U fixture. Test verifies decode, header parse, fixup parse, output-capacity rejection, and malformed stream rejection.
- Added `tools/xe3u_native_audit.c`, a host CLI that decodes a locally supplied `biohazard.s3e` and prints the header plus fixup-section/symbol/relocation counts without applying relocations.
- Registered the CLI for strict compilation in the liblzma-enabled host test path.
- The wrapper is compiled into Android native target only when `RE_ENABLE_LIBLZMA=ON`; host regression is registered when `pkg-config liblzma` is available.

## Explicit boundary
- The synthetic compressed fixture is not the original game executable.
- The original user-supplied APK and `biohazard.s3e` are not stored in Git. This wrapper has not yet been run against the original 673,661-byte compressed S3E asset or compared to the Stage 97 decoded-image SHA-256.
- No code/data/BSS mapping, internal relocation writes, external S3E import binding, constructor execution for XE3U, or game entry execution is implemented here.

## Validation
Host CI run 42869522643 passed, including strict compilation of the native audit CLI, and explicitly reported:
- Guest XE3U LZMA decode tests: PASS
- Guest XE3U header and fixup parser tests: PASS

To audit a locally staged original asset after installing host liblzma, run `build/host-tests/xe3u_native_audit /path/to/biohazard.s3e` (the host test script compiles this tool). Android NDK build and validation against the user's original S3E asset remain required.
