# Roadmap Through Stage 111

## Current state

Stage 111 is complete at source/automation level. A reproducible Linux CI path now provisions Android SDK/NDK and Gradle, builds pinned XZ Utils/liblzma for ARM64, and attempts the Android app build with ELF verification.

## Completed milestones

- Stages 1–100: reverse-engineering foundation, DTRZ inspection/VFS/decoder, guest file adapter and dispatcher tests, XE3U symbol/relocation audit, and Android ARM64 build planning.
- Stage 101: local-only APK asset staging with allowlist and SHA-256 manifest.
- Stage 102: Android build orchestration and ELF64/AArch64 verifier.
- Stage 103: pinned XZ/liblzma Android cross-build automation.
- Stage 104: host CMake integration smoke and AArch64 archive-member verifier.
- Stage 105: Android toolchain doctor.
- Stage 106: pinned Android/Gradle/XZ toolchain contract.
- Stage 107: pinned XZ source fetcher and SDK package installer.
- Stage 108: standalone offline XZ archive verifier and archive safety tests.
- Stage 109: strict Android SDK/NDK package verifier and installer integration.
- Stage 110: Android CLI migration and all-member liblzma archive architecture verification.
- Stage 111: checksum-verified Gradle 8.9 installer, safe Gradle ZIP extractor, and GitHub Actions ARM64 build workflow.

## Validation snapshot

- Python suite: 69 tests passed; host C regression suite, six host LZMA smoke scenarios, and all 15 shell syntax checks passed.
- Full host C regression suite and host liblzma smoke remain required.
- The current workspace has no Android CLI, SDK/NDK, Gradle, or Android ARM64 liblzma prefix.
- The GitHub Actions workflow has not yet been run on GitHub.
- No Android ARM64 library, ELF, APK, device runtime, or playable game boot is claimed.

## Next: Stage 112 — First Real CI Build

1. Run `.github/workflows/android-arm64.yml` on a network-enabled Ubuntu 24.04 runner.
2. Confirm official Android CLI package installation and pinned NDK revision.
3. Confirm Gradle 8.9 checksum verification and wrapper generation.
4. Confirm XZ Utils 5.8.4 checksum verification.
5. Build `liblzma.a` and verify every member as ELF64/AArch64.
6. Build the Android app and verify `libre_asset_probe.so` and APK contents.
7. Save CI logs, output hashes, and artifacts; address any real-world integration failures.
8. Keep original APK assets local-only and production guest imports quarantined.
