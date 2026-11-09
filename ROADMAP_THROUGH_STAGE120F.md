## Stage 120F — Dynamic relocation reconciliation

- Added a unified read-only report across section-backed REL/RELA, PT_DYNAMIC REL/RELA/JMPREL, and Android APS2 REL/RELA.
- Normalizes target, relocation type, symbol index, addend, source table, and symbol namespace.
- Reports exact duplicates only within the same symbol namespace.
- Reports cross-namespace matches as candidates and exact/partial file-range overlaps separately.
- Preserves every source record and raw count; no silent deduplication or relocation application.
- Keeps REL implicit addends unresolved.
- Added synthetic reconciliation and integrated APS2 tests.

## Stage 120G — Dynamic symbol namespace and GNU hash bounds

- Added bounded DT_GNU_HASH parsing for ELF32 EM_ARM.
- Validates header dimensions, power-of-two bloom size, bloom shift width, bloom filter, bucket indices, chain bounds, and terminators.
- Derives a safe dynamic symbol count without requiring DT_HASH.
- Verifies GNU chain hashes and bloom membership against bounded DT_SYMTAB/DT_STRTAB names.
- Cross-checks GNU-derived symbol bounds against SysV DT_HASH nchain when both exist.
- Reuses the improved resolver for ordinary dynamic REL/RELA/JMPREL and APS2 relocation audits.
- Added GNU-hash-only, bloom corruption, chain corruption, and dimension-boundary fixtures.

### Next — Stage 120H: Dynamic version metadata and symbol identity

1. Parse DT_VERSYM and version index metadata with PT_LOAD-bounded reads.
2. Parse DT_VERDEF/DT_VERNEED and their auxiliary chains.
3. Correlate versioned symbols without treating equal symbol indices as equivalent identities.
4. Preserve unresolved, hidden, local, and weak version states.
5. Add malformed linked-list, string-offset, and index fixtures.
6. Keep provider lookup and relocation application disabled.
