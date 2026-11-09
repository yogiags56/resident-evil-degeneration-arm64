# Stage 120R Report — PT_DYNAMIC RELR Differential Validation

Status: implementation added; CI validation pending.

## Changes

- Added a generated ELF32 ARM DYN fixture exposing one RELR stream through
  PT_DYNAMIC and a matching SHT_RELR section.
- Added an independent GNU readelf -rW differential test for the dynamic
  RELR decoder's direct and bitmap-expanded addresses.
- Added a unified reconciliation regression ensuring RELR entries retain
  null symbol indices and addends.

## Validation

Not yet run. The Stage 120R branch must pass GitHub Actions before this stage
can be marked complete. The required checks are full Python test discovery,
host C mock compilation, and native C regressions.

## Safety and project scope

No relocation is applied, no target contents are read or written, and no
runtime load bias, Android device execution, or original game boot is claimed.
