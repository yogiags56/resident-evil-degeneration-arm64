# Stage 106 — Reproducible Android Toolchain Lock

## Objective

Convert the previous build-readiness checks into a reproducible version contract. The Android build must not silently drift between NDK, Gradle, Android SDK packages, CMake, and the liblzma dependency.

## Pinned toolchain

| Component | Pinned version |
|---|---:|
| Android NDK | r30 / `30.0.16248370` |
| Android ABI | `arm64-v8a` |
| Android native minimum API | 23 |
| Android compile SDK | 35 |
| Android Build Tools | `35.0.0` |
| Android SDK CMake | `3.22.1` |
| Android Gradle Plugin | `8.7.3` |
| Gradle Wrapper distribution | `8.9-bin` |
| Java runtime minimum | 17 |
| XZ Utils / liblzma source | `5.8.4` |

The versions are recorded in `config/android-toolchain.lock.json`. The app Gradle configuration now pins NDK and Build Tools. The Gradle wrapper properties file pins the official Gradle 8.9 binary distribution and its SHA-256 checksum.

## Changes

- Added a single machine-readable toolchain lock file.
- Pinned `ndkVersion` to `30.0.16248370` and Build Tools to `35.0.0`.
- Added a pinned Gradle 8.9 wrapper distribution URL and SHA-256.
- Added `tools/provision_gradle_wrapper.sh`. It requires a system Gradle 8.9+ only once, generates wrapper scripts/JAR from an isolated minimal bootstrap project, then pins the wrapper to Gradle 8.9 and its official checksum.
- Updated the Android build script to use the wrapper provisioner rather than generating the wrapper from the application project.
- Pinned the liblzma source build to XZ Utils 5.8.4 by reading its real `src/liblzma/api/lzma/version.h` version macros. A negative test rejects 5.8.3.
- Audited the upstream XZ Utils 5.8.4 CMake project and corrected the cross-build flags to disable its actual command-line tool targets and NLS/docs options, rather than relying on obsolete option names.
- Downgraded `CMakePresets.json` schema from version 6 to version 3 so the preset format remains compatible with the pinned Android SDK CMake 3.22.1; `cmake --list-presets` recognizes the Android ARM64 preset.
- Expanded the toolchain doctor to verify Android SDK Platform 35, Build Tools 35.0.0, SDK CMake 3.22.1, NDK revision, Gradle, and Java 17+.
- Added seven Stage 106 tests, including a negative test proving that the liblzma builder rejects an XZ 5.8.3 source tree.

## Validation

- Python unit tests: **45 passed**.
- Native C host mock-header syntax checks: passed.
- Host C regression suite through Stage 99: passed.
- Host CMake + system liblzma integration: passed; 6 decoder scenarios.
- Shell syntax checks for all changed build/doctor scripts: passed.
- CMake preset parsing with the host CMake 3.31.6: passed.
- Negative XZ version-gate test: passed.
- Android toolchain doctor: correctly reports **BLOCKED** in this environment.

## Environment boundary

The execution environment still does not contain the Android NDK, Android SDK Platform 35, Build Tools 35.0.0, SDK CMake 3.22.1, Gradle, or an Android ARM64 liblzma prefix. The XZ Utils 5.8.4 source package was not downloaded or built in this stage; the version-header layout and CMake options were checked against the upstream 5.8.4 source files. Therefore:
- No Android ARM64 liblzma archive was produced.
- No Gradle/Android CMake build was run.
- No ELF64/AArch64 application library or APK was produced.
- No device runtime or original game boot is claimed.

The host liblzma smoke test uses host x86_64 liblzma 5.8.1 and is not Android ABI evidence.

## Next stage — Stage 107

1. Provision the pinned Android SDK/NDK and SDK packages.
2. Provision Gradle 8.9 wrapper using the new isolated bootstrap script.
3. Obtain and verify the official XZ Utils 5.8.4 source package.
4. Build static liblzma for Android `arm64-v8a`, API 23.
5. Require all toolchain doctor gates to pass.
6. Run the first Android Gradle build and verify ELF64/AArch64/JNI exports.
