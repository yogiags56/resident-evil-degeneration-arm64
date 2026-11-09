# Roadmap through Stage 120M

## Stage 120L — Relocation symbol and target contract cross-validation

- Added conservative symbol-index policies, processing applicability, and target policies.
- Preserved RELATIVE symbol-free behavior, mandatory dynamic symbol cases, nullable TLS exceptions, NONE behavior, and ET_REL section-relative target domains.
- GitHub Actions run 42603875912 passed 206 Python tests and host C/native regression checks.

## Stage 120M — Independent ELF cross-check and COPY symbol-size validation

- Added GNU/LLVM readelf comparison of section relocation offsets and type names.
- Tool absence is an explicit skip; readelf is not a runtime dependency.
- Symbol st_size and defined-state provenance now flows from SHT_SYMTAB and PT_DYNAMIC symbol tables.
- R_ARM_COPY target width is resolved only when symbol index is nonzero, symbol is defined, st_size is positive, and namespace identity is explicit.
- Unknown or unproven COPY widths remain unknown. APS2 symbol-size parity remains a Stage 120N task.
- CI validation record is maintained in STAGE120M_VALIDATION_LOG.txt.

## Next — Stage 120N: Packed and dynamic relocation symbol metadata parity

1. Carry bounded dynamic symbol size, definition state, and version metadata consistently into APS2 records.
2. Prove symbol namespace identity before using symbol indices for COPY metadata.
3. Add sectionless PT_DYNAMIC and APS2 fixtures.
4. Cross-check dynamic relocation tags and records without section headers.
5. Preserve unresolved states for missing hashes, symbol bounds, and provider resolution.
6. Keep target bytes read-only; never apply relocations.
