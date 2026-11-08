# Stage 112 Report — CI Artifact Validation and Build Provenance

## Delivered

- Added `tools/collect_android_build_artifacts.py` to collect exactly one debug APK, the ARM64 `libre_asset_probe.so`, and the Android ARM64 `liblzma.a` from a completed CI build.
- The collector verifies that the APK contains `lib/arm64-v8a/libre_asset_probe.so`, rejects the three original proprietary game asset paths, and writes `BUILD_ARTIFACTS.json` plus `SHA256SUMS.txt`.
- The build manifest records source commit, workflow run identifiers, ABI, minimum SDK, file sizes, SHA-256 hashes, and explicitly labels the output `bootstrap-probe-only` / `game_ready: false`.
- Updated GitHub Actions to run the collector before uploading a single curated artifact directory. Artifact retention is 14 days; compression is disabled for already-compressed APK/native binaries. The workflow artifact remains immutable under the v4 upload action.
- Updated the Android app version label to `0.1.0-stage112`.
- Added offline tests for successful collection, missing ARM64 library rejection, and proprietary asset rejection.

## Validation

- Python unit suite: **72 tests passed**.
- Synthetic artifact integration tests: **3 passed**.
- The test fixture verifies generated manifest fields and checksum list without using the original game APK or proprietary assets.
- GitHub Actions workflow syntax/order is covered by existing workflow tests; all shell scripts are syntax-checked in the full validation pass.

## Important build classification

The CI APK produced by this workflow is only an Android ARM64 bootstrap/probe package. It verifies native build, packaging, and dependency architecture. It is **not a playable Resident Evil: Degeneration build**. The user's original APK remains a local-only input and is not uploaded to GitHub. The real game asset staging and runtime integration still require a controlled local build with the user's APK, and Marmalade guest file imports remain disabled until their semantics are proven.

## Remote CI status and limitation

The GitHub Actions workflow has not been executed. No matching project repository was available through the connected GitHub account in this session, so there was no repository on which to dispatch the workflow or inspect a real runner log. The local environment also lacks Android CLI, SDK/NDK, Gradle, and an Android ARM64 liblzma prefix. Therefore this stage does not claim an actual Android APK, ARM64 `.so`, ARM64 `liblzma.a`, device test, or game boot.

## Next

Stage 113 should execute this workflow in the project's GitHub repository (or an equivalent network-enabled Linux runner), fix issues from real toolchain integration, and retain the actual APK/native outputs and runner logs. Once the probe build is green, a separate local asset-enabled build must be validated without publishing proprietary assets.

## Stage 113 follow-up note

Stage 113 identified and corrected a cross-step GitHub Actions environment handoff gap: the SDK installer now writes the verified pinned NDK path to `GITHUB_ENV` as `ANDROID_NDK_HOME` and `ANDROID_NDK_ROOT`. The workflow checks the persisted path and exact NDK revision before Gradle and native compilation. Artifact collection also compares the standalone `.so` SHA-256 with the exact `.so` payload embedded in the APK, preventing mismatched native outputs from being published.
