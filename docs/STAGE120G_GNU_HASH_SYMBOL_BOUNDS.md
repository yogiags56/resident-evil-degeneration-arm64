# Stage 120G — GNU Hash Dynamic Symbol Bounds

## Goal

Support safe dynamic-symbol bounds for ELF32 EM_ARM binaries that provide DT_GNU_HASH without DT_HASH, and cross-check GNU and SysV metadata when both are present.

## Implementation

- Recognizes DT_GNU_HASH (0x6ffffef5) in PT_DYNAMIC.
- Reads the four-word header, ELF32 bloom words, bucket array, and chain array through unique PT_LOAD file-backed mappings.
- Requires nonzero bucket/bloom dimensions, power-of-two bloom_size, bounded dimensions, and bloom_shift below 32.
- Rejects buckets before symoffset, out-of-range chain references, unterminated chains, and chain counts beyond the safety cap.
- Derives the dynamic-symbol count from symoffset and the greatest terminated GNU hash chain.
- Validates each reachable chain hash against the symbol name and validates the corresponding bloom bits.
- Uses the GNU-derived count when DT_HASH is absent.
- When DT_HASH and DT_GNU_HASH are both present, rejects a GNU bound greater than SysV nchain and reports whether the GNU-derived bound equals or is lower than nchain. SysV nchain remains the complete table bound in that case.
- Resolves names only when DT_SYMTAB, DT_SYMENT, DT_STRTAB, and DT_STRSZ also map safely.
- The shared symbol parser is used by ordinary dynamic REL/RELA/JMPREL and APS2 audits.

## Usage

    python3 tools/guest_arm_dynamic_relocation_segment_audit.py /path/to/guest.elf --json /tmp/dynamic-relocations.json

## Validation

GitHub Actions Source checks run 42599061503 passed:
- 152 Python tests.
- GNU-hash-only fixture resolves foo without DT_HASH.
- Corrupt bloom filter rejected.
- Corrupt chain hash rejected.
- Non-power-of-two bloom size rejected.
- Bloom shift outside ELF32 word width rejected.
- Host C mock and native regression checks passed.

## Limitations

ELF32 EM_ARM only. This stage does not parse symbol versioning (DT_VERSYM/DT_VERDEF/DT_VERNEED), resolve providers, calculate load bias, apply relocations, or boot the original game. GNU hash gives a bounded dynamic symbol count only under the validated chain/bucket assumptions; no inference is made about game-specific binaries that are not present.
