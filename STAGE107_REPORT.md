# Stage 107 — Dependency Acquisition & SDK Provisioning Automation

## Objective

Prepare a reproducible route to provision the pinned Android SDK/NDK and obtain the exact XZ Utils source archive required for the Android ARM64 liblzma cross-build.

## Verified upstream release metadata

- XZ Utils 5.8.4 source archive: `xz-5.8.4.tar.gz`
- Expected SHA-256: `0014c7886930454fe8bd4228665b51af55eeae560ea135c9c4cd33f55b2591d9`
- Release date: 2026-09-09
- Official release page states 5.8.4 fixes GHSA-5qpq-xqfv-j9pg, an invalid memory write after decoder reinitialization following allocation failure.
- Android NDK r30 package revision: `30.0.16248370`
- Linux NDK archive size: 738,633,529 bytes
- Linux NDK published SHA-1: `5107f898313790e449e87eee2183d9a20602dee9`

The metadata is recorded in `config/android-toolchain.lock.json`.

## Changes

- Added `tools/fetch_xz_source.sh`:
  - Downloads only the pinned XZ Utils 5.8.4 release archive.
  - Uses the official GitHub release and SourceForge mirror.
  - Verifies the archive SHA-256 before extraction.
  - Checks tar entry paths before extraction to reject absolute paths and `..` traversal.
  - Checks the extracted `version.h` macros equal 5.8.4.
  - Keeps downloaded source and cache under ignored `.deps/`.
- Updated `tools/build_liblzma_android_arm64.sh` to fetch and verify the pinned XZ source automatically when `XZ_SOURCE_DIR` is not supplied.
- Added `tools/install_android_sdk_packages.sh` to present Android SDK license prompts and install the pinned Platform Tools, SDK Platform 35, Build Tools 35.0.0, CMake 3.22.1, and NDK r30.
- Expanded the lock file with XZ archive SHA-256 and NDK Linux package metadata.
- Added unit tests for source checksum pinning, safe extraction gates, SDK package selection, and a clear failure when `sdkmanager` is absent.

## Validation

- Python unit tests: **48 passed**.
- Native C host mock-header syntax checks: passed.
- Host C regression suite through Stage 99: passed.
- Host CMake + system liblzma integration: passed; 6 decoder scenarios.
- Shell syntax checks: passed.
- CMake preset parsing: passed.
- Android SDK installer correctly refuses to proceed when `sdkmanager` is absent.

## Environment boundary

This environment still has no Android SDK command-line tools, SDK Platform 35, Build Tools 35.0.0, Android SDK CMake 3.22.1, Android NDK r30, Gradle, or Android ARM64 liblzma prefix.

The XZ release archive was identified and its upstream SHA-256 independently checked against the release listing, but the archive download itself could not be completed in this execution environment. Consequently:
- XZ source was not downloaded or extracted here.
- Android NDK was not installed.
- No ARM64 liblzma archive was built.
- No Android Gradle/CMake build or APK was produced.
- No device runtime or game boot is claimed.

The host liblzma smoke test remains a host x86_64 test and is not evidence of Android ABI output.

## Next stage — Stage 108

1. Run the SDK package installer in an environment with Android command-line tools and accept SDK licenses.
2. Download and verify XZ Utils 5.8.4 using the new source-fetch script.
3. Generate Gradle Wrapper 8.9.
4. Run the toolchain doctor and require all checks to pass.
5. Cross-compile liblzma for `arm64-v8a`, API 23.
6. Verify the static archive's object members are ELF64/AArch64.
7. Only then run the first Android Gradle build.
