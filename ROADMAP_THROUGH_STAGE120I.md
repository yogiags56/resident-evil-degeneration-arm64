# Roadmap through Stage 120I

Stage 120H added bounded dynamic symbol version metadata and version-aware relocation identity. See STAGE120H_REPORT.md.

## Stage 120I — ELF32 RELR Relative Relocation Decoding

- Parses DT_RELR/DT_RELRSZ/DT_RELRENT through PT_DYNAMIC/PT_LOAD.
- Decodes direct offsets and bitmap entries for 32-bit words.
- Validates alignment, file-backed table mapping, malformed bitmaps, and 32-bit overflow.
- Adds RELR target records to unified reconciliation without inventing symbol indices or addends.
- GitHub Actions Source checks passed: 177 Python tests and host C/native regressions (run 42600655563).
- No relocation application, target dereference, Android device execution, or game boot is claimed.

## Next — Stage 120J: Relocation target segment and memory-range validation

1. Map REL/RELA/JMPREL/APS2/RELR targets to PT_LOAD memory ranges.
2. Distinguish file-backed bytes from zero-fill/BSS.
3. Validate target width and alignment only for known relocation types.
4. Report ambiguous mappings and out-of-range targets.
5. Preserve unknown relocation types without guessing.
6. Add boundary and overlapping-segment fixtures.
7. Keep target memory read-only and load-bias calculation disabled.
