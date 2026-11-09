# Stage 120C — PT_DYNAMIC / PT_LOAD Parser

## Goal

Parse ELF32 EM_ARM program headers without requiring section headers. Map virtual addresses only through file-backed PT_LOAD ranges and decode PT_DYNAMIC entries.

## Usage

    python3 tools/guest_arm_pt_dynamic_audit.py /path/to/stripped.elf --json /tmp/pt-dynamic.json

## Features

- Reads ELF32 EM_ARM program headers in little- or big-endian form.
- Supports ELF files with section-header count zero.
- Validates segment file size, memory size, file bounds, and alignment.
- Maps a virtual range only when exactly one PT_LOAD file-backed range contains it.
- Parses PT_DYNAMIC entries through DT_NULL.
- Resolves DT_NEEDED, DT_SONAME, DT_RPATH, and DT_RUNPATH only when DT_STRTAB and DT_STRSZ are both present.

## Boundaries

Dynamic symbol and relocation tables are reported as tags but are not decoded here. BSS-only virtual addresses are never treated as file bytes. PT_DYNAMIC virtual-address cross-checking, section-vs-segment comparison, and APS2/RELR parsing are future work. No relocations are applied and no dependency provider is inferred.

## Tests

Synthetic fixtures cover section-header-free ELF, PT_LOAD file-backed mapping, BSS rejection, DT_NEEDED strings, missing DT_NULL, malformed segment sizes, and big-endian parsing.
