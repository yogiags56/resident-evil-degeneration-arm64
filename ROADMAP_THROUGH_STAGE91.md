# Roadmap checkpoint — Stage 91

## Completed
- Stage 82: platform-neutral file adapter.
- Stage 83: original APK asset inventory and prefix validation.
- Stage 84: original path strings and file-read semantics audit.
- Stage 85: Android AssetManager backend foundation.
- Stage 86: JNI AssetManager lifetime bridge.
- Stage 87: Android NDK asset-probe harness.
- Stage 88: build preflight and device-validation guide.
- Stage 89: `s3eFileRead` argument contract capture; return convention remains unresolved.
- Stage 90: GitHub-ready source-only repository consolidation and conservative DTRZ boundary inspector.
- Stage 91: complete DTRZ profile reconstruction for the examined `main.dz`; 47/47 payloads decoded and verified.

## Next
Stage 92 builds a native C DTRZ index and virtual-file layer with a decompression callback. It must remain independent of the guest `s3eFileRead` ABI until that contract is proven.

## Not yet achieved
- Android NDK APK build and physical-device execution.
- Runtime DTRZ VFS integration.
- Verified `s3eFileRead` return/short-read behavior.
- Original game boot or interactive gameplay.
