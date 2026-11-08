# Stage 105 — Android Toolchain Doctor & Build Readiness Gate

## Objective

Make the remaining Android ARM64 build blockers explicit and reproducible before attempting a costly native build. This stage adds a single diagnostic entry point that audits the Android NDK, its host LLVM tools/CMake toolchain, CMake/Ninja, Java/Gradle, and the ARM64 liblzma prefix.

## Changes

- Added `tools/android_toolchain_doctor.sh`.
  - Supports human-readable output and machine-readable `--json`.
  - Checks NDK root and revision metadata (`source.properties`).
  - Checks NDK host-specific Clang, `llvm-ar`, `llvm-readelf`, and Android CMake toolchain.
  - Checks CMake/Ninja, Java, and either repository `gradlew` or system Gradle.
  - Checks for `lzma.h` and `liblzma.a`, then delegates architecture verification to `verify_android_liblzma.sh`.
  - Exits non-zero when the environment is not ready; it does not modify or install the toolchain.
- Added `tests/test_stage105_android_toolchain_doctor.py` with 3 tests for required gates, JSON output, and clear missing-NDK diagnostics.

## Validation

- Python unit tests: **38 passed**.
- Native host mock-header syntax checks: **passed**.
- C regression suite through Stage 99: **passed** (including DTRZ backend, LZMA, guest CPU/runtime, file adapter, bridge, and dispatch tests).
- Host CMake + system liblzma integration: **passed**, 6 decoder scenarios.
- Doctor script Bash syntax: **passed**.
- The doctor was run in this environment and correctly reports **BLOCKED** because Android NDK and `ANDROID_LIBLZMA_ROOT` are not configured; no repository Gradle wrapper or system Gradle is available.

## Environment boundary

This stage is diagnostics and reproducibility work only. It did **not** install an Android SDK/NDK, download XZ Utils, build an Android ARM64 liblzma archive, run Gradle/Android CMake, produce an APK, or test on a device. The host x86_64 liblzma smoke test is not evidence of Android ABI compatibility. No game boot or playable state is claimed.

## Next stage — Stage 106

1. Provision a supported Android SDK/NDK and Gradle 8.9+ (or a pinned Gradle wrapper).
2. Obtain a versioned XZ Utils source tree and build static `arm64-v8a` liblzma for Android API 23.
3. Run `android_toolchain_doctor.sh` until all checks pass.
4. Run the Android ARM64 Gradle build and verify ELF64/AArch64 plus JNI exports.
5. Only after packaging, move to on-device AssetManager/VFS validation.
