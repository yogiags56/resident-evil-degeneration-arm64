# Stage 120D — Dynamic Relocation Segment Mapping

## Goal

Decode ELF32 EM_ARM dynamic REL/RELA records through PT_DYNAMIC and PT_LOAD, independently of section headers.

## Usage

    python3 tools/guest_arm_dynamic_relocation_segment_audit.py /path/to/guest.elf --json /tmp/dynamic-relocations.json

## Features

- Parses DT_REL/DT_RELSZ/DT_RELENT and DT_RELA/DT_RELASZ/DT_RELAENT.
- Parses DT_JMPREL/DT_PLTRELSZ/DT_PLTREL as a distinct relocation table.
- Maps every table through a unique file-backed PT_LOAD range.
- Validates entry sizes, table lengths, and bounded input size.
- Decodes ARM relocation type and symbol index.
- Resolves symbol names only when a valid SysV DT_HASH, DT_SYMTAB, DT_SYMENT, DT_STRTAB, and DT_STRSZ set is available.
- Preserves unknown relocation types.

## Deliberate limitations

- ELF32 EM_ARM only.
- SysV DT_HASH is required for safe dynamic-symbol table bounds. GNU-hash-only inputs keep symbol names unresolved until a dedicated GNU hash parser exists.
- DT_JMPREL may overlap DT_REL/DT_RELA; this stage reports declared tables separately and does not deduplicate them.
- Relocation targets are not dereferenced or modified. No runtime relocation is applied.
- No library provider or Marmalade/S3E import is inferred.
- Android APS2 packed relocation and RELR formats are deferred to Stage 120E.

## Tests

Synthetic section-header-free fixtures cover REL and PLT relocation mapping, symbol correlation, missing hash fallback, malformed tag combinations, invalid PLT kind, file-backed range enforcement, invalid entry sizes, and big-endian ELF.
