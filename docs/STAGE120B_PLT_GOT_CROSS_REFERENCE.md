# Stage 120B — PLT/GOT Cross-Reference

## Goal

Build a conservative PLT/GOT-oriented view over section-backed ELF32 ARM dynamic relocation records. Identify R_ARM_JUMP_SLOT entries and retain the exact undefined dynamic symbol each entry references.

## Usage

    python3 tools/guest_arm_plt_got_audit.py /path/to/guest.elf --json /tmp/plt-got.json

## Output

- DT_PLTGOT, DT_JMPREL, DT_PLTRELSZ, and DT_PLTREL values when present.
- REL/RELA interpretation of DT_PLTREL when known.
- R_ARM_JUMP_SLOT entries, their target slot offsets, and dynamic symbol identity.
- Undefined/defined status of the referenced symbol.
- Explicit unknown values when PLT tags are absent.

## Boundaries

This stage cross-references section-backed relocation reports only. It does not decode PLT instruction stubs, independently map DT_JMPREL through PT_LOAD, read or infer resolved GOT contents, apply relocations, or identify an import provider. PT_DYNAMIC-only stripped ELF and Android packed relocation formats remain future work.

## Tests

Synthetic fixtures cover JUMP_SLOT-to-undefined-symbol correlation, absent PLT tag handling, and empty JUMP_SLOT inventories.
