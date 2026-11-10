# Roadmap through Stage 120J

Stage 120I completed ELF32 RELR direct/bitmap decoding and unified reconciliation. Stage 120J adds PT_LOAD target range classification.

## Stage 120J — Relocation Target Segment and Memory-Range Validation

- Maps each virtual relocation target against PT_LOAD p_memsz and p_filesz ranges.
- Distinguishes file-backed, BSS/zero-fill, file-to-BSS crossing, segment-end crossing, and out-of-range targets.
- Reports ambiguous overlapping PT_LOAD mappings.
- Applies known ELF32 ARM relocation access widths only; unknown relocation types remain unknown.
- Treats R_ARM_NONE as a no-target-access relocation.
- GitHub Actions Source checks passed: 188 Python tests and host C/native regressions (run 42601733876).
- No target bytes are read or modified; no load bias or game boot is claimed.

## Stage 120K — ARM ELF32 relocation semantics and ABI validation

- Added a 60-entry AAELF32-backed relocation metadata registry.
- Records target storage width, place alignment, class, relocation category, and semantic summary.
- Distinguishes ABS8 (1 byte), ABS16 (2 bytes), ABS32 (4 bytes), Thumb16 (2 bytes), Thumb32 (4 bytes), variable-size COPY, and NONE (zero access).
- Preserves unknown relocation codes and marks TARGET1 platform-specific.
- Integrates registry with target range checks, dynamic relocation naming, and per-type coverage counts.
- GitHub Actions run 42602775178 passed 199 Python tests plus host C/native regression checks.
- No target bytes or instruction bits are modified; no relocation is applied.

## Next — Stage 120L: Relocation symbol and target contract cross-validation

1. Cross-check symbol requirements, target requirements, and static/dynamic applicability against AAELF32.
2. Separate storage width from instruction encoding and runtime write semantics.
3. Add independent-tool fixtures for ARM, Thumb16, Thumb32, TLS, and dynamic relocations.
4. Validate R_ARM_COPY against symbol st_size when safely available.
5. Preserve unknown, reserved, private, and platform-specific types.
6. Keep the pipeline read-only.
