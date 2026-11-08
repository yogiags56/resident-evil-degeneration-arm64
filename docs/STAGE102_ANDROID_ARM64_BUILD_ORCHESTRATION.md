# Stage 102 — Android ARM64 build orchestration

The Gradle Android native configuration now requires LZMA decoding (`RE_ENABLE_LIBLZMA=ON`) and forwards `ANDROID_LIBLZMA_ROOT` to CMake. `tools/build_android_arm64.sh` is the intended one-command build entry point. After Gradle creates the ARM64 shared library, `tools/verify_android_arm64_elf.sh` checks ELF64, AArch64, and the JNI nativeProbe export.

Required local prerequisites:
- Android NDK (`ANDROID_NDK_HOME`)
- liblzma built for `arm64-v8a` (`ANDROID_LIBLZMA_ROOT`, containing `lzma.h` and `liblzma.a` or `.so`)
- Gradle 8.9 or newer (no wrapper is currently committed)
- original APK assets staged locally with `tools/stage_android_assets.py`

The current environment has no NDK, no Android liblzma prefix, and no Gradle wrapper. Consequently no Android binary or APK was produced in Stage 102. See `STAGE102_REPORT.md`.
