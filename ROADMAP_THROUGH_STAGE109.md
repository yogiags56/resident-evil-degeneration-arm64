# Roadmap Through Stage 108

## Current state

Stage 108 is complete. The source repository now includes a standalone XZ archive verifier, an XZ fetcher that invokes it before extraction, and an Android toolchain doctor that also checks `sdkmanager` discovery.

## Completed stages

- Stages 1–100: reverse-engineering foundation, DTRZ inspection/VFS/decoder, guest file adapter and dispatcher tests, XE3U symbol/relocation audit, and Android ARM64 build planning.
- Stage 101: local-only APK asset staging with allowlist and SHA-256 manifest.
- Stage 102: Android build orchestration and ELF64/AArch64 verifier.
- Stage 103: pinned XZ/liblzma Android cross-build automation.
- Stage 104: host CMake integration smoke and AArch64 archive-member verifier.
- Stage 105: Android toolchain doctor.
- Stage 106: pinned Android/Gradle/XZ toolchain contract.
- Stage 107: pinned XZ source fetcher and SDK package installer.
- Stage 108: standalone offline XZ archive verifier, path/link traversal checks, five security/identity fixture tests, and `sdkmanager` doctor check.

## Validation snapshot

- 53 Python tests passed.
- Full host C regression suite passed.
- Host CMake/liblzma smoke passed (6 scenarios).
- All shell scripts passed syntax checks.
- Android toolchain doctor reports BLOCKED because this environment has no configured Android SDK/NDK, Gradle wrapper/system Gradle, or Android ARM64 liblzma prefix.
- No Android ELF, APK build, device runtime, or playable game boot is claimed.

## Next: Stage 110 — Provisioned ARM64 liblzma Build

1. Provision pinned Android SDK Platform 35, Build Tools 35.0.0, CMake 3.22.1, and NDK 30.0.16248370.
2. Provision the Gradle 8.9 wrapper using system Gradle or generate it from a trusted Gradle distribution.
3. Download XZ Utils 5.8.4 and verify it against the pinned SHA-256 using `tools/verify_xz_source_archive.py`.
4. Build static/PIC `liblzma.a` for `arm64-v8a`, API 23.
5. Verify every archive member as ELF64/AArch64.
6. Only then attempt the Android app build and verify the resulting shared library and APK.
7. Keep production guest imports quarantined until original Marmalade behavior is proven.

## Stage 109 — Android SDK/NDK Provisioning Gate

- Added strict post-install verification of Platform 35, Build Tools 35.0.0, SDK CMake 3.22.1, Platform Tools, pinned NDK r30, host LLVM tools, and Android CMake toolchain.
- SDK installer now invokes this verifier after installation; Toolchain Doctor reports consolidated package integrity.
- Three offline tests cover verifier coverage and integration. The complete Python suite now has 56 passing tests; host C regressions, six host LZMA smoke scenarios, and 14 shell syntax checks pass.
- Environment still lacks SDK/NDK, Gradle, XZ source, and ARM64 liblzma; Toolchain Doctor reports BLOCKED. No Android build is claimed.
- See `STAGE109_REPORT.md`.

## Next: Stage 110 — Provisioned ARM64 liblzma Build
