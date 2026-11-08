# Current Status

**Current source milestone: Stage 118 implementation committed; CI pending.** Stage 117 integrated the partial ARM32/Thumb guest CPU, AAPCS32 dispatcher, runtime coordinator, and ABI support into the Android ARM64 native target. The Android ARM64 workflow passed on commit `b090f90`: SDK/NDK provisioning, ARM64 liblzma compilation, APK assembly, ELF verification, artifact collection, and upload. Source Checks passed on the same commit.

Stage 118 adds guest runtime stop diagnostics: fault PC, ARM/Thumb state, CPU status, import status, and stop reason. Tests cover unresolved imports, import-dispatch failure due to invalid guest stack alignment, and guest instruction fetch outside mapped memory. The Stage 118 report documents the forward roadmap and outstanding research. Its CI validation is pending.

**Still blocked / unverified:** full ARM/Thumb instruction coverage; XE3U loader and relocation engine; guest address-space map; production file-import activation and exact partial-read/error behavior; complete S3E import table; renderer/audio/input; physical-device execution; original game boot/gameplay. The original APK and game executable are not distributed in the repository.

See [Stage 118 report](STAGE118_RUNTIME_DIAGNOSTICS_AND_ROADMAP.md).
