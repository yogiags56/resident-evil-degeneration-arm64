# Stage 120N — Packed and Dynamic Relocation Metadata Parity

## Goal

Carry bounded dynamic-symbol metadata from the APS2 decoder into the unified relocation inventory, preserve a proven symbol-table namespace, and verify that the same COPY symbol-size rules used for ordinary dynamic relocations apply to packed Android relocations.

## APS2 symbol metadata

When bounded DT_HASH or DT_GNU_HASH plus DT_SYMTAB/DT_STRTAB metadata establishes the dynamic symbol table, each APS2 relocation now carries:
- symbol name;
- symbol size (st_size);
- defined/undefined state;
- symbol section index;
- symbol version metadata.

If the symbol table cannot be bounded, these fields remain null. The normalized namespace is explicitly marked DT_SYMTAB:unresolved rather than inferred from an address alone. If symbol resolution succeeds, the namespace is keyed by the verified DT_SYMTAB virtual address.

The unified target audit can resolve R_ARM_COPY width for APS2 only when the nonzero symbol index, defined state, positive st_size, and known symbol namespace are all present. Otherwise width remains unknown.

## APS2 decoding correctness correction

Cross-validation against the Android Bionic linker source identified incorrect assumptions in the previous decoder:
- r_info is an absolute ELF relocation field value, not a cumulative delta.
- GROUPED_BY_INFO supplies one absolute r_info value reused for the entire group.
- Without GROUPED_BY_INFO, each relocation supplies its own absolute r_info value.
- GROUPED_BY_ADDEND supplies one addend delta applied once per group.
- HAS_ADDEND without GROUPED_BY_ADDEND supplies an addend delta per relocation.
- For RELA groups without HAS_ADDEND, the group's addend is zero.
- SLEB128 signed decoding is converted back to the ELF32 32-bit r_info bit pattern. Initial offsets are also interpreted as unsigned ELF32 addresses, including values with the high bit set.

The decoder now follows these rules and rejects invalid REL/RELA flag combinations, malformed streams, overflow, and trailing data.

## Tests

Synthetic fixtures validate:
- APS2 symbol size and definition metadata from bounded SysV DT_HASH;
- unresolved metadata without bounded symbol hashes;
- COPY width resolution only with complete provenance;
- grouped r_info reuse and ungrouped absolute r_info values;
- grouped addend applied once per group;
- per-relocation addend deltas;
- addend reset for groups without HAS_ADDEND;
- signed SLEB128 r_info bit-pattern preservation;
- existing PT_DYNAMIC, APS2, RELR, reconciliation, and target-range regressions.

## Reference implementation

Android Bionic's `linker_reloc_iterators.h` defines the APS2 group flags and decoding behavior:
https://android.googlesource.com/platform/bionic/+/refs/heads/main/linker/linker_reloc_iterators.h

LLVM's APS2 format description:
https://reviews.llvm.org/D39152

## Safety boundaries

- ELF32 EM_ARM only.
- No symbol provider is inferred and no dynamic linker resolution is simulated.
- No relocation target bytes are read, changed, or applied.
- APS2 remains a read-only metadata inventory.
- No Android device build or original game boot is claimed by this stage.
