# Stage 120K Report — ARM ELF32 Relocation Semantics and ABI Metadata

Status: implementation and CI validation in progress.

## Delivered

- Added tools/guest_arm_relocation_abi.py with 60 explicit ELF32 ARM relocation metadata entries.
- Records target storage width, place alignment, instruction/data class, relocation category, and semantic summary.
- Corrects prior blanket 4-byte assumptions for ABS8, ABS16, Thumb16, Thumb32, and variable-size COPY.
- Keeps R_ARM_COPY width unresolved pending symbol st_size.
- Preserves R_ARM_NONE zero-width behavior and TARGET1 platform-specific status.
- Integrates the registry with target-range validation and dynamic relocation names.
- Adds per-type ABI coverage counts to unified reconciliation output.
- Adds tests for widths, alignment, unknown types, and metadata completeness.

## Normative source

ARM ABI AAELF32:
https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst

## Safety boundary

No relocation is applied. Target bytes, instruction encodings, implicit addends, GOT/PLT contents, and runtime load bias are not read or modified. The registry is intentionally incomplete; unknown types remain unknown.
