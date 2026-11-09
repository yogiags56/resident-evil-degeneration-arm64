# Stage 120F Report — Dynamic Relocation Reconciliation

Status: implementation committed; CI validation pending.

## Delivered
- Unified read-only report across section-backed REL/RELA, PT_DYNAMIC REL/RELA/JMPREL, and Android APS2 REL/RELA.
- Normalized target, type, symbol index, addend, source table, and symbol-table namespace.
- Exact duplicate detection only within the same symbol namespace.
- Cross-namespace tuple matches classified as candidates, not proven duplicates.
- Exact and partial file-range overlap reporting.
- All source records and raw counts preserved; no silent deduplication.
- REL implicit addends remain unresolved.

## Validation
Run bash tools/run_tests.sh. Do not claim success until GitHub Actions confirms the full host workflow.

## Safety boundary
The tool compares metadata only. It never applies relocations, reads target contents to infer REL addends, calculates runtime load bias, resolves providers, or boots the game.
