# Stage 110 Report — Android CLI Migration + ARM64 liblzma Build Gate

## Delivered

- Updated `tools/install_android_sdk_packages.sh` to prefer the official `android` CLI and its `android sdk install` subcommand.
- Kept `sdkmanager` only as a clearly labelled temporary compatibility fallback.
- Pinned requested SDK packages in Android CLI syntax: `platforms/android-35`, `build-tools@35.0.0`, `cmake@3.22.1`, and `ndk@30.0.16248370`, plus platform-tools.
- Updated Android Toolchain Doctor to accept the Android CLI or the legacy fallback.
- Added an integration test using a fake Android CLI. It verifies the exact requested package set, synthetic SDK/NDK layout, and post-install validation.
- Hardened `tools/verify_android_liblzma.sh`: it now inspects every object member reported by `llvm-ar t`, rather than checking only the first member. It rejects an archive if any member is empty, not ELF64, or not AArch64.
- Added two mocked archive tests proving a good two-member archive passes and a bad second member is rejected.
- Updated README with official Android CLI installation guidance for Linux x86-64 and ARM64.

## Official platform notes

Google's current Android documentation marks `sdkmanager` deprecated and recommends `android sdk`; package names use slash-separated paths and optional `@version` selectors. [SDK manager migration reference](https://developer.android.com/tools/sdkmanager) [Android CLI SDK command reference](https://developer.android.com/tools/agents/android-cli/commands/sdk).

Google provides official Android CLI installers for Linux x86-64 and Linux ARM64. [Android CLI download](https://developer.android.com/tools/agents/android-cli/download).

NDK r30 `30.0.16248370` remains the latest LTS version documented by Android, with Linux archive size and SHA-1 pinned in the project lock. [NDK downloads](https://developer.android.com/ndk/downloads/).

## Validation

- Python suite: **62 tests passed**.
- `bash tools/run_tests.sh`: passed; host C syntax checks and all historical C regression suites (Stages 60–99) passed.
- Host CMake/liblzma smoke: **6 decoder scenarios passed** using host liblzma; this is not an Android ARM64 build.
- All 14 shell scripts passed `bash -n`.
- Android Toolchain Doctor JSON parses and reports `BLOCKED` with 14 checks; Android CLI is not installed in this environment.
- Four new Stage 110 tests cover Android CLI installation preference, package pinning, fake-CLI provisioning, and all-member ELF architecture verification.


## Environment limitations

This environment still has no configured Android SDK/NDK, Android CLI, Gradle 8.9, or ARM64 liblzma prefix. The pinned XZ Utils source archive has not been downloaded. Therefore no real Android SDK installation, ARM64 liblzma compilation, Android ELF, APK build, device execution, or game boot is claimed.

## Next

Stage 111 should run Android CLI installation and package provisioning in a network-enabled Linux environment, provision Gradle 8.9, fetch and verify XZ Utils 5.8.4, build liblzma for `arm64-v8a` API 23, and validate all static archive members as ELF64/AArch64.
