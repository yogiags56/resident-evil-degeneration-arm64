# Stage 103 — ARM64 liblzma Dependency Build Automation

## Scope

Stage 103 addresses a concrete missing prerequisite from Stage 102: there was no documented, reproducible path to produce an Android `arm64-v8a` liblzma dependency. A host liblzma library cannot be substituted because it has the wrong ABI.

## Implementation

- Added `tools/build_liblzma_android_arm64.sh`.
- The script cross-compiles XZ Utils/liblzma with the Android NDK CMake toolchain, `arm64-v8a`, Android API 23, static linking, and PIC.
- It validates the installed `lzma.h` and `liblzma.a`.
- Improved `tools/build_android_arm64.sh` to use an existing Gradle wrapper or generate one with system Gradle 8.9+.
- Improved NDK preflight checks for host-specific LLVM clang presence.
- Added dependency build documentation.

## Environment audit

The current environment has:
- CMake and Ninja available.
- Java/Javac available.
- No Android NDK in the standard checked locations.
- `ANDROID_NDK_HOME` unset.
- `ANDROID_LIBLZMA_ROOT` unset.
- `XZ_SOURCE_DIR` unset.
- No system Gradle, `sdkmanager`, or `adb` available.
- No Gradle wrapper in the repository.

Therefore the cross-compile cannot be executed in this environment. No Android liblzma artifact, Android shared library, APK, or device test is claimed.

## Validation

- Python unit tests: 27 passed.
- Native C host mock syntax checks: passed.
- C regression tests through Stage 99: passed.
- Shell syntax checks: passed for preflight, Android build, liblzma build, and ELF verifier.
- Python asset staging tool syntax: passed.
- Android NDK / liblzma cross-build: blocked by missing NDK and XZ source.

## Stage 104

1. Obtain Android NDK and XZ Utils source tree.
2. Run the new liblzma ARM64 cross-build script.
3. Confirm `liblzma.a` machine architecture is AArch64 and PIC-compatible.
4. Run Android CMake/Gradle build.
5. Inspect resulting JNI shared library and LZMA linkage.
6. Package staged APK assets and run on ARM64 Android.
