# Stage 102 — Android ARM64 Build Orchestration + ELF Verification

## Objective

Prepare a reproducible Android ARM64 build path and a post-build verifier. This stage does not claim an Android binary was built: the environment has no configured Android NDK, no Android `arm64-v8a` liblzma prefix, and no Gradle wrapper.

## Changes

- Gradle native configuration now forces `RE_ENABLE_LIBLZMA=ON`, so an Android Gradle build cannot silently produce a probe without the LZMA decoder.
- Gradle passes `ANDROID_LIBLZMA_ROOT` through to CMake's `LibLZMA_ROOT`.
- Added `tools/build_android_arm64.sh` as the single build entry point.
- Added `tools/verify_android_arm64_elf.sh` to validate the produced shared library:
  - ELF class must be ELF64.
  - machine must be AArch64.
  - JNI export `Java_org_example_redstage_MainActivity_nativeProbe` must exist in the dynamic symbol table.
- Kept ABI filter restricted to `arm64-v8a`.
- Updated app version to `0.1.0-stage102`.

## Environment findings

- CMake 3.31.6: available.
- Ninja: available.
- Android NDK: unavailable / `ANDROID_NDK_HOME` unset.
- Android liblzma prefix: unavailable / `ANDROID_LIBLZMA_ROOT` unset.
- Gradle wrapper: absent from checkout.
- `llvm-readelf` on PATH: unavailable; the verifier can locate the copy bundled in Android NDK or accept an explicit readelf path.

The preflight was executed and correctly stopped with exit code 2, reporting the missing NDK and liblzma prefix. This is an environment prerequisite block, not a compiler diagnostic.

## Validation

- Python tests: 27 passed.
- Native C host mock syntax checks: passed.
- Existing C regression tests through Stage 99: passed.
- Shell syntax checks for preflight, build, and ELF verifier: passed.
- Python syntax check for asset staging tool: passed.
- Android CMake configure/build: not run.
- APK packaging / ARM64 device runtime: not run.
- ELF64/AArch64 verification: not run because no Android shared library exists.

## Reproduction prerequisites

1. Install Android NDK with CMake toolchain.
2. Build/install liblzma for Android `arm64-v8a`; its prefix must contain `lzma.h` and `liblzma.a` or `liblzma.so`.
3. Provide Gradle 8.9+ (the repository currently has no Gradle wrapper).
4. Stage original APK assets using `tools/stage_android_assets.py`.
5. Set `ANDROID_NDK_HOME` and `ANDROID_LIBLZMA_ROOT`.
6. Run `tools/build_android_arm64.sh`.

## Stage 103

- Resolve NDK, Gradle, and ARM64 liblzma prerequisites.
- Run Android CMake configure/build and resolve actual compiler/linker diagnostics.
- Inspect ELF headers, dynamic JNI exports, and LZMA linkage.
- Assemble debug APK and test required assets on an ARM64 device/emulator.
- Continue Marmalade runtime import semantics research; production guest imports remain disabled.
