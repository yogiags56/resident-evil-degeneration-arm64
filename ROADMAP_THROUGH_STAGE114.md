# Roadmap through Stage 114

## Completed foundations
- Stages 91–99: DTRZ archive reconstruction, native parser/VFS, bounded optional LZMA decoder, guest file adapter, and AAPCS32 synthetic integration tests.
- Stages 100–110: Android ARM64 build configuration, asset staging, toolchain diagnostics, pinned SDK/NDK/Gradle/XZ provisioning, and strict architecture verification.
- Stage 111: GitHub Actions Android ARM64 build workflow and pinned Gradle distribution verifier.
- Stage 112: artifact collection, provenance manifest, checksums, and proprietary asset rejection.
- Stage 113: cross-step NDK environment handoff and byte-for-byte APK/native-library consistency checks.
- Stage 114: one-command GitHub repository publisher, private by default, with pre-push protection against proprietary assets and generated binaries.

## Current limitations
The GitHub integration in this session does not expose a repository-creation action, and no project repository is currently connected. The Stage 114 publisher is prepared but has not been run because it requires the user's authenticated GitHub CLI session and repository-creation permission. The remote CI workflow has not run. Android SDK/NDK/Gradle provisioning, ARM64 liblzma compilation, Android APK creation, device execution, and game boot remain unverified.

## Next: Stage 115
After the source is pushed to GitHub, trigger the `Android ARM64 native build` workflow, inspect the first real runner logs, fix any integration failures, and verify the uploaded APK, ELF architecture, native library hash, manifest, and checksum. The result remains a bootstrap/probe APK until the original game runtime is integrated and validated.
