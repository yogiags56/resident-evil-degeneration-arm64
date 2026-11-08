# Roadmap through Stage 107

## Completed

- Stages 1–90: guest image, archive, runtime, and host-side foundations; see earlier roadmap snapshots.
- Stage 91: reconstructed and Python-verified the examined `main.dz` archive.
- Stage 92: native C DTRZ parser/VFS.
- Stage 93: optional bounded liblzma decoder; 47 archive entries matched Python extraction.
- Stage 94: DTRZ VFS connected to generic guest file adapter; 47 entries matched by size and FNV-1a digest.
- Stage 95: file API argument capture/readiness audit; production imports disabled.
- Stage 96: synthetic AAPCS32 file-import dispatcher test.
- Stage 97: original APK/XE3U audit; 227 symbols, 36,377 internal relocations, 238 external relocations; `s3eFileRead` return unit established as items.
- Stage 98: backend-backed Open/Read/GetChar/Close guest bridge; 48 assertions.
- Stage 99: real Stage 98 handlers through AAPCS32 dispatcher and synthetic DTRZ backend; 31 end-to-end assertions.
- Stage 100: exact XE3U symbol indices for four file APIs emitted and tested; Android ARM64 + liblzma CMake preset and preflight script added.

## Current gates

1. Obtain a supported Android NDK and Android ARM64 liblzma CMake package; run `tools/android_arm64_preflight.sh`.
2. Establish exact `s3eFileRead` partial-item, short-read, and error behavior from a version-matched SDK/runtime or controlled trace.
3. Verify production import-table construction and bind only individually proven symbols.
4. Validate Android AssetManager -> DTRZ VFS -> guest file adapter on a physical ARM64 device.
5. Expand guest CPU/runtime support from observed execution and diagnostics.
6. Establish graphics/audio/input lifecycle and observe an original game boot.

## Current status

Host-side archive and dispatcher integration is validated. Android ARM64 build and device runtime remain unverified. Production file imports remain disabled. The original APK and game executable are not distributed in the source repository.


# Stage 101 — Android Asset Packaging + Build Readiness

Stage 101 adds an allowlisted asset staging script, proprietary asset ignore rules, a generated local SHA-256 manifest, and runtime checks for required APK assets. Actual original APK extraction was validated. Host tests pass. Android NDK configure/build and device tests remain blocked/not run because the NDK is not configured.

See `STAGE101_REPORT.md` and `docs/STAGE101_ANDROID_ASSET_PACKAGING_AND_BUILD_READINESS.md`.


# Stage 102 — Android ARM64 Build Orchestration + ELF Verification

Gradle now requires LZMA-enabled native compilation; added build and ELF verification scripts. Host validation passed. The Android build could not be run because NDK and Android liblzma are unavailable and the checkout has no Gradle wrapper. See `STAGE102_REPORT.md`.


# Stage 103 — ARM64 liblzma Dependency Build Automation

Added an NDK-based XZ Utils/liblzma cross-build script and improved Android build/NDK preflight automation. Host tests pass. Actual cross-compilation remains blocked by missing NDK and XZ source. See `STAGE103_REPORT.md`.


# Stage 104 — liblzma CMake Integration + ARM64 Archive Verification

Validated production decoder CMake linkage against host liblzma and added an ELF64/AArch64 archive verifier. All 35 Python tests and host regressions pass. Android cross-build is still blocked by missing NDK/XZ source. See `STAGE104_REPORT.md`.


# Stage 105 — Android Toolchain Doctor & Build Readiness Gate

Added `tools/android_toolchain_doctor.sh` with text/JSON diagnostics for NDK, host LLVM tools, Android CMake toolchain, CMake/Ninja, Java/Gradle, and ARM64 liblzma files/architecture. Three new tests; all 38 Python tests and host regressions pass. The actual Android build remains blocked by missing NDK, XZ source, liblzma prefix, and Gradle. See `STAGE105_REPORT.md`.


# Stage 106 — Reproducible Android Toolchain Lock

Added `config/android-toolchain.lock.json` and pinned NDK r30, Gradle 8.9 with official SHA-256, AGP 8.7.3, Android SDK Platform 35, Build Tools 35.0.0, CMake 3.22.1, Java 17+, and XZ Utils 5.8.4. Added isolated Gradle wrapper provisioning and strict XZ source version guard; downgraded CMake preset schema to version 3 for compatibility with pinned CMake 3.22.1. All 44 Python tests and host regressions pass. Android build remains blocked because this environment has no SDK/NDK/Gradle/liblzma ARM64 prefix. See `STAGE106_REPORT.md`.


# Stage 107 — Dependency Acquisition & SDK Provisioning Automation

Added `tools/fetch_xz_source.sh` with pinned SHA-256, safe tar path validation, and extracted version-header verification. Added `tools/install_android_sdk_packages.sh` for the pinned Android SDK/NDK packages. The Android liblzma build can now fetch the exact source automatically. 48 Python tests and all host regression checks pass. Android SDK/NDK and XZ source archive were not provisioned in this environment, so ARM64 cross-compilation remains pending. See `STAGE107_REPORT.md`.
