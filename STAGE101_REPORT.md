# Stage 101 — Android Asset Packaging + ARM64 Build Readiness

## Scope

Stage 101 closes a packaging gap between the Android `AssetManager` probe and the original game APK. The app previously contained only `app/src/main/assets/.gitkeep`, while `native_probe.c` expected `main.dz`. A native Android build could therefore succeed while the probe failed at runtime due to absent game assets.

This stage adds a local-only asset staging workflow. It extracts exactly three required APK assets into the Android assets directory and writes a SHA-256 manifest. The original APK and extracted proprietary assets are not included in the source repository archive.

## Original APK evidence

- APK SHA-256: `6654b4af083a899b5c75de92a66b485435b3ce6ac65afbe96082d5ede4eb23d2`
- Asset audit performed against the user-supplied original APK.
- Only the three allowlisted `assets/` entries below are staged.

| APK entry | Size (bytes) | SHA-256 |
|---|---:|---|
| `assets/main.dz` | 14,552,804 | `1de13896c0f2b1805d3808e72d9e95a7f181b2805dfc966d4a606d134f77927e` |
| `assets/biohazard.s3e` | 673,661 | `3b673d9e88071ad6a83ff892eef394447cf4cb81e540a11ae31f9265a181c271` |
| `assets/s3e.icf` | 18,490 | `1bad3703e805b557faa2bec7cde91dd0e82e68fb92294b9fbc112592d1f8aa9e` |

The `main.dz` payload was confirmed to begin with the `DTRZ` magic. The original `biohazard.s3e` was separately audited in Stage 97/100; it is staged unchanged from the APK.

## Implementation

- Added `tools/stage_android_assets.py`.
- The script requires exactly one occurrence of each allowlisted asset, validates the DTRZ magic, and writes SHA-256/size metadata.
- Existing staged files are not overwritten unless `--force` is explicitly supplied.
- Added `tests/test_stage_android_assets.py` covering success, missing entries, duplicate entries, bad DTRZ magic, and overwrite protection.
- Added `.gitignore` rules for all three proprietary assets and `local-assets-manifest.json`.
- Updated the Android native probe to report whether all three required payloads are actually packaged in the APK.
- Hardened `tools/android_arm64_preflight.sh` to report missing prerequisites together and check for Android liblzma headers and library files.

## Reproduce locally

```bash
python3 tools/stage_android_assets.py /path/to/original-game.apk
```

Then configure/build after installing the Android NDK and an Android `arm64-v8a` liblzma build:

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
export ANDROID_LIBLZMA_ROOT=/path/to/arm64-v8a/liblzma-prefix
bash tools/android_arm64_preflight.sh
```

The staged proprietary assets remain local and are ignored by Git.

## Validation

- Python unit tests: 27 passed.
- Native host mock syntax checks: passed.
- Existing host C regression suite through Stage 99: passed.
- Asset staging against the actual user-supplied APK: passed; 3 files / 15,244,955 bytes.
- Android NDK build: **not run**. `ANDROID_NDK_HOME` is not set in this environment.
- Device execution / game boot: **not performed**.

The preflight currently stops before CMake configure because no Android NDK is installed/configured here. This is an environment blocker, not evidence of an Android build failure.

## Runtime boundary

This remains an Android asset/loader probe scaffold, not the completed native game runtime. The guest import bridge is still not authorized for production registration. In particular, exact Marmalade `s3eFileRead` partial-read and error semantics remain open.

## Stage 102

1. Obtain/configure Android NDK and `arm64-v8a` liblzma.
2. Run the real Android CMake configure and build.
3. Inspect the produced ELF with `llvm-readelf` / `llvm-nm`; verify `ELF64`, `AArch64`, JNI exports, and linked liblzma symbols.
4. Package staged assets into a debug APK and verify the probe's AssetManager checks on an ARM64 device/emulator.
5. Continue original-runtime import and file-read semantics investigation without enabling unverified production imports.
