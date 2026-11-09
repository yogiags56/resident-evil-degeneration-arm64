# Roadmap Through Stage 120E

## Current milestone — Stage 120E: Android APS2 Packed Relocations

Stage 120E adds a read-only ELF32 EM_ARM parser for Android APS2 packed REL/RELA streams. It consumes dynamic tags from PT_DYNAMIC and maps packed data through file-backed PT_LOAD ranges. It reuses Stage 120D's bounded SysV hash symbol correlation and preserves unknown ARM relocation types.

### Completed in this milestone

- Bounded signed 32-bit SLEB128 decoding.
- APS2 stream header, relocation count, initial offset, group size, and group flag decoding.
- Grouped and per-entry relocation info and offset-delta handling.
- Grouped and per-entry addend delta handling for RELA.
- Android DT_ANDROID_REL/DT_ANDROID_RELSZ and DT_ANDROID_RELA/DT_ANDROID_RELASZ handling.
- Safety checks for malformed streams, integer overflow, group/count mismatch, size caps, tag pairing, and file-backed segment mapping.
- Synthetic regression tests and stage documentation.

### Next — Stage 120F: APS2 / ordinary relocation inventory reconciliation

1. Parse ordinary REL/RELA and PLT tables alongside APS2 tables from one ELF.
2. Identify exact duplicate ranges and record overlaps without silently double-counting.
3. Compare normalized relocation tuples (target, type, symbol index, addend) across representations.
4. Preserve source provenance and report discrepancies.
5. Add fixtures for exact duplicates, partial overlaps, and distinct tables.
6. Keep relocation application and runtime loader behavior disabled.

### Later milestones

- Stage 121: GNU hash parsing and bounded dynamic-symbol resolution fallback.
- Stage 122: ARM relocation semantics research and relocation dry-run model (no writes).
- Stage 123: XE3U image mapping/load-bias analysis.
- Stage 124: import table binding audit and verified provider mapping.
- Later: renderer, audio, input, lifecycle integration, and physical ARM64 device boot/gameplay validation.

## Non-goals

No proprietary game assets or executable are committed. No relocation is applied, no guest import is activated, and no playable-game claim is made by Stage 120E.
