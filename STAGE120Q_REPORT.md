# Stage 120Q Report — RELR Differential Validation

Status: complete; GitHub Actions passed 273 Python tests, including the GNU readelf differential fixture, plus host C/native regressions.

## Changes

- Aligned DT_ANDROID_RELRCOUNT handling with Android Bionic: ignored/non-authoritative metadata, not a required table tag.
- Exposed the optional count hint at the top level of both dynamic and APS2 audit reports.
- Added a minimal ELF32 ARM DYN fixture containing SHT_RELR.
- Added a GNU readelf -rW differential check for direct and bitmap-expanded relocation addresses.

## Validation

See STAGE120Q_VALIDATION_LOG.txt after the CI run completes.

## Safety and project scope

No relocation is applied and no target contents are read or written. No runtime load-bias calculation, Android device execution, or game boot is claimed.


## Validation evidence

GitHub Actions run 42640031158 completed successfully. All 273 Python tests passed in 5.135 seconds. The generated ELF32 ARM SHT_RELR fixture produced the same four expanded target addresses in the project decoder and GNU readelf -rW. Host C mock compilation and native C regression checks also passed. See STAGE120Q_VALIDATION_LOG.txt.
