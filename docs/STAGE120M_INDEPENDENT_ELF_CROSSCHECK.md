# Stage 120M — Independent ELF Cross-Check and COPY Symbol-Size Validation

## Goal

Cross-check section-backed ELF32 EM_ARM relocation offsets and types against an independent GNU readelf or LLVM llvm-readelf implementation. Resolve R_ARM_COPY target width only when the referenced symbol's st_size is available, nonzero, defined, and tied to a known symbol-table namespace.

## Independent tool

New tool: tools/guest_arm_relocation_independent_crosscheck.py

Usage:

    python3 tools/guest_arm_relocation_independent_crosscheck.py guest.elf
    python3 tools/guest_arm_relocation_independent_crosscheck.py guest.elf --tool /usr/bin/readelf --json /tmp/readelf-crosscheck.json

Behavior:
- Prefers GNU readelf, then llvm-readelf.
- Runs readelf -rW in read-only inspection mode.
- Compares the multiset of relocation offsets and recognized AAELF32 type names.
- Reports missing and unexpected records, plus external relocation names that this registry cannot compare.
- Returns an explicit skipped status if neither independent tool is installed.
- Does not make readelf/LLVM a runtime dependency.

## R_ARM_COPY size provenance

The conventional SHT_REL/SHT_RELA parser now preserves symbol st_size and definition state. The PT_DYNAMIC symbol resolver already bounds the dynamic symbol table using hash metadata; its relocation records now carry symbol size and definition state as well.

The unified target audit resolves R_ARM_COPY width only when all conditions hold:
- symbol index is a nonzero integer;
- symbol is defined;
- symbol st_size is a positive integer;
- symbol-table namespace is explicitly known.

Otherwise the width remains unknown with width_source=unresolved_copy_symbol_size. APS2 records without proven symbol-size provenance remain unresolved. This does not copy bytes or perform a relocation.

## Tests

Tests cover readelf row parsing, matching and mismatching offset/type inventories, unsupported external type preservation, missing-tool skip behavior, real readelf invocation when available, and COPY width resolution/rejection for missing, zero, undefined, or unnamespaced symbols.

## Safety boundaries

- Section-backed ELF32 EM_ARM SHT_REL/SHT_RELA only for the independent comparison.
- Symbol names, provider selection, and REL implicit addends are not cross-validated by readelf in this stage.
- No target bytes or instruction encodings are read or modified by this audit.
- No relocation is applied and no game boot is claimed.
