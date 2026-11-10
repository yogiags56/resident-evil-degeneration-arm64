# Stage 120K Report — ARM ELF32 Relocation Semantics and ABI Metadata

Status: implementation complete; GitHub Actions Source checks passed.

## Delivered

- Added tools/guest_arm_relocation_abi.py with 60 explicit ELF32 ARM relocation metadata entries.
- Records target storage width, place alignment, instruction/data class, relocation category, and semantic summary.
- Corrects prior blanket 4-byte assumptions for ABS8, ABS16, Thumb16, Thumb32, and variable-size COPY.
- Keeps R_ARM_COPY width unresolved pending symbol st_size.
- Preserves R_ARM_NONE zero-width behavior and TARGET1 platform-specific status.
- Integrates the registry with target-range validation and dynamic relocation names.
- Adds per-type ABI coverage counts to unified reconciliation output.
- Adds tests for widths, alignment, unknown types, and metadata completeness.

## Validation

GitHub Actions run 42602775178, validated commit 0e47c78724e38f789dd8d827d521f18c001cac89:
- 199 Python tests passed.
- ABI registry width and metadata tests passed.
- Thumb32 alignment metadata and misalignment tests passed.
- Unified relocation target-range integration passed.
- Host C mock compilation and native regression executables passed.

## Normative source

ARM ABI AAELF32:
https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst

## Safety boundary

No relocation is applied. Target bytes, instruction encodings, implicit addends, GOT/PLT contents, and runtime load bias are not read or modified. The registry is intentionally incomplete; unknown types remain unknown.
