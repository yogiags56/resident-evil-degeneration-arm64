# Stage 125B.2 — A32 ARM branch relocation encoder

## Scope

Introduces a standalone, bounded encoder for A32 ARM-state B/BL instruction relocations. It is intentionally not wired into the generic relocation apply path yet; Thumb relocations remain fail-closed.

## Supported relocation types

- R_ARM_PC24 (1)
- R_ARM_PLT32 (27)
- R_ARM_CALL (28)
- R_ARM_JUMP24 (29)

## Validation and encoding

- Validates A32 branch opcode class and rejects the reserved condition NV encoding.
- Enforces BL for R_ARM_CALL and B for R_ARM_JUMP24.
- Rejects Thumb-state targets rather than silently emitting a non-interworking branch.
- Decodes signed REL imm24 addends and accepts signed byte addends for RELA.
- Applies A32 PC bias (P + 8), checks 4-byte alignment and signed 26-bit byte displacement range, then replaces only imm24.
- Does not publish the output instruction on failure.

## Tests

Host regression test covers positive and negative branch displacements, REL and RELA addends, opcode/link-bit mismatches, unsupported Thumb type, alignment, interworking, PC overflow and invalid flags.

## Limitations

This is a helper only; it is not yet connected to the relocation planner or memory transaction. No Thumb-1/Thumb-2 encoder, veneers, PLT stub generation, or automatic interworking is implemented. The next step is independent validation against ARM ABI fixtures and then transactional integration.

## CI

Host Source Checks and Android ARM64 native build are running for the new helper and test at commit `0dc992c67e4e6b97dc9c7a3e4536f04e78819fe3`.
