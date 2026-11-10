# Stage 120M Report — Independent ELF Cross-Check and COPY Symbol-Size Validation

Status: implementation complete; GitHub Actions Source checks passed.

## Delivered

- Added GNU/LLVM readelf cross-check tool.
- Compares section relocation offset/type multisets and reports mismatches.
- Explicitly skips when no independent tool is installed.
- Exposes symbol st_size and definition state from bounded SHT_SYMTAB and PT_DYNAMIC symbol parsing.
- Resolves R_ARM_COPY target width only with nonzero symbol index, defined symbol, positive size, and explicit namespace.
- Keeps COPY width unknown if provenance is incomplete; APS2 remains unresolved when symbol-size provenance is unavailable.
- Added tests for parser, matching/mismatch, unsupported external types, skip behavior, real tool execution, and COPY metadata.

## Usage

    python3 tools/guest_arm_relocation_independent_crosscheck.py guest.elf --json /tmp/readelf-crosscheck.json

## Validation result

GitHub Actions run 42605382359 completed successfully:
- 218 Python tests passed (18.876 seconds).
- Real independent readelf invocation test passed when available.
- COPY size provenance and rejection cases passed.
- Host C mock compilation and native C regression checks passed.

See STAGE120M_VALIDATION_LOG.txt for details.

## Normative reference

ARM ABI AAELF32:
https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst

## Safety

This stage audits metadata only. It does not execute input, resolve providers, dereference relocation targets, reconstruct implicit addends, or apply relocations.
