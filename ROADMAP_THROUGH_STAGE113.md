# Roadmap Through Stage 113

## Completed source/runtime groundwork

- Stages 94–99: DTRZ virtual filesystem, host LZMA integration, guest file bridge, AAPCS32 synthetic dispatch tests, and archive equivalence checks.
- Stages 100–107: Android ARM64 build configuration, APK asset staging, toolchain checks, pinned Android/Gradle/XZ versions, and provisioning scripts.
- Stages 108–111: fetcher security tests, SDK/NDK verification, Android CLI provisioning, pinned Gradle installer, and initial GitHub Actions workflow.
- Stage 112: CI artifact collector, proprietary-asset exclusion, manifest, and SHA-256 output.
- Stage 113: persistent NDK environment handoff between CI steps, exact NDK revision gate, and APK-vs-standalone native library hash equality.

## Current state

Host-side validation and synthetic tests are available. A real Android build has not yet been completed because no connected GitHub repository or local Android SDK/NDK/Gradle installation is available in this execution environment. The CI APK remains explicitly classified as bootstrap/probe-only, not a playable game.

## Next milestones

1. Stage 114: execute the workflow on a network-enabled GitHub Actions runner or equivalent Linux runner.
2. Resolve real provisioning, Gradle, CMake, and NDK integration failures.
3. Verify the full `liblzma.a` archive and native `.so` are AArch64.
4. Build and inspect the bootstrap APK; record logs, hashes, and manifest.
5. Continue local asset-enabled integration while keeping original game assets out of the public source repository.
6. Only after runtime imports and asset handling are proven, proceed to device/emulator boot and gameplay validation.
