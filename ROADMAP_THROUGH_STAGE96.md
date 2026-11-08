# Roadmap through Stage 96

- Stages 1–73: original executable/JNI/loader analysis, partial ARM32 guest CPU, import dispatch infrastructure, and individually verified GL import adapters.
- Stages 74–81: file/memory import audits, file API ABI and callsite tracing.
- Stage 82: platform-neutral guest file adapter foundation.
- Stage 83: original APK asset inventory and real-asset prefix validation.
- Stage 84: original path strings and file-read semantics investigation.
- Stage 85: Android `AAssetManager` backend foundation.
- Stage 86: JNI AssetManager lifecycle bridge.
- Stages 87–88: Android NDK probe project and build/device preflight; physical device validation remains pending.
- Stage 89: `s3eFileRead` argument contract closure; guest return convention remains unresolved.
- Stage 90: GitHub source repository consolidation.
- Stage 91: DTRZ metadata reconstruction; all 47 entries decoded by the Python oracle.
- Stage 92: native C DTRZ metadata parser and read-only VFS.
- Stage 93: bounded liblzma LZMA-alone callback; all 47 native outputs matched Python byte-for-byte.
- Stage 94: DTRZ VFS connected to the generic guest file adapter; all 47 original archive entries read through the adapter and compared to Stage 93 Python output by size and FNV-1a digest.
- Stage 95: guest file API bridge readiness audit; typed argument capture added for Open/GetChar/Close, while guest import activation remains disabled.
- Stage 96: synthetic Open/Read/GetChar/Close import-dispatch integration test; 28 assertions pass; production authorization gates remain disabled.

## Current gates

1. Recover exact SDK/runtime semantics for all guest file APIs, especially `s3eFileRead` return and short-read behavior.
2. Re-audit the original decompressed guest image and callsites; the original APK/guest binary is not included in the source repository.
3. Implement and test backend-backed guest file handlers only after contracts are evidenced.
4. Build liblzma for Android ARM64 and complete an actual Android NDK build.
5. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
6. Expand guest CPU coverage and establish graphics/audio/input lifecycle, then observe an original game boot.

## Next: Stage 97 — Original Runtime Evidence and File Handle Lifecycle

- Acquire the original APK or decompressed `biohazard.s3e` and repeat xref analysis.
- Recover a version-matched Marmalade SDK header/reference or capture controlled original-runtime traces.
- Test full/partial reads, EOF, missing paths, invalid/stale handles, repeated close, and cleanup against the original semantics.
- Keep production imports disabled until each contract is independently closed.
