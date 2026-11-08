# Stage 101 — Android Asset Packaging and Build Readiness

The Android native probe expected `main.dz`, but the app's assets directory contained only `.gitkeep`. A successful APK build could therefore still fail its runtime asset check.

`tools/stage_android_assets.py` now extracts exactly `main.dz`, `biohazard.s3e`, and `s3e.icf` from a user-supplied original APK into `app/src/main/assets/`. It validates DTRZ magic, rejects missing/duplicate entries, protects existing files unless `--force` is passed, and writes a local SHA-256 manifest.

These game assets are proprietary build inputs. `.gitignore` excludes them and the generated manifest; they must not be committed or included in source archives.

Run:

```bash
python3 tools/stage_android_assets.py /path/to/original-game.apk
```

The native probe now verifies all three files are packaged before proceeding with its DTRZ header checks.

The Android NDK build remains blocked until `ANDROID_NDK_HOME` and an Android `arm64-v8a` liblzma prefix are provided. See `STAGE101_REPORT.md` for evidence, validation, and Stage 102.
