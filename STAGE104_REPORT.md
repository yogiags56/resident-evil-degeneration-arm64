# Stage 104 — liblzma CMake Integration & ARM64 Archive Verification

## Objective

Stage 104 validates the exact CMake integration path used by the Android native target and adds a guard against accidentally linking a host-architecture liblzma archive into the Android ARM64 build.

## Changes

- Added `tests/cmake_liblzma_smoke/CMakeLists.txt`, which uses CMake `find_package(LibLZMA REQUIRED)` and the imported target `LibLZMA::LibLZMA` to compile the production `runtime/archive/re_dtrz_lzma.c`.
- Added `tools/run_host_liblzma_cmake_smoke.sh`.
- The host CMake smoke test successfully found liblzma 5.8.1, built the production decoder, linked the imported target, and ran all six decoder scenarios.
- Added `tools/verify_android_liblzma.sh`, which inspects a member object inside `liblzma.a` and requires ELF64 + AArch64.
- The liblzma cross-build and Android app build scripts now invoke the archive architecture verifier before proceeding.
- Added four Python tests for the CMake and architecture-verification contracts.

## Validation evidence

- Python unit tests: 35 passed.
- Native C host mock syntax checks: passed.
- Existing C regression suite through Stage 99: passed.
- Host CMake + real system liblzma integration: passed; 6 decoder scenarios.
- Negative architecture test: supplied a host x86_64 liblzma archive to the Android archive verifier; it correctly rejected it with `liblzma member is not AArch64`.
- Shell syntax checks: passed for all build and verification scripts.

## Environment boundary

The host CMake smoke test uses the machine's x86_64 liblzma 5.8.1. This proves the CMake imported-target wiring and production decoder compile/run on the host; it does NOT prove Android ARM64 compatibility.

Android NDK, XZ Utils source, and `ANDROID_LIBLZMA_ROOT` are still unavailable. Consequently:
- No ARM64 liblzma archive has been produced.
- No Android CMake/Gradle build has been run.
- No ELF64/AArch64 application shared library or debug APK has been produced.
- No ARM64 device runtime or original game boot is claimed.

## Stage 105

1. Obtain Android NDK and XZ Utils source.
2. Build Android `arm64-v8a` liblzma.
3. Run `verify_android_liblzma.sh` and confirm AArch64.
4. Run Android CMake/Gradle build.
5. Verify native library ELF64/AArch64 and JNI exports.
6. Package staged game assets and test on ARM64 Android.
