# Roadmap through Stage 120J

Stage 120I completed ELF32 RELR direct/bitmap decoding and unified reconciliation. Stage 120J adds PT_LOAD target range classification.

## Stage 120J — Relocation Target Segment and Memory-Range Validation

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
