# Roadmap Through Stage 110

## Current state

Stage 110 is complete. SDK package installation now prefers Google's official Android CLI, with deprecated `sdkmanager` retained only as a compatibility fallback.

## Completed milestones

- Stages 1–100: reverse-engineering foundation, DTRZ inspection/VFS/decoder, guest file adapter and dispatcher tests, XE3U symbol/relocation audit, and Android ARM64 build planning.
- Stage 101: local-only APK asset staging with allowlist and SHA-256 manifest.
- Stage 102: Android build orchestration and ELF64/AArch64 verifier.
- Stage 103: pinned XZ/liblzma Android cross-build automation.
- Stage 104: host CMake integration smoke and AArch64 archive-member verifier.
- Stage 105: Android toolchain doctor.
- Stage 106: pinned Android/Gradle/XZ toolchain contract.
- Stage 107: pinned XZ source fetcher and SDK package installer.
- Stage 108: standalone offline XZ archive verifier, path/link traversal checks, five security/identity fixture tests, and `sdkmanager` doctor check.
- Stage 109: strict Android SDK/NDK package verifier and installer integration.
- Stage 110: migration to official Android CLI package management; `sdkmanager` now fallback only; fake-CLI integration test validates exact package requests and post-install verification; ARM64 liblzma verifier now checks every archive object member, with tests that reject a bad second member.

## Validation snapshot

- Python suite: 62 tests passed. Full host C regression suite, six host LZMA smoke scenarios, and all 14 shell syntax checks passed.
- Host C regression and host liblzma smoke suites remain part of the validation gate.
- Real Android SDK/NDK provisioning is still unavailable in this execution environment.
- No Android ARM64 liblzma, ELF, APK, device runtime, or playable game boot is claimed.

## Next: Stage 111 — Provisioned ARM64 liblzma Build

1. Install the official Android CLI for the build host and place `android` on `PATH`.
2. Set `ANDROID_SDK_ROOT` and run `tools/install_android_sdk_packages.sh`.
3. Provision the pinned Gradle 8.9 wrapper.
4. Fetch XZ Utils 5.8.4 and verify its locked SHA-256.
5. Build static/PIC liblzma for `arm64-v8a`, API 23.
6. Verify all archive members are ELF64/AArch64.
7. Attempt the Android app build only after all preflight gates pass.
8. Keep proprietary APK/assets out of the source repository and guest imports quarantined until their semantics are proven.
