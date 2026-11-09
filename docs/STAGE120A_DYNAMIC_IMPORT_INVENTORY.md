# Stage 120A — Dynamic Import Inventory

## Goal

Read ELF32 EM_ARM dynamic-link metadata without loading or executing the input. Inventory named undefined symbols, DT_NEEDED dependencies, dynamic tags, and SHT_REL/SHT_RELA sections linked to SHT_DYNSYM.

## Usage

    python3 tools/guest_arm_dynamic_import_inventory.py /path/to/guest.elf --json /tmp/dynamic-imports.json

## Output

- Dynamic symbol tables and named undefined symbols, retaining symbol index, binding, type, visibility, and section index.
- Dynamic section entries with known tag names and string values for DT_NEEDED, DT_SONAME, DT_RPATH, and DT_RUNPATH.
- Dependencies listed by DT_NEEDED.
- Dynamic relocation entries joined to dynamic symbols by symbol index, including relocation type and explicit RELA addend when present.
- Unknown dynamic tags and ARM relocation numbers are retained numerically.

## Boundaries

This stage supports ELF32 EM_ARM section-based metadata using conventional SHT_DYNSYM, SHT_DYNAMIC, SHT_REL, and SHT_RELA sections. It does not yet handle stripped files that expose dynamic data only through PT_DYNAMIC, Android packed APS2 relocations, RELR, symbol version decoding, or GNU hash traversal. Undefined names are candidates for external resolution, not proof of Marmalade/S3E API identity. No relocation is applied and no import resolver is modified.

## Tests

Synthetic ELF fixtures cover DT_NEEDED, undefined dynamic symbols, R_ARM_JUMP_SLOT symbol correlation, unknown dynamic tags, invalid dynsym string-table links, and missing DT_NULL termination.
