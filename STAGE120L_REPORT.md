# Stage 120L Report — Relocation Symbol and Target Contract Cross-Validation

Status: implementation complete; GitHub Actions Source checks passed.

## Delivered

- Added conservative contract metadata to the AAELF32 relocation registry.
- Classifies symbol-index policy, processing applicability, and target policy.
- Flags invalid nonzero symbol indices on R_ARM_RELATIVE.
- Requires symbol references for R_ARM_COPY, R_ARM_GLOB_DAT, R_ARM_JUMP_SLOT, and R_ARM_TLS_DTPOFF32.
- Preserves nullable-symbol TLS cases for R_ARM_TLS_DTPMOD32 and R_ARM_TLS_TPOFF32.
- Leaves R_ARM_NONE symbol index unconstrained while preserving zero target access.
- Preserves valid ET_REL section-relative target domains.
- Annotates unified relocation entries with abi_contract and reports aggregate contract counts/issues.
- Unknown relocation types retain unknown contract metadata.

## Validation result

GitHub Actions run 42603875912 completed successfully on source commit f626e08e27577fdd88639fa5279c1fbdbb60fc06:
- 206 Python tests passed.
- Relocation symbol contract tests passed.
- TLS null-symbol exception tests passed.
- ET_REL section-relative target-domain tests passed.
- Unified reconciliation tests passed.
- Host C mock compilation and native regression checks passed.

See STAGE120L_VALIDATION_LOG.txt for the captured validation summary.

## Validation scope

Dedicated tests cover the symbol-free, required-symbol, nullable TLS, no-op, ET_REL, and unknown-type cases. CI status is tracked in STAGE120L_VALIDATION_LOG.txt.

## Normative reference

ARM ABI AAELF32:
https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst

## Safety boundary

No provider resolution, target dereference, implicit-addend reconstruction, load-bias calculation, or relocation application is performed. This does not demonstrate game boot.
