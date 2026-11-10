# Stage 120L — Relocation Symbol and Target Contract Cross-Validation

## Goal

Extend Stage 120K's AAELF32 metadata with conservative symbol-index and target-domain contracts. This is an audit layer, not a relocation engine.

## Contract metadata

Each known relocation can now report:
- symbol-index policy: may be zero, must be nonzero, or must be zero/absent;
- processing applicability: dynamic, static/link-time, or unknown;
- target policy: no access, instruction storage, or data memory;
- contract issues observed in the input record.

Examples:
- R_ARM_RELATIVE is symbol-free; a nonzero symbol index is reported.
- R_ARM_GLOB_DAT, R_ARM_JUMP_SLOT, R_ARM_COPY, and R_ARM_TLS_DTPOFF32 require a nonzero symbol index.
- R_ARM_TLS_DTPMOD32 and R_ARM_TLS_TPOFF32 allow the ABI's null-symbol cases.
- R_ARM_NONE has no target access and does not impose a symbol-index restriction.
- ET_REL section-relative target domains remain valid; they are not incorrectly treated as missing virtual addresses.
- RELR entries are represented as R_ARM_RELATIVE and retain the symbol-free contract.

Unknown relocation types retain unknown contract fields and are not guessed.

## Integration

Unified output now annotates each relocation entry with abi_contract and adds relocation_abi.contract_counts and contract_issue_count.

Usage:

    python3 tools/guest_arm_relocation_reconciliation.py /path/to/guest.elf --json /tmp/relocation-reconciliation.json

## Validation

Tests cover RELATIVE symbol-free policy, mandatory symbol references for dynamic lookup/copy, nullable TLS cases, NONE behavior, ET_REL section target domains, and unknown types.

## Normative reference

ARM ABI — AAELF32:
https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst

## Deliberate limitations

- Symbol provider resolution is not performed.
- No relocation target bytes or instruction encodings are dereferenced.
- No REL implicit addends are reconstructed.
- COPY size is not resolved unless a later stage safely supplies symbol st_size.
- The contract is intentionally conservative and incomplete; no relocation is applied.
