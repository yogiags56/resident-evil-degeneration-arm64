# Roadmap through Stage 115

- Stages 94–99: native DTRZ VFS, guest file bridge, dispatcher integration, binary API audit.
- Stages 100–107: Android ARM64 build configuration, asset staging, pinned toolchain and Gradle/XZ provisioning.
- Stages 108–111: provisioning tests, SDK/NDK validation, GitHub Actions workflow.
- Stage 112: artifact collector, APK/native binary consistency and SHA-256 manifest.
- Stage 113: persistent NDK environment handoff and CI integration hardening.
- Stage 114: GitHub repository publisher, private by default, proprietary-asset guard.
- Stage 115: private-only publication, main-branch push, automatic workflow dispatch, retry guidance.

## Remaining blockers

1. Run `gh auth login` on a machine with GitHub CLI and permission to create private repositories.
2. Execute `bash tools/publish_github_repository.sh` from this source tree.
3. Inspect the first GitHub Actions run, fix runner/toolchain issues, and obtain verified ARM64 `liblzma.a`, ELF and APK artifacts.
4. Integrate original game assets only through local ignored asset staging; never commit or publish proprietary game files.
5. Continue from bootstrap/probe toward the actual native Marmalade runtime and game boot.

No remote GitHub repository, Android APK, device test, or playable game is claimed by this stage.
