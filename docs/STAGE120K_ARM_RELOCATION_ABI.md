# Stage 120K — ARM ELF32 Relocation Semantics and ABI Metadata

## Goal

Replace broad, guessed ARM relocation target widths with an explicit metadata registry grounded in AAELF32. Keep this stage read-only: it describes relocation semantics and target storage; it does not calculate or apply relocations.

## Registry

tools/guest_arm_relocation_abi.py provides 60 explicitly catalogued ELF32 ARM relocation codes. Each record carries:
- ABI relocation name;
- target storage width in bytes (not relocation record size);
- required place alignment;
- instruction/data class;
- static, dynamic, deprecated, or platform-specific classification;
- concise semantic description.

The inventory includes byte/halfword/word data relocations, ARM instruction relocations, Thumb16 and Thumb32 instruction relocations, TLS relocations, and special cases.

Important distinctions:
- R_ARM_ABS8 writes 1 byte.
- R_ARM_ABS16 writes 2 bytes.
- R_ARM_ABS32 writes 4 bytes.
- Thumb16 instruction relocations occupy 2 bytes.
- Thumb32 instruction relocations occupy 4 bytes and generally have 2-byte place alignment.
- R_ARM_COPY has a variable write size derived from the referenced symbol's st_size; its width remains unknown until that metadata is available.
- R_ARM_NONE has zero target access.
- R_ARM_TARGET1 remains platform-specific.
- Unlisted relocation codes remain unknown; no width is guessed.

## Integration

The target-range auditor now consumes the registry for access widths and alignment. Each target record carries abi_spec, access_width, required_alignment, and alignment_valid. Dynamic relocation names are sourced from the same registry. Unified reconciliation reports the number of known and unknown relocation records and per-type counts.

Usage:

    python3 tools/guest_arm_relocation_reconciliation.py /path/to/guest.elf --json /tmp/relocation-reconciliation.json

## Validation

Tests cover distinct 1/2/4-byte data widths, Thumb16 and Thumb32 storage, Thumb32 place alignment, symbol-size-dependent COPY width, R_ARM_NONE, platform-specific TARGET1, unknown relocation preservation, registry completeness, and alignment metadata.

## Normative reference

ARM ABI — AAELF32, ELF for the Arm Architecture:
https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst

The registry is a bounded audit catalogue, not a complete relocation linker. Any unlisted or insufficiently specified relocation stays unknown.

## Deliberate limitations

- ELF32 EM_ARM only.
- No instruction bits, target bytes, implicit addends, or GOT/PLT contents are read or modified.
- No load bias or final relocation value is calculated.
- R_ARM_COPY width is unresolved until symbol size is available.
- Platform-specific and unlisted relocation types are not silently treated as universal.
- No Android runtime execution or original game boot is claimed.
