# Roadmap through Stage 120H

Stage 120F reconciles section-backed, PT_DYNAMIC, and APS2 relocation inventories. Stage 120G adds GNU hash dynamic symbol bounds.

## Stage 120H — ELF Dynamic Symbol Version Metadata

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
