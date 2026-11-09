# Roadmap through Stage 120I

Stage 120H added bounded dynamic symbol version metadata and version-aware relocation identity. See STAGE120H_REPORT.md.

## Stage 120I — ELF32 RELR Relative Relocation Decoding

- Parses DT_RELR/DT_RELRSZ/DT_RELRENT through PT_DYNAMIC/PT_LOAD.
- Decodes direct offsets and bitmap entries for 32-bit words.
- Validates alignment, file-backed table mapping, malformed bitmaps, and 32-bit overflow.
- Adds RELR target records to unified reconciliation without inventing symbol indices or addends.
- GitHub Actions Source checks passed: 177 Python tests and host C/native regressions (run 42600655563).
- No relocation application, target dereference, Android device execution, or game boot is claimed.

## Stage 120J — Relocation target segment and memory-range validation

- Maps each virtual relocation target against PT_LOAD p_memsz and p_filesz ranges.
- Distinguishes file-backed, BSS/zero-fill, file-to-BSS crossing, segment-end crossing, and out-of-range targets.
- Reports ambiguous overlapping PT_LOAD mappings.
- Applies known ELF32 ARM relocation access widths only; unknown relocation types remain unknown.
- Treats R_ARM_NONE as a no-target-access relocation.
- GitHub Actions Source checks passed: 187 Python tests and host C/native regressions (run 42601441300).
- No target bytes are read or modified; no load bias or game boot is claimed.

## Next — Stage 120K: Relocation semantic coverage and ARM ABI validation

1. Verify supported R_ARM relocation types and widths against ARM ELF ABI references.
2. Expand known types only with documented semantics and access widths.
3. Add independent-tool cross-check fixtures where available.
4. Preserve unknown and processor-specific types explicitly.
5. Keep relocation targets read-only and load-bias calculation disabled.
