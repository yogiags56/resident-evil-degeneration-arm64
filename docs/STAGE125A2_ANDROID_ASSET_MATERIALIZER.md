# Stage 125A.2 — Android AssetManager module materializer

## Implementation

Added a host-testable materialization layer:

- `re_guest_module_materializer.h/.c` defines source callbacks, bounded module buffers, transactional output, and cleanup.
- `re_android_module_asset_source.h/.c` adapts an initialized Android `AAssetManager` to generic source callbacks.
- Android CMake compiles the generic materializer and Android adapter.

## Safety and ownership contract

- Requires a valid v1 manifest and an empty, zero-initialized output set.
- Revalidates safe relative paths before opening; rejects absolute paths, traversal, empty/repeated components, trailing separators, controls, non-ASCII bytes and colon.
- Enforces per-entry byte limits and a 256 MiB aggregate cap.
- Accumulates positive short reads in chunks no larger than 1 MiB.
- Rejects premature EOF, read errors and over-reported byte counts.
- Closes every opened asset on success and failure.
- Commits output only after all modules are fully read; failure frees temporary buffers and leaves output unchanged.
- Caller releases successful buffers with `re_guest_module_materialized_free`.

## CI incident and correction

The first Stage 125A.2 CI attempt exposed an over-escaped backslash character literal in the path validator. Both host and Android builds failed under `-Werror` with multi-character constant diagnostics. Corrected the literals to valid C backslash character constants in commit `1c10be6172901b104745e7e642d1130a3c4abdc2`. Validation is being rerun against that exact commit.

## Tests and limits

Added `tests/test_re_guest_module_materializer.c` for short-read accumulation, byte integrity, close semantics, read failure rollback, traversal rejection and missing asset failure; registered it in the host test runner.

ELF identity/digest verification and integration of materialized buffers into the ELF32 loader remain subsequent stages. Android adapter reads packaged APK assets only. No original game payload or game boot is claimed.
