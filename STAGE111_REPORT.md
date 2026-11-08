# Stage 111 Report — Reproducible Gradle Bootstrap + Android ARM64 CI

## Delivered

- Added `tools/install_gradle_distribution.sh`, which reads Gradle 8.9 and its SHA-256 from the project lock, downloads the official binary distribution, then invokes an independent verifier.
- Added `tools/verify_gradle_distribution.py`. It validates the pinned SHA-256, rejects ZIP path traversal and symbolic links, requires the Gradle executable, safely extracts the distribution, and sets executable permission.
- Added five offline Gradle distribution tests, including valid extraction, checksum mismatch, and ZIP traversal rejection.
- Added `.github/workflows/android-arm64.yml`, a GitHub Actions workflow for Ubuntu 24.04. It installs Java 17, official Android CLI, pinned SDK/NDK packages, Gradle 8.9, XZ Utils 5.8.4, builds and verifies ARM64 liblzma, builds the Android app, verifies the native ELF, and uploads build artifacts.
- Updated the app version label to `0.1.0-stage111`.
- The CI workflow deliberately does not stage or embed the user's original APK/assets. Those remain local-only inputs.

## Android CLI package syntax

Google's current command reference defines `android sdk install <package>[@<version>]`, slash-separated SDK package paths, and version selectors after `@`. The workflow uses that documented syntax for Platform 35, Build Tools 35.0.0, CMake 3.22.1, and NDK 30.0.16248370. [Android CLI SDK install reference](https://developer.android.com/tools/agents/android-cli/commands/sdk_install) [Android CLI SDK command](https://developer.android.com/tools/agents/android-cli/commands/sdk).

The official Android CLI download page provides the Linux x86-64 installer used by CI. [Android CLI download](https://developer.android.com/tools/agents/android-cli/download).

## Validation

- Python suite: **69 tests passed**.
- `bash tools/run_tests.sh`: passed, including native C syntax checks and all historical C regression suites (Stages 60–99).
- Host CMake/liblzma smoke: **6 decoder scenarios passed** using host liblzma; this is not an Android ARM64 build.
- All **15 shell scripts** passed `bash -n`.
- Android Toolchain Doctor JSON parsed successfully and reports `BLOCKED` with 14 checks; the Android CLI is absent.
- Five new Gradle distribution tests cover lock usage, valid extraction, executable permissions, checksum mismatch, and ZIP path traversal.
- Two CI workflow tests verify provisioning/build order and ensure original proprietary APK assets are not embedded in the workflow.


## Environment limitations

The current execution environment has Java, CMake, Ninja, curl, and host LLVM utilities, but does not have Android CLI, Android SDK/NDK, Gradle, or an Android ARM64 liblzma prefix. Network downloads are not assumed to have succeeded. Therefore this stage does not claim an actual Gradle distribution download, real SDK/NDK provisioning, liblzma ARM64 compilation, Android ELF, APK build, device runtime, or game boot.

The GitHub Actions workflow is authored but has not been run on GitHub. Its first real CI run is the next validation point.

## Next

Stage 112 should run the workflow on a network-enabled GitHub runner, fix any integration issues exposed by the real package manager/build tools, and preserve the resulting ARM64 `liblzma.a`, native `.so`, APK, test logs, and hashes as CI artifacts. Production guest imports remain quarantined pending proof of original Marmalade semantics.
