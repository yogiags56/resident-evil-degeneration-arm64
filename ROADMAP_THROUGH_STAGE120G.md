# Roadmap through Stage 120G

Stage 120F unified section-backed, PT_DYNAMIC, and APS2 relocation inventories. See STAGE120F_REPORT.md.

## Stage 120G — GNU Hash Dynamic Symbol Bounds

- Implemented DT_GNU_HASH parsing for ELF32 EM_ARM.
- Bounded header, bloom, buckets, and chains using PT_LOAD file-backed ranges.
- Derived a safe dynamic symbol count for GNU-hash-only inputs.
- Validated bloom membership and GNU chain hashes against symbol names.
- Cross-checked GNU bounds against SysV DT_HASH nchain when both are present.
- Reused the shared symbol resolver in ordinary dynamic and APS2 relocation auditors.
- GitHub Actions Source checks passed: 157 Python tests and host C/native regressions.
- No relocation application, provider lookup, Android device execution, or game boot is claimed.

## Stage 120H — Dynamic symbol version metadata and identity

- Parses DT_VERSYM, DT_VERDEF/DT_VERDEFNUM, and DT_VERNEED/DT_VERNEEDNUM.
- Associates dynamic symbols with version definitions or dependency requirements.
- Preserves version name, library, version index, and hidden bit in relocation records.
- Includes symbol version identity in duplicate and cross-namespace reconciliation.
- GitHub Actions Source checks passed: 165 Python tests and host C/native regression checks (run 42600277144).
- No provider compatibility, relocation application, Android device execution, or game boot is claimed.

## Next — Stage 120I: ELF32 RELR relative relocation decoding

1. Verify RELR tag and bitmap semantics for ELF32.
2. Parse DT_RELR/DT_RELRSZ/DT_RELRENT through PT_DYNAMIC and PT_LOAD.
3. Decode direct offsets and bitmap offsets with alignment and overflow guards.
4. Reconcile RELR target inventories without inventing symbol indices or addends.
5. Add malformed, boundary, and endian tests.
6. Keep load-bias calculation and relocation application disabled.
