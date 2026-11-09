# Stage 120P Report — APS2/RELR Coexistence

Status: implementation complete; GitHub Actions passed 272 Python tests and host C/native regressions.

## Delivered

- Added a bounded ELF32 RELR decoder for direct entries and 31-bit bitmap expansion.
- Added standard DT_RELR and Android DT_ANDROID_RELR tag support, including mixed aliases.
- Added optional DT_ANDROID_RELRCOUNT hint preservation without treating it as table length.
- Integrated RELR tables into ordinary dynamic REL/RELA and Android APS2 audit outputs.
- Added cross-format target collision reporting.
- Added little-/big-endian, malformed stream, overflow, tag completeness, alias, and coexistence fixtures.

## Safety

No target contents are read or modified. No load bias is computed. No relocation is applied. No game boot or Android-device execution is claimed.

## Reference

The decoder's direct-entry and bitmap cursor semantics are based on Android Bionic's linker implementation:
https://android.googlesource.com/platform/bionic/+/master/linker/linker.cpp


## Validation

GitHub Actions run 42619189507 passed 272 Python tests in 12.026 seconds, including RELR decoding, standard/Android aliases, APS2 coexistence, malformed inputs, overflow, and cross-format collision reporting. Host C mock compilation and native C regression checks also passed. See STAGE120P_VALIDATION_LOG.txt.
