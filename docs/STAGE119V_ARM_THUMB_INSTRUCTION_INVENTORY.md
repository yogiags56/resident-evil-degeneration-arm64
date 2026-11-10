# Stage 119V — ARM/Thumb Instruction Inventory

## Goal

Add an optional, read-only instruction inventory for a locally supplied ARM32 ELF. The tool scans only sections marked SHF_EXECINSTR and uses Capstone to decode each section in an explicitly selected mode.

## Usage

Install a reviewed/pinned Capstone version in the analysis environment, then run:

    python3 tools/guest_arm_instruction_inventory.py /path/to/guest.elf --mode arm --json /tmp/arm-inventory.json
    python3 tools/guest_arm_instruction_inventory.py /path/to/guest.elf --mode thumb --json /tmp/thumb-inventory.json

Do not use an automatic mode. A single global mode is a deliberate limitation: ARM ELF mapping symbols $a, $t, and $d are not yet interpreted, so mixed-mode sections must not be decoded as one mode. Run per-mode only when the section's mode is independently established, or wait for mapping-symbol support.

## Report semantics

- Raw executable bytes: total bytes in executable sections.
- Decoded bytes: bytes consumed by successful decoder instructions.
- Undecoded bytes/ranges: bytes for which the selected decoder/mode did not return an instruction.
- Mnemonic counts: counts of decoded instructions by mnemonic.
- Sample instructions: capped examples to keep reports manageable.
- Every report records input SHA-256, Capstone version, selected mode, and section addresses/offsets.

An undecoded range is not necessarily invalid code; it may indicate a wrong mode, data embedded in a code section, unsupported ISA extension, or corrupt/truncated input. Attribute declarations are not proof that a feature is actually used.

## Limits

The first implementation accepts conventional ELF32 EM_ARM inputs only. It does not handle ELF64/AArch64, mixed-mode mapping symbols, relocations, IT-block semantic validation, indirect control flow, or complete ISA conformance. The input file is never executed or modified, and is not committed.

## Tests

A mocked Capstone engine tests ARM/Thumb mode selection, instruction byte accounting, mnemonic counts, undecoded ranges, non-ARM rejection, and non-executable section filtering. Real decoder behavior requires Capstone installed in the analysis environment; host CI validates deterministic accounting via the mock.
