# Roadmap through Stage 120G

Stage 120F unified section-backed, PT_DYNAMIC, and APS2 relocation inventories. See STAGE120F_REPORT.md.

## Stage 120G — GNU Hash Dynamic Symbol Bounds

- Implemented DT_GNU_HASH parsing for ELF32 EM_ARM.
- Bounded header, bloom, buckets, and chains using PT_LOAD file-backed ranges.
- Derived a safe dynamic symbol count for GNU-hash-only inputs.
- Validated bloom membership and GNU chain hashes against symbol names.
- Cross-checked GNU bounds against SysV DT_HASH nchain when both are present.
- Reused the shared symbol resolver in ordinary dynamic and APS2 relocation auditors.
- GitHub Actions Source checks passed: 154 Python tests and host C/native regressions.
- No relocation application, provider lookup, Android device execution, or game boot is claimed.

## Next — Stage 120H: Dynamic version metadata and symbol identity

1. Parse DT_VERSYM using PT_LOAD-bounded reads.
2. Parse DT_VERDEF and DT_VERNEED structures and bounded auxiliary chains.
3. Correlate versioned symbols using version indices and string-table names.
4. Preserve hidden, local, weak, and unresolved version states.
5. Reject malformed linked lists, invalid offsets, cyclic chains, and index mismatches.
6. Add synthetic little-endian and big-endian fixtures.
7. Keep provider lookup and relocation application disabled.
