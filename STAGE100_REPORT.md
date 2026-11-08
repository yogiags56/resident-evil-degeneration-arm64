# Stage 100 Report — ABI Evidence Closure + Android ARM64 Build Preparation

## Result

Stage 100 adds a reproducible audit of the four original guest file API symbol indices and prepares an explicit Android ARM64 + liblzma CMake build preset. The host validation suite remains green. An Android NDK build was not performed because this execution environment has no installed Android NDK, Ninja, Gradle, or Android-ABI liblzma package.

## Original XE3U symbol-index evidence

The user-supplied APK was re-audited directly with `tools/s3e_binary_audit.py`. The actual XE3U symbol table contains 227 entries and the external relocation table contains 238 entries. Each target API has one external Thumb relocation cell (fixup type 4), and the symbol-index-to-name relationship is now emitted explicitly by the audit utility:

| API | XE3U symbol index | External relocation cell | Fixup type |
|---|---:|---:|---:|
| `s3eFileClose` | 10 | `0x13B71C` | 4 (Thumb) |
| `s3eFileRead` | 57 | `0x13B7E0` | 4 (Thumb) |
| `s3eFileOpen` | 204 | `0x13BA50` | 4 (Thumb) |
| `s3eFileGetChar` | 211 | `0x13BA6C` | 4 (Thumb) |

The audit now reports `target_import_symbol_indices` and `target_import_relocation_counts_by_type`; parser tests assert both fields. These indices identify symbols in this exact examined XE3U image. They are not a license to activate production imports: runtime dispatch binding still requires a verified import-table construction path and ABI/runtime semantics.

## Android ARM64 build preparation

Added `CMakePresets.json` with an `android-arm64` configure/build preset:
- Android ABI: `arm64-v8a`
- Android platform: API 23
- Release build
- `RE_ENABLE_LIBLZMA=ON`
- `LibLZMA_ROOT` supplied through `ANDROID_LIBLZMA_ROOT`
- Android NDK toolchain supplied through `ANDROID_NDK_HOME`

Added `tools/android_arm64_preflight.sh`, which checks the NDK toolchain, Android liblzma CMake package prefix, CMake, Ninja, and Python; then runs the host Python tests, CMake configure, and Android build. It fails early with a clear message when prerequisites are absent.

The preset parses successfully with CMake. The preflight was intentionally run with Android environment variables absent and stopped at the expected NDK prerequisite check. No ARM64 `.so` or APK is claimed as built.

## `s3eFileRead` semantics boundary

Stage 97's original-code evidence establishes that the return value is an item count, not a byte count. Stage 100 does not overstate this as complete semantic closure. Exact partial-item copying, short-read behavior, error sentinel/state, and version-matched Marmalade runtime details remain unverified. The current host bridge's conventional fread-style handling remains provisional. Production guest file imports remain disabled.

## Validation

- Python unittest discovery: 22 tests passed.
- Host mock-header C syntax checks: passed.
- All native host tests passed, including Stage 94 (20), Stage 95 (14), Stage 96 (28), Stage 98 (48), and Stage 99 (31) assertions.
- `cmake --list-presets`: Android ARM64 preset parsed successfully.
- Original APK XE3U audit: 227 symbols, 238 external relocations; all four target symbol indices and relocation cells emitted and checked.
- Android preflight: correctly blocked because `ANDROID_NDK_HOME` is not available in this environment; no Android build attempted.

## Activation decision

**Production guest file imports remain disabled.** Stage 100 closes the target symbol-index evidence for this exact binary and prepares the build recipe, but does not close `s3eFileRead` partial/error semantics or verify production import-table binding. No original game boot, Android runtime, or gameplay is claimed.
