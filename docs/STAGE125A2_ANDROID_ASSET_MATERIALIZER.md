# Stage 125A.2 — Android AssetManager module materializer

## Implementation

Added a host-testable materialization layer:

- `re_guest_module_materializer.h/.c` defines source callbacks, bounded module buffers, transactional output, and cleanup.
- `re_android_module_asset_source.h/.c` adapts an initialized Android `AAssetManager` to the generic source callbacks.
- Android CMake compiles both the generic materializer and Android adapter.

## Safety and ownership contract

- Requires a valid v1 manifest and an empty, zero-initialized output set.
- Revalidates relative asset paths before opening: rejects absolute paths, traversal, empty/repeated components, trailing separators, control/non-ASCII bytes, and colon.
- Enforces each manifest entry's maximum size and a 256 MiB aggregate cap.
- Reads each asset into caller-owned heap storage, looping over positive short reads in chunks no larger than 1 MiB.
- Rejects zero/negative reads before the advertised length is reached and rejects over-reported read counts.
- Closes each opened asset on success and every failure path.
- Commits the output set only after every module is fully read; failures free all temporary buffers and leave output unchanged.
- Caller releases successful buffers with `re_guest_module_materialized_free`.

## Tests

Added `tests/test_re_guest_module_materializer.c` and registered it in `tools/run_tests.sh`. Coverage includes successful short-read accumulation, byte-for-byte payload integrity, close semantics, read failure rollback, path traversal rejection, and missing asset failure.

## Limits / next stage

This stage does not yet verify ELF class/machine/endianness/SONAME against module metadata, cryptographic digests, or feed buffers into the ELF32 pipeline. The Android adapter reads packaged APK assets only; it does not extract to shared storage. The current manifest format has no digest field. Those are follow-up gates. No original game assets or game boot are claimed.
