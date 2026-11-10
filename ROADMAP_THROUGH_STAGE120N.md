# Roadmap through Stage 120N

## Stage 120L — Relocation symbol and target contract cross-validation

- Added conservative symbol-index policies, processing applicability, and target policies.
- Preserved RELATIVE symbol-free behavior, mandatory dynamic symbol cases, nullable TLS exceptions, NONE behavior, and ET_REL section-relative target domains.
- GitHub Actions run 42603875912 passed 206 Python tests and host C/native regressions.

## Stage 120M — Independent ELF cross-check and COPY symbol-size validation

- Added GNU/LLVM readelf comparison of section relocation offsets and type names.
- Tool absence is an explicit skip; readelf is not a runtime dependency.
- Symbol st_size and defined-state provenance flows from SHT_SYMTAB and PT_DYNAMIC symbol tables.
- R_ARM_COPY target width is resolved only when symbol index is nonzero, symbol is defined, st_size is positive, and namespace identity is explicit.
- GitHub Actions run 42605382359 passed 218 Python tests and host C/native regressions.

## Stage 120N — Packed and dynamic relocation metadata parity

- APS2 records now preserve symbol size, definition state, section index, and version metadata when bounded hash and dynamic symbol tables are available.
- Unified inventory uses DT_SYMTAB namespace only when symbol bounds are proven; otherwise the namespace remains unresolved.
- Dynamic REL and APS2 entries are tested against the same symbol-table identity and symbol metadata.
- R_ARM_COPY target width is resolved only with a nonzero, defined, positive-sized symbol and proven namespace.
- Corrected APS2 r_info to absolute values, GROUPED_BY_INFO reuse, grouped addend once-per-group semantics, per-entry addend deltas, no-addend resets, signed r_info bit patterns, and high-bit ELF32 initial offsets.
- GitHub Actions run 42614083636 passed 229 Python tests and host C/native regressions.

## Next — Stage 120O: Independent APS2 conformance vectors and malformed-input hardening

1. Add differential APS2 vectors transcribed from AOSP/LLVM source examples.
2. Exercise every legal REL and RELA group-flag combination.
3. Test multi-group state transitions and SLEB128 edge cases.
4. Add fuzz-style bounded malformed-input cases.
5. Keep unknown values preserved and all target bytes read-only.
