## Stage 120F — Dynamic relocation reconciliation

- Added a unified read-only report across section-backed REL/RELA, PT_DYNAMIC REL/RELA/JMPREL, and Android APS2 REL/RELA.
- Normalizes target, relocation type, symbol index, addend, source table, and symbol namespace.
- Reports exact duplicates only within the same symbol namespace.
- Reports cross-namespace matches as candidates and exact/partial file-range overlaps separately.
- Preserves every source record and raw count; no silent deduplication or relocation application.
- Keeps REL implicit addends unresolved.
- Added synthetic reconciliation and integrated APS2 tests.

### Next — Stage 120G: Dynamic symbol namespace and GNU hash bounds

1. Parse DT_GNU_HASH header, bloom filter, buckets, and chain termination with strict bounds.
2. Derive safe dynamic symbol count without assuming SysV DT_HASH exists.
3. Cross-check SysV DT_HASH nchain and GNU-hash-derived bounds when both are present.
4. Resolve symbols only when hash metadata safely bounds complete symbol/string ranges.
5. Preserve unavailable/ambiguous symbol status; do not guess.
6. Add malformed chain, bucket, bloom, overflow, and big-endian fixtures.
7. Keep RELR and relocation application out of scope.
