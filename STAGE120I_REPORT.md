# Stage 120I Report — ELF32 RELR Relative Relocation Decoding

Status: implementation complete; GitHub Actions Source checks passed.

## Delivered

- Added tools/guest_arm_relr_audit.py for ELF32 EM_ARM DT_RELR parsing.
- Added direct-entry and bitmap-entry decoding with 32-bit word semantics.
- Added strict dynamic-tag pairing, table mapping, size, alignment, and overflow validation.
- Integrated RELR target records into guest_arm_relocation_reconciliation.py.
- RELR records retain direct/bitmap provenance and intentionally contain no symbol index or addend.
- Added malformed and big-endian synthetic fixtures.

## Validation

GitHub Actions run 42600655563, commit a053c922421577ac9a0ef9f1544f55f3ff607f1b: 177 Python tests passed, followed by host C mock compilation and native host regression executables.

## Safety boundary

The decoder reports target offsets only. It does not read target bytes, calculate runtime load bias, apply relocations, resolve providers, or claim original game boot.
