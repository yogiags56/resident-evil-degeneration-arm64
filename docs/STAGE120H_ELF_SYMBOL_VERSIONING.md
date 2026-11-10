# Stage 120H — ELF Dynamic Symbol Version Metadata

## Goal

Decode ELF32 dynamic symbol version metadata so a relocation's symbol identity includes ABI version context, not merely a symbol-table index or name.

## Dynamic tags

- DT_VERSYM (0x6ffffff0): per-dynamic-symbol version index array.
- DT_VERDEF (0x6ffffffc) and DT_VERDEFNUM (0x6ffffffd): version definitions exported by the object.
- DT_VERNEED (0x6ffffffe) and DT_VERNEEDNUM (0x6fffffff): version requirements imported from dependencies.

## Implementation

- Parses Elf32_Verdef and linked Elf32_Verdaux records.
- Parses Elf32_Verneed and linked Elf32_Vernaux records.
- Reads the version symbol array using the bounded dynamic symbol count established by SysV DT_HASH or GNU DT_GNU_HASH.
- Resolves version names and requirement library names through DT_STRTAB/DT_STRSZ.
- Preserves version index, kind (local/global/definition/requirement), name, library, flags/hash metadata, and DT_VERSYM hidden bit.
- Validates paired address/count tags, bounded record counts, version structure version, reserved indices, auxiliary offsets, final next offsets, duplicate indices, and string termination.
- Integrates version identity into dynamic REL/RELA/JMPREL and APS2 reports.
- Relocation reconciliation includes version identity in semantic matches; matching numeric symbol indices are not treated as proof of identical ABI identity.

## Usage

    python3 tools/guest_arm_dynamic_relocation_segment_audit.py /path/to/guest.elf --json /tmp/dynamic-relocations.json
    python3 tools/guest_arm_aps2_relocation_audit.py /path/to/guest.elf --json /tmp/aps2-relocations.json
    python3 tools/guest_arm_relocation_reconciliation.py /path/to/guest.elf --json /tmp/relocation-reconciliation.json

## Validation

GitHub Actions run 42600277144 passed:
- 165 Python tests.
- GNU-hash-only and SysV/GNU cross-check coverage remains passing.
- Version requirement and definition fixtures.
- DT_VERSYM hidden-bit preservation.
- Missing version index, malformed auxiliary offset, and unterminated version string rejection.
- Version-aware duplicate and cross-namespace reconciliation.
- Host C mock compilation and native regression checks.

## Deliberate limitations

- ELF32 EM_ARM only.
- Does not resolve a version requirement to a real installed shared-library provider.
- Does not decide whether a specific provider satisfies a version requirement.
- Does not calculate load bias, apply relocations, dereference relocation targets, or boot the game.
- Weak symbol resolution and runtime symbol lookup order remain outside this stage.
- RELR decoding is deferred to Stage 120I.
