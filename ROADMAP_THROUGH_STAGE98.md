# Roadmap through Stage 98

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
- Stage 95: guest file API bridge readiness audit; typed argument capture added for Open/GetChar/Close.
- Stage 96: synthetic Open/Read/GetChar/Close import-dispatch integration test; 28 assertions passed.
- Stage 97: original APK reopened; XE3U fixups parsed; 35 `s3eFileRead` callsites recovered; guest return established as item count.
- Stage 98: backend-backed host-side Open/Read/GetChar/Close bridge added; item-count results, EOF, partial-item count, handle invalidation, and cleanup tested; 48 assertions passed.

## Current gates

1. Finish exact `s3eFileRead` partial-item and error semantics.
2. Test Stage 98 handlers through the AAPCS32 dispatcher against the DTRZ-backed backend.
3. Bind actual symbol indices only after the original symbol table mapping is reproduced in the integration test.
4. Build liblzma for Android ARM64 and complete an actual Android NDK build.
5. Validate AssetManager -> DTRZ VFS -> file adapter -> guest file bridge on a physical ARM64 device.
6. Expand guest CPU coverage and establish graphics/audio/input lifecycle, then observe an original game boot.

## Next: Stage 99 — Guest Dispatcher + DTRZ End-to-End Integration

- Use the real Stage 98 handler functions in a synthetic AAPCS32 dispatcher test.
- Drive Open/Read/GetChar/Close through the guest dispatcher, not only direct host calls.
- Mount a synthetic DTRZ archive and read a known entry through the entire stack.
- Verify guest r0 item counts, bytes written to guest memory, EOF, invalid handles, and cleanup.
- Keep production guest imports disabled pending exact runtime error/partial-read evidence.
