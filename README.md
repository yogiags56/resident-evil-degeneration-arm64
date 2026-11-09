# Resident Evil: Degeneration — Native ARM64 Port Research

[![Stage](https://img.shields.io/badge/stage-119P-blue)](#project-status)
[![Platform](https://img.shields.io/badge/platform-Android%20ARM64-green)](#build-the-android-probe)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)

An engineering research project to run the original ARM32 Android game runtime on **native Android ARM64**, without an ARM32 emulator. The repository consolidates the verified guest CPU/import/runtime/GL source through Stage73, the file/JNI/AssetManager modules through Stage99, a verified Python DTRZ archive parser/extractor plus a native C DTRZ metadata parser, read-only VFS, optional bounded liblzma LZMA-alone decoding, and a DTRZ-backed guest file-adapter backend, alongside analysis tools, reproducible tests, and an Android asset-probe harness. It is a clean current-source baseline, not an archive of every intermediate experiment from Stages 1–89.

> **Status:** research/prototype. The original game has **not** been booted or made playable by this repository. The Android ARM64 native probe has been built and ELF-verified in GitHub Actions; execution on a physical device is still unverified.


## Android toolchain diagnostics (Stage 105)

Before attempting the Android build, run:

```bash
bash tools/android_toolchain_doctor.sh
# Machine-readable report:
bash tools/android_toolchain_doctor.sh --json
```

The doctor checks the installed NDK revision and host LLVM/CMake tools, CMake/Ninja, Java/Gradle, and the `ANDROID_LIBLZMA_ROOT` header/archive. It also verifies that the static liblzma archive contains an ELF64 AArch64 object when the required tools are present. It is diagnostic-only and does not install SDK components or download dependencies. A non-zero exit means the environment is not ready for the Android build.

## Project status

Stage 91 reconstructed and Python-verified the examined `main.dz`: 47 filenames, 34 non-root folder strings, 47 attribute records, a 47-entry location table, and 47 LZMA-alone payload windows. Stage 92 adds a native C parser/VFS. Stage 93 adds an optional bounded liblzma decoder; during local validation, all 47 native outputs matched the Python extraction byte-for-byte. Stage 94 connects the DTRZ VFS to the generic guest file adapter; all 47 original archive entries were read through that adapter and matched the Stage 93 Python extraction by decoded size and FNV-1a digest. Stage 95 adds non-executing argument capture and a readiness audit; Stage 96 adds a synthetic import-dispatch integration test. Stage 97 reopens the user-supplied original APK, parses the real XE3U fixup table, expands the file API callsite inventory, and proves that `s3eFileRead` returns an item count rather than a byte count. Stage 98 adds a backend-backed host-side guest file bridge for Open/Read/GetChar/Close. Stage 99 exercises those real handlers through the AAPCS32 dispatcher and DTRZ backend; 31 end-to-end assertions verify item-count results, guest-memory writes, EOF, stale handles, and cleanup. Stage 100 records exact XE3U symbol indices for all four file APIs and adds an Android ARM64 + liblzma CMake preset and preflight script. Production guest file imports remain disabled pending exact original partial-read/error behavior and production import-table binding verification. See [Stage 100 report](STAGE100_REPORT.md), [Stage 99 report](STAGE99_REPORT.md), [Stage 91 report](STAGE91_REPORT.md), [Stage 92 report](STAGE92_REPORT.md), [Stage 93 report](STAGE93_REPORT.md), [Stage 95 report](STAGE95_REPORT.md), [Stage 96 report](STAGE96_REPORT.md), [Stage 97 report](STAGE97_REPORT.md), and [Stage 98 report](STAGE98_REPORT.md).

Current confirmed foundations:
- ARM32 guest CPU/interpreter work from earlier stages (partial instruction coverage).
- AAPCS32 import/dispatcher infrastructure (only individually verified imports may be enabled).
- Android `AAssetManager` backend and JNI lifetime bridge.
- `s3eFileRead` argument capture/validation plus a backend-backed Stage 98 bridge and Stage 99 dispatcher integration; Stage 97 establishes its return unit as complete items, while exact partial-read/error semantics remain quarantined.
- Python DTRZ attribute/location parser and LZMA-alone verifier/extractor for the observed profile.
- Native C DTRZ metadata parser and read-only VFS, with optional `liblzma` LZMA-alone callback implementation.

## Repository layout

```text
app/                    Android ARM64 asset-probe application
  src/main/cpp/          C backend, JNI bridge, file adapter, ABI contract
  src/main/java/         Java Activity
  src/main/assets/       local-only game assets (ignored by Git)
runtime/guest/          ARM32 guest CPU, AAPCS32, imports, runtime coordinator, verified GL adapters
runtime/archive/       Native DTRZ parser, VFS, LZMA callback, and file-adapter backend
tools/                  archive inspection and build/test helpers
tests/                  host tests and mock Android headers
docs/                   reverse-engineering notes and stage reports
local-assets/           optional local working area (ignored by Git)
```

## Requirements

### Host tools
- Python 3.10+
- Clang or GCC
- Bash (for the provided host compile helper)

### Android probe build
- Android Studio or Gradle 8.x
- Android SDK platform 35
- Android NDK
- CMake 3.22.1
- An ARM64 Android device for runtime validation

## Run host checks

From the repository root:

```bash
bash tools/run_tests.sh
```

The host mock compile checks C translation units against small test-only Android API headers. It is **not** a substitute for compiling with the Android NDK.

## Audit a locally supplied original APK / S3E image

The Stage 97 read-only audit accepts the original APK (default asset path `assets/biohazard.s3e`) or an extracted compressed/uncompressed S3E image:

```bash
python3 tools/s3e_binary_audit.py /path/to/original.apk --json /tmp/s3e-audit.json --code-out /tmp/biohazard-code.bin
# Or pass a standalone .s3e file:
python3 tools/s3e_binary_audit.py /path/to/biohazard.s3e
```

It validates the LZMA-alone/XE3U header and fixup bounds, maps imported symbols to external relocation cells, and reports SHA-256 provenance. It does not patch, execute, or redistribute the proprietary game binary. See [Stage 97 evidence](docs/STAGE97_ORIGINAL_BINARY_AND_FILE_API_EVIDENCE.md).

## Inspect and extract a locally supplied `main.dz`

The project does not distribute the copyrighted game archive or executable. Place your own original APK or extracted S3E image outside the repository to audit the executable, and place your own extracted `main.dz` outside the repository to inspect the archive.

```bash
# List the parsed entry table without writing game data
python3 tools/dtrz_extract.py /path/to/main.dz --list

# Decompress and verify every entry without extracting files
python3 tools/dtrz_extract.py /path/to/main.dz --verify

# Extract into a directory outside this repository
python3 tools/dtrz_extract.py /path/to/main.dz --extract /path/to/output
```

The Stage 91 parser reads counts from the archive header. Its validated compression support is intentionally limited to the examined profile: method `0x200` (LZMA-alone) and the optional stored method `0x100`. Other method variants are rejected rather than guessed. The earlier `tools/dtrz_inspect.py` remains available as a conservative boundary/hex inspector.

The Stage 91 report records the observed table offsets and the full 47-entry validation. This is not a claim that every DTRZ variant uses identical details.


## Native DTRZ VFS and LZMA decoder (Stages 92–93)

The C module in `runtime/archive/` parses the observed DTRZ table layout and exposes path lookup plus read/seek/tell/close. Stored entries (`0x100`) are supported directly. Stage 93 adds `re_dtrz_lzma.c`, an optional liblzma-backed LZMA-alone (`0x200`) decoder with a configurable 64 MiB default dictionary/state memory limit and strict output-cap checks. The Android CMake option `RE_ENABLE_LIBLZMA` is OFF by default; enable it only after supplying liblzma built for Android ARM64. See [Stage 92 design](docs/STAGE92_NATIVE_DTRZ_VFS.md) and [Stage 93 decoder notes](docs/STAGE93_NATIVE_LZMA.md).

Run the synthetic native VFS tests with `bash tools/run_tests.sh`. To inspect a user-supplied archive using the native metadata parser:

```bash
clang -std=c11 -Wall -Wextra -Werror -Iruntime/archive \
  tools/dtrz_native_inspect.c runtime/archive/re_dtrz_vfs.c \
  -o /tmp/dtrz_native_inspect
/tmp/dtrz_native_inspect /path/to/main.dz
```

The native VFS caps a decoded entry at 64 MiB by default. Its archive parser borrows the input byte buffer; keep that buffer alive until the archive is destroyed and all open VFS files are closed. The native verifier can extract a user-supplied archive through the same C VFS: `tools/dtrz_native_verify.c` takes the archive path and output directory. The host test script uses `pkg-config liblzma` when available.

## Build the Android probe

1. Obtain a legally usable copy of the original APK.
2. Extract the required assets locally into `app/src/main/assets/`:
   - `main.dz`
   - `app.icf`
   - `s3e.icf`
3. Build:

```bash
gradle assembleDebug
```

4. Install and launch on an ARM64 device:

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n org.example.redstage/.MainActivity
```

The app is an **asset/JNI probe**, not the game port. It validates the APK asset access path only.

## Engineering rules

- Do not activate an import handler until its ABI and return behavior are supported by evidence.
- Do not map an internal byte count to guest `r0` unless the original API contract is proven.
- Treat all unverified DTRZ metadata as opaque.
- Keep guest ARM32 state separate from host AArch64 pointers and types.
- Test malformed paths, truncated archives, short reads, and handle lifetime.
- Do not claim successful game boot without a recorded real-device boot/gameplay test.

## Copyright and assets

This repository contains project source code and documentation only. It does not include the original APK, game executable, audio, video, textures, levels, or other proprietary assets. Names such as *Resident Evil* and *Resident Evil: Degeneration* are used only to identify the compatibility research target and remain the property of their respective owners. Users must supply assets they are legally entitled to use.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for coding, test, and evidence requirements. See [DTRZ format references](docs/DTRZ_FORMAT_REFERENCES.md) for public format documentation and [Stage 91 report](STAGE91_REPORT.md) for archive-specific validation. Reverse-engineering findings should distinguish **observed**, **inferred**, and **unknown** behavior.

## Native LZMA verification command (Stage 93)

Install `liblzma` development files and `pkg-config` on the host. The CI workflow installs these dependencies automatically. Then run `bash tools/run_tests.sh`.

To run the native extractor directly:

```bash
clang -std=c11 -Wall -Wextra -Werror -Iruntime/archive \
  runtime/archive/re_dtrz_vfs.c runtime/archive/re_dtrz_lzma.c \
  tools/dtrz_native_verify.c $(pkg-config --cflags --libs liblzma) \
  -o /tmp/dtrz_native_verify
/tmp/dtrz_native_verify /path/to/main.dz /path/to/native-output
```

This native command writes decoded entries to the requested output directory. Keep proprietary archives and extracted outputs outside the Git repository.

## DTRZ-backed guest file adapter (Stage 94)

`runtime/archive/re_dtrz_file_backend.c` implements the existing `REFileBackend` callback contract over a parsed DTRZ VFS. The consolidated Stage 82 adapter source now lives at `app/src/main/cpp/re_file_adapter.c/.h`. The adapter backend is read-only (`rb`), supports sequential byte reads and `get_char` EOF behavior, and tracks active streams. Archive bytes, parsed metadata, VFS, and decoder context are borrowed and must outlive all streams. Each open currently decodes a complete entry into memory; there is no decoded-entry cache or streaming LZMA. The generic backend interface has no seek callback.

Run synthetic integration and all regressions:

```bash
bash tools/run_tests.sh
```

To validate a locally supplied original `main.dz` through the full guest file adapter -> DTRZ backend -> VFS -> liblzma path:

```bash
./tools/run_archive_adapter_validation.sh /path/to/main.dz
```

The verifier reads every entry to EOF and prints per-entry byte count and FNV-1a digest. In the Stage 94 local validation, all 47 entries (57,976,257 decoded bytes) matched the Stage 93 Python extraction by size and digest. Keep the APK, archive, and extracted files outside Git. The adapter is not yet wired to guest `s3eFile*` imports; `s3eFileRead` return semantics remain unresolved. See [Stage 94 notes](docs/STAGE94_ARCHIVE_FILE_BACKEND.md).

## Guest file import bridge audit (Stages 95–96)

Stage 95 added evidence-only argument capture for `s3eFileOpen`, `s3eFileGetChar`, and `s3eFileClose`. Stage 96 adds a test-only synthetic AAPCS32 dispatch test for Open/Read/GetChar/Close, proving argument and register plumbing through the dispatcher. The test uses synthetic symbol IDs and marker returns; it does not call the file backend or establish Marmalade API return semantics. No production guest file import is enabled. See [Stage 95 audit](docs/STAGE95_GUEST_FILE_API_BRIDGE_READINESS.md) and [Stage 96 notes](docs/STAGE96_SYNTHETIC_FILE_IMPORT_DISPATCH.md).


## Android local asset staging

The Android probe requires the original APK's `main.dz`, `biohazard.s3e`, and `s3e.icf` assets. They are intentionally not stored in this repository. Stage them locally from a user-supplied APK:

```bash
python3 tools/stage_android_assets.py /path/to/original-game.apk
```

The script validates the archive entries and DTRZ magic, then writes a local SHA-256 manifest. Proprietary staged assets are ignored by Git and must not be redistributed or included in source archives. See `STAGE101_REPORT.md`.

Android ARM64 build prerequisites and commands are documented in `tools/android_arm64_preflight.sh`; an installed NDK and Android `arm64-v8a` liblzma build are required.


## Stage 102 Android ARM64 build

The Android Gradle configuration now requires LZMA support. Set `ANDROID_NDK_HOME` and `ANDROID_LIBLZMA_ROOT` (an `arm64-v8a` liblzma prefix), stage the original APK assets, and run `tools/build_android_arm64.sh`. The output verifier checks ELF64/AArch64 and the JNI `nativeProbe` export. The current source checkout does not include a Gradle wrapper; use Gradle 8.9+ locally. See `STAGE102_REPORT.md`.


## Stage 103: build Android ARM64 liblzma

A host liblzma cannot be linked into the Android ARM64 target. With Android NDK, CMake/Ninja, and an unpacked XZ Utils source tree available, build the correct dependency using:

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
export ANDROID_LIBLZMA_ROOT=/path/to/deps/android-arm64
export XZ_SOURCE_DIR=/path/to/xz-source
bash tools/build_liblzma_android_arm64.sh
```

Then run `tools/build_android_arm64.sh`. See `docs/STAGE103_LIBLZMA_ANDROID_BUILD.md` and `STAGE103_REPORT.md`.


## Stage 104: validate liblzma CMake linkage and ABI

Run `bash tools/run_host_liblzma_cmake_smoke.sh` to compile the production LZMA decoder through CMake's `LibLZMA::LibLZMA` imported target and execute its six decoder scenarios using host liblzma. This is a host integration test only.

Before Android linking, `tools/verify_android_liblzma.sh` inspects an object from the static archive and requires ELF64/AArch64, preventing accidental use of host libraries. See `STAGE104_REPORT.md`.


## Dependency provisioning (Stage 107)

Install the pinned Android SDK packages after installing command-line tools and accepting the license prompts:

```bash
bash tools/install_android_sdk_packages.sh
```

The liblzma cross-build fetches the pinned XZ Utils 5.8.4 source automatically when `XZ_SOURCE_DIR` is not supplied. The archive SHA-256 is verified before safe extraction into the ignored `.deps/` directory. See `STAGE107_REPORT.md`.

## Verify the pinned XZ source archive (Stage 108)

Before extracting the XZ Utils source archive, the fetcher invokes an offline verifier:

```bash
python3 tools/verify_xz_source_archive.py .deps/cache/xz-5.8.4.tar.gz
bash tools/fetch_xz_source.sh
```

The verifier checks the locked SHA-256, tar member and link paths, and the embedded liblzma version header. Fixture tests do not require network access:

```bash
python3 -m unittest discover -s tests -p 'test_stage108*.py' -v
```

`tools/android_toolchain_doctor.sh` also checks that `sdkmanager` is discoverable, including the SDK command-line tools path.


Current milestone: **Stage 108**. See [Stage 108 report](STAGE108_REPORT.md) and [roadmap through Stage 108](ROADMAP_THROUGH_STAGE108.md).


Current milestone: **Stage 109**. See [Stage 109 report](STAGE109_REPORT.md) and [roadmap through Stage 109](ROADMAP_THROUGH_STAGE109.md).

Stage 109 validation: 56 Python tests, all host C regression tests, six host liblzma smoke scenarios, and all 14 shell syntax checks passed. Android SDK/NDK provisioning remains blocked in this environment.

## Android CLI migration (Stage 110)

Android now documents the standalone `android` CLI as the preferred SDK package manager; `sdkmanager` is deprecated. Install the official CLI using Google's documented installer for your host OS:

- Linux x86-64: `curl -fsSL https://dl.google.com/android/cli/latest/linux_x86_64/install.sh | bash`
- Linux ARM64: `curl -fsSL https://dl.google.com/android/cli/latest/linux_arm64/install.sh | bash`

Official installation options: https://developer.android.com/tools/agents/android-cli/download

After installation, make sure `android` is on `PATH`, set `ANDROID_SDK_ROOT`, then run:

```bash
bash tools/install_android_sdk_packages.sh
bash tools/android_toolchain_doctor.sh
```

The installer prefers `android sdk install` and keeps `sdkmanager` only as a temporary compatibility fallback. It pins Platform 35, Build Tools 35.0.0, CMake 3.22.1, and NDK 30.0.16248370, then verifies the installed package tree. Package licenses may be presented interactively by the CLI.

Stage 110 validation currently covers the Android CLI path with a fake CLI that creates a synthetic SDK layout; it does not represent a real Android SDK installation or an ARM64 liblzma build.

## Stage 111 — Reproducible Android ARM64 CI

A GitHub Actions workflow is available at `.github/workflows/android-arm64.yml`. It provisions the official Android CLI, pinned SDK/NDK, checksum-verified Gradle 8.9, XZ Utils 5.8.4, builds ARM64 liblzma, verifies every archive member, builds the Android app, and checks the native ELF. It is triggered manually or on relevant pull requests.

For local Gradle provisioning, run:

```bash
GRADLE_HOME="$(bash tools/install_gradle_distribution.sh | tail -n 1)"
export PATH="$GRADLE_HOME/bin:$PATH"
bash tools/provision_gradle_wrapper.sh
```

The Gradle installer validates the pinned SHA-256 and rejects unsafe ZIP paths/symlinks before extraction. Stage 111's workflow has not yet been executed on GitHub; no Android APK or ARM64 library is claimed until that run succeeds.

Stage 111 validation: 69 Python tests passed; full host C regression and six host liblzma smoke scenarios passed; all 15 shell scripts passed syntax checks. The Android Toolchain Doctor still reports BLOCKED because this workspace has no Android CLI/SDK/NDK/Gradle ARM64 prefix. The GitHub Actions workflow is authored but awaits its first remote run.


## Stage 112 — CI artifact validation and provenance

The CI collector now gathers the debug APK, ARM64 native probe library, and Android ARM64 liblzma archive into one curated artifact. It validates the APK's `lib/arm64-v8a/libre_asset_probe.so` entry, blocks accidental inclusion of original proprietary game assets, and writes `BUILD_ARTIFACTS.json` and `SHA256SUMS.txt`. GitHub Actions retains this artifact for 14 days. The output is explicitly a **bootstrap/probe-only package**, not a playable game.

Stage 112 validation: 72 Python tests passed, including three artifact collector integration tests. The remote workflow has not yet run because no matching project repository was available through the connected GitHub account. See `STAGE112_REPORT.md` and `ROADMAP_THROUGH_STAGE112.md`.

## Stage 113 — NDK environment handoff and artifact consistency

Stage 113 fixes a CI integration defect: GitHub Actions runs each step in a separate shell, so the SDK installer now persists the verified NDK path to `GITHUB_ENV` as `ANDROID_NDK_HOME` and `ANDROID_NDK_ROOT`. A dedicated workflow gate checks the path and exact NDK revision before Gradle and native compilation. The artifact collector additionally requires the standalone native `.so` to be byte-identical (SHA-256) to the copy packaged inside the APK.

Stage 113 validation: **76 Python tests passed**, the full host C regression suite passed, all six host LZMA smoke scenarios passed, all 15 shell scripts passed syntax checks, and the GitHub Actions YAML parsed successfully. Remote GitHub Actions has still not been run; this environment has no connected project repository and no Android SDK/NDK/Gradle installation. No Android APK, ARM64 build output, device boot, or playable game is claimed. See `STAGE113_REPORT.md` and `ROADMAP_THROUGH_STAGE113.md`.

## Publish this source tree to GitHub (Stage 115)

The source can be published with GitHub CLI without creating files one by one. From an authenticated shell, run:

```bash
bash tools/publish_github_repository.sh
```

This creates `OWNER/resident-evil-degeneration-arm64` as a **private-only** repository, commits the current source tree if needed, pushes it to `main`, and dispatches the Android ARM64 workflow. The script refuses to repoint an existing `origin` remote and scans the Git index for proprietary game assets, generated binaries, and common credential/keystore files before committing. It requires `git`, `gh`, and a successful `gh auth login`; repository creation permissions must be granted to the authenticated GitHub account. Do not upload the original APK or extracted game assets.

This automation script is provided because the connected GitHub integration in the current session does not expose repository-creation permission/action. Once the repository exists and is connected, the workflow can be run from its Actions tab or via an authorized workflow dispatch.


## Stage 117–118 runtime foundation

Stage 117 compiles the partial ARM32/Thumb CPU interpreter, AAPCS32 dispatcher, and runtime coordinator into the Android ARM64 native library. Its CI build and source checks passed on commit `b090f90`. Stage 118 adds runtime stop/fault diagnostics and tests for unresolved imports, failed import dispatch, and invalid guest instruction fetch. See [Stage 118 report and roadmap](docs/STAGE118_RUNTIME_DIAGNOSTICS_AND_ROADMAP.md). The native probe remains a research harness: no physical-device execution or original game boot is claimed.


## Current engineering milestone — Stage 119P

The latest documented source milestone is Stage 119P (isolated A32 scalar transfer integration). The opt-in helper routes immediate pre-indexed/no-writeback byte, word, and halfword transfers through the Stage 119O scalar helper, including signed LDRSB/LDRSH. It is not called by the production CPU interpreter.

See [current status](docs/STATUS.md), [Stage 119P report](docs/STAGE119P_A32_SCALAR_TRANSFER_INTEGRATION.md), [Stage 119O report](docs/STAGE119O_CHECKED_SCALAR_TRANSFER.md), and [project roadmap](ROADMAP.md). Stage 119P branch CI is pending. No production CPU migration, original game boot, or playable build is claimed.
