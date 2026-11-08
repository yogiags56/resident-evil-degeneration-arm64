# Roadmap through Stage 94

- Stages 1–73: original executable/JNI/loader analysis, partial ARM32 guest CPU, import dispatch infrastructure, and two individually verified GL import adapters.
- Stages 74–81: file/memory import audits, file API ABI and callsite tracing.
- Stage 82: platform-neutral guest file adapter foundation.
- Stage 83: original APK asset inventory and real-asset prefix validation.
- Stage 84: original path strings and file-read semantics investigation.
- Stage 85: Android `AAssetManager` backend foundation.
- Stage 86: JNI AssetManager lifecycle bridge.
- Stages 87–88: Android NDK probe project and build/device preflight. Physical device validation remains pending.
- Stage 89: `s3eFileRead` argument contract closure. Guest return convention remains unresolved.
- Stage 90: GitHub source repository consolidation.
- Stage 91: DTRZ metadata reconstruction; all 47 entries decoded by the Python oracle.
- Stage 92: native C DTRZ metadata parser and read-only VFS.
- Stage 93: bounded liblzma LZMA-alone callback; all 47 native outputs matched Python byte-for-byte.
- Stage 94: DTRZ VFS connected to the generic guest file adapter; all 47 original archive entries read through the adapter and compared to Stage 93 Python output by size and FNV-1a digest.

## Next: Stage 95 — Guest File API Bridge Readiness Audit

- Revisit original `s3eFileOpen`, `s3eFileRead`, `s3eFileGetChar`, and `s3eFileClose` callsites.
- Establish `s3eFileRead` return behavior from stronger callee/caller evidence or a controlled original-runtime observation.
- Design typed wrappers without patching guest import slots until all signatures and return semantics are proven.
- Add guest-memory and handle-lifecycle tests for each proven operation.
- Keep unresolved functions quarantined.

## Not yet achieved

- Android NDK build with Android-ABI liblzma.
- Physical-device AssetManager/VFS test.
- Proven guest `s3eFileRead` return semantics.
- Guest file import handler activation.
- Original game boot or gameplay.
