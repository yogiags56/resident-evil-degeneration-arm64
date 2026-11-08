# Roadmap

## Completed foundations
- Stages 1–69: executable/container analysis, guest memory, ARM/Thumb interpreter expansion, relocation and control-flow research.
- Stages 70–73: import ABI audit, guest dispatcher/trampoline infrastructure, verified OpenGL parameter adapters.
- Stages 74–81: memory/file API tracing and ABI callsite investigation.
- Stage 82: platform-neutral file adapter.
- Stage 83: original APK asset inventory and prefix validation.
- Stage 84: original path strings and file-read semantics audit.
- Stage 85: Android `AAssetManager` backend foundation.
- Stage 86: JNI AssetManager lifetime bridge.
- Stage 87: Android NDK asset probe harness.
- Stage 88: build preflight and device-validation guide.
- Stage 89: `s3eFileRead` argument capture/validation; guest return remains quarantined.
- Stage 90: GitHub-ready source layout, consolidated Stage73 guest runtime core, and conservative DTRZ metadata-boundary inspector.

## Next: Stage 91 — DTRZ parser correlation
1. Trace guest routines that consume the DTRZ metadata region.
2. Recover candidate record widths and field meanings from instruction-level evidence.
3. Correlate metadata references with actual payload starts and known asset signatures.
4. Validate the same candidate schema against several independent entries.
5. Only then implement a bounded payload reader/decompressor.

## Later
- Stage 92+: prove archive entry lookup and payload extraction.
- Android NDK/device build and AAssetManager runtime validation.
- Close `s3eFileRead` return/short-read semantics.
- Activate only evidence-backed file import handlers.
- Expand guest CPU instruction coverage based on observed execution.
- Integrate renderer, input, audio, lifecycle and boot sequence.
- Real-device original-game boot and gameplay verification.

## Definition of playable
The project is not playable until the original game reaches interactive gameplay on a real ARM64 Android device, with the result reproducibly documented.
