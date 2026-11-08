# Roadmap through Stage 97

- Stages 1–73: original executable/JNI/loader analysis, partial ARM32 guest CPU, import dispatch infrastructure, and individually verified GL import adapters.
- Stages 74–81: file/memory import audits, file API ABI and callsite tracing.
- Stage 82: platform-neutral guest file adapter foundation.
- Stage 83: original APK asset inventory and real-asset prefix validation.
- Stage 84: original path strings and file-read semantics investigation.
- Stage 85: Android `AAssetManager` backend foundation.
- Stage 86: JNI AssetManager lifecycle bridge.
- Stages 87–88: Android NDK probe project and build/device preflight; physical device validation remains pending.
- Stage 89: `s3eFileRead` argument contract closure; guest return convention remained unresolved at that time.
- Stage 90: GitHub source repository consolidation.
- Stage 91: DTRZ metadata reconstruction; all 47 entries decoded by the Python oracle.
- Stage 92: native C DTRZ metadata parser and read-only VFS.
- Stage 93: bounded liblzma LZMA-alone callback; all 47 native outputs matched Python byte-for-byte.
- Stage 94: DTRZ VFS connected to the generic guest file adapter; all 47 original archive entries read through the adapter and compared to Stage 93 Python output by size and FNV-1a digest.
- Stage 95: guest file API bridge readiness audit; typed argument capture added for Open/GetChar/Close, while guest import activation remained disabled.
- Stage 96: synthetic Open/Read/GetChar/Close import-dispatch integration test; 28 assertions passed; production authorization gates remained disabled.
- Stage 97: reopened the original APK; decoded the real XE3U image; parsed 227 symbols and 238 external relocations; confirmed all four file API symbol cells; found 35 `s3eFileRead` callsites; established that its guest return value is an item count.

## Current gates

1. Complete exact `s3eFileRead` partial-read/error semantics and handle lifecycle.
2. Implement backend-backed guest file handlers using the now-established item-count return unit, but keep production authorization disabled until all edge cases are covered.
3. Build liblzma for Android ARM64 and complete an actual Android NDK build.
4. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
5. Expand guest CPU coverage and establish graphics/audio/input lifecycle, then observe an original game boot.

## Next: Stage 98 — Guest File Bridge Contract Implementation

- Implement host-side guest `Open/Read/GetChar/Close` adapters against the existing DTRZ-backed file backend.
- Return complete item counts from `s3eFileRead`; do not return byte counts.
- Test full read, EOF, short read, partial final item, missing path, invalid/stale handle, repeated close, and cleanup.
- Preserve explicit guest-memory bounds checks and handle-token ownership.
- Keep production import registration disabled until each behavior is independently validated.

The original APK, `biohazard.s3e`, and decoded game binary are not distributed in this repository. The read-only audit can be repeated with a user-supplied APK using `tools/s3e_binary_audit.py`.
