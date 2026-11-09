# Roadmap through Stage 120K

Stage 120J added PT_LOAD target range classification. Stage 120K adds AAELF32-backed relocation metadata for storage width, place alignment, class, and semantics.

## Stage 120K — ARM ELF32 relocation semantics and ABI validation

- Added a 60-entry AAELF32-backed relocation metadata registry.
- Distinguishes ABS8, ABS16, ABS32, Thumb16, Thumb32, variable-size COPY, and NONE.
- Integrates ABI width and alignment with target-range audit.
- Preserves unknown and platform-specific types.
- GitHub Actions run 42602775178 passed 199 Python tests and host C/native regressions.
- No relocation application or game boot is claimed.

## Stage 120L — Relocation symbol and target contract cross-validation

- Adds conservative symbol-index policies, target policies, and processing applicability to the AAELF32 registry.
- Flags nonzero symbol indices on R_ARM_RELATIVE.
- Requires symbol references for R_ARM_COPY, R_ARM_GLOB_DAT, R_ARM_JUMP_SLOT, and R_ARM_TLS_DTPOFF32.
- Preserves nullable-symbol TLS cases for DTPMOD32 and TPOFF32.
- Leaves R_ARM_NONE symbol index unconstrained; retains no-target-access behavior.
- Accepts ET_REL section-relative target domains.
- Adds per-entry abi_contract and aggregate contract counts/issues to unified reconciliation.
- Dedicated tests cover valid and invalid symbol policies, TLS null-symbol exceptions, ET_REL, and unknown relocation types.
- Final CI validation pending.

## Next — Stage 120M: Independent ELF-tool cross-check and COPY symbol-size validation

1. Compare synthetic ELF relocation records with GNU readelf and/or LLVM llvm-readelf.
2. Validate offsets, symbol indices, names, and table kinds.
3. Skip clearly if independent tools are unavailable.
4. Resolve R_ARM_COPY size only with a valid symbol st_size and verified symbol-table namespace.
5. Keep all target bytes read-only; do not apply relocations.
