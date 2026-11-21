# Stage 119X — ELF32 ARM Relocation Audit

## Goal

Add a read-only parser for ELF32 EM_ARM relocation sections. It associates each REL/RELA section with its target section and linked symbol table, validates bounds and indices, and reports relocation types and symbol references.

## Usage

    python3 tools/guest_arm_relocation_audit.py /path/to/guest.elf --json /tmp/relocations.json

## Supported records

- SHT_REL: r_offset and r_info; the implicit addend remains in target bytes and is not decoded.
- SHT_RELA: r_offset, r_info, and signed explicit addend.
- ARM relocation type names are provided for common call, jump, absolute, relative, Thumb, and instruction-group relocations. Unknown types remain visible as numeric R_ARM_UNKNOWN_n.
- sh_link selects the symbol table; sh_info selects the relocation target section.
- ET_REL relocation offsets are section-relative. ET_EXEC/ET_DYN offsets are translated from target virtual addresses to section-relative offsets.

## Safety and limits

The tool never applies relocations, modifies the input, or executes the ELF. It reports metadata only. ELF32 EM_ARM and conventional SHT_SYMTAB are supported; dynamic symbol tables, extended section indices, packed RELR, Android APS2, and ELF64/AArch64 relocations remain deferred. REL implicit addends are deliberately not guessed.

## Tests

Synthetic ELF fixtures cover REL and RELA, R_ARM_CALL/R_ARM_JUMP24 names, unknown relocation preservation, symbol-index bounds, section-link bounds, target-offset bounds, and signed RELA addends.
