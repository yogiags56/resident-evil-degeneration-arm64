# Roadmap through Stage 95

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
- Stage 95: guest file API bridge readiness audit; typed argument capture added for Open/GetChar/Close, while all guest file import activation remains disabled.

## Next: Stage 96 — Original API Contract Recovery and Synthetic Bridge Tests

- Seek an authoritative Marmalade SDK reference matching the game's SDK generation or controlled observations from the original 32-bit runtime.
- Revisit all `s3eFileRead` callers with the original decompressed guest image; prove return use, short-read behavior, and error handling.
- Test full read, partial read, EOF, missing path, invalid/stale handle, repeated close, and close cleanup.
- Build synthetic import-dispatch tests for each API only after its return and lifecycle contract is evidence-backed.
- Keep production import binding disabled until the contract is proven.

## Not yet achieved

- Exact guest `s3eFileRead` return/short-read semantics.
- Android NDK build with Android-ABI liblzma.
- Physical-device AssetManager/VFS test.
- Guest file import activation.
- Original game boot or gameplay.
