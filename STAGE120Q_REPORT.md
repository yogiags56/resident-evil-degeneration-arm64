# Stage 120Q Report — RELR Differential Validation

Status: implementation committed; final CI validation pending.

## Changes

- Aligned DT_ANDROID_RELRCOUNT handling with Android Bionic: ignored/non-authoritative metadata, not a required table tag.
- Exposed the optional count hint at the top level of both dynamic and APS2 audit reports.
- Added a minimal ELF32 ARM DYN fixture containing SHT_RELR.
- Added a GNU readelf -rW differential check for direct and bitmap-expanded relocation addresses.

## Validation

See STAGE120Q_VALIDATION_LOG.txt after the CI run completes.

## Safety and project scope

No relocation is applied and no target contents are read or written. No runtime load-bias calculation, Android device execution, or game boot is claimed.
