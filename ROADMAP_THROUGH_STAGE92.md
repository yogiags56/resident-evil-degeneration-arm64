# Roadmap through Stage 92

- Stage 90: GitHub-ready source consolidation and DTRZ metadata boundary inspection.
- Stage 91: DTRZ attribute/location table reconstruction; all 47 observed payloads decompressed and size-verified using Python LZMA-alone.
- Stage 92: Native C DTRZ metadata parser and read-only VFS with stored-method support, injected codec callback, 64 MiB decoded-entry guard, 29 assertions, UBSan pass, and exact metadata match for 47 real archive entries.

## Next stages

- Stage 93: Integrate a vetted bounded LZMA-alone decoder; cross-check all 47 entries against Python reference output.
- Stage 94: Add archive-backed VFS integration tests for path lookup, stream semantics, and memory limits.
- Stage 95: Bridge the VFS through generation-checked host handles into the generic file adapter, while keeping guest ABI return semantics quarantined.
- Stage 96: Resolve original `s3eFileRead` return semantics from executable evidence and callsites.
- Stage 97+: Android NDK/device validation, controlled guest import activation, and runtime diagnostics.

The game has not booted or become playable. Each stage must report observed facts separately from inference and unknowns.
