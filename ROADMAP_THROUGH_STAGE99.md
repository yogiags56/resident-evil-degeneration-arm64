# Roadmap through Stage 99

- Stages 1–73: original executable/JNI/loader analysis, partial ARM32 guest CPU, import dispatch infrastructure, and individually verified GL import adapters.
- Stages 74–81: file/memory import audits, file API ABI and callsite tracing.
- Stage 82: platform-neutral guest file adapter foundation.
- Stage 83: original APK asset inventory and real-asset prefix validation.
- Stage 84: original path strings and file-read semantics investigation.
- Stage 85: Android `AAssetManager` backend foundation.
- Stage 86: JNI AssetManager lifecycle bridge.
- Stages 87–88: Android NDK probe project and build/device preflight; physical device validation remains pending.
- Stage 89: `s3eFileRead` argument contract closure; guest return convention unresolved at that point.
- Stage 90: GitHub source repository consolidation.
- Stage 91: DTRZ metadata reconstruction; all 47 entries decoded by the Python oracle.
- Stage 92: native C DTRZ metadata parser and read-only VFS.
- Stage 93: bounded liblzma LZMA-alone callback; all 47 native outputs matched Python byte-for-byte.
- Stage 94: DTRZ VFS connected to the generic guest file adapter; all 47 original archive entries read through the adapter and compared by size and FNV-1a digest.
- Stage 95: guest file API bridge readiness audit; typed argument capture added for Open/GetChar/Close.
- Stage 96: synthetic Open/Read/GetChar/Close import-dispatch integration test; 28 assertions passed.
- Stage 97: original APK reopened; XE3U fixups parsed; 35 `s3eFileRead` callsites recovered; guest return established as item count.
- Stage 98: backend-backed host-side Open/Read/GetChar/Close bridge; 48 assertions passed.
- Stage 99: actual Stage 98 handlers exercised through AAPCS32 dispatcher against a synthetic DTRZ-backed VFS; 31 end-to-end assertions passed.

## Current gates

1. Close exact original `s3eFileRead` partial-item and error semantics.
2. Reproduce original XE3U symbol-index mapping in the runtime loader before binding real import IDs.
3. Build liblzma for Android ARM64 and complete an actual Android NDK build.
4. Validate AssetManager -> DTRZ VFS -> file adapter -> guest dispatcher on a physical ARM64 device.
5. Expand guest CPU coverage and establish graphics/audio/input lifecycle, then observe an original game boot.

## Next: Stage 100 — ABI Evidence Closure + Android ARM64 Build Preparation

- Document or experimentally verify partial-item/error behavior; do not infer undocumented guest error sentinels.
- Audit symbol table parsing and import binding using original XE3U evidence.
- Prepare a reproducible Android ARM64 liblzma build and NDK configuration.
- Keep production guest file imports disabled until each behavior is evidence-backed.
