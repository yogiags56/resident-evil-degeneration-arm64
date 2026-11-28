# Stage 125B.1 — ARM relocation symbol and control-flow audit

## Scope

Audited the bounded ELF32 ARM relocation preparation path and hardened symbol classification before producing relocation records. The existing relocation engine applies 32-bit data-word relocations only; it does not rewrite ARM/Thumb instruction encodings.

## Changes

- Reject reserved ELF section indices (SHN_LORESERVE and above), except SHN_ABS, instead of treating special indices such as SHN_COMMON as ordinary section-relative definitions.
- Reject GNU IFUNC symbols alongside TLS symbols. IFUNC requires resolver execution and is outside the current static relocation preparation contract.
- Add a dedicated control-flow relocation rejection status and fail closed for ARM PC24, PLT32, CALL, JUMP24 and Thumb CALL, JUMP24, JUMP19, JUMP6, JUMP11 and JUMP8 relocation types.
- Preserve transactional output semantics: rejected records do not publish partially prepared relocation arrays.
- Add regression coverage for SHN_COMMON, IFUNC and every currently classified branch relocation type.

## Supported relocation subset

The planner continues to accept R_ARM_RELATIVE, R_ARM_ABS32, R_ARM_GLOB_DAT and R_ARM_JUMP_SLOT. It explicitly rejects unsupported relocation types, versioned dynamic symbol tags, TLS, IFUNC, unsupported special section indices and instruction-encoding relocations.

## Known gaps

This is not a complete ARM EABI dynamic linker. Implementing branch relocation requires opcode validation, correct ARM/Thumb PC bias, displacement alignment/range checks, instruction-bitfield encoding, Thumb-2 wide instruction validation, and ARM/Thumb interworking policy. Other remaining gaps include COPY relocations, TLS models, GNU symbol version decoding, GNU hash-only symbol bounds, REL/RELA semantic cross-checks, PLT/GOT conventions, IFUNC resolver policy, and real payload relocation inventory. These must be driven by actual ELF metadata, not assumptions.

## Validation

Stage 125A.6 Host Source Checks passed on commit `2de75cf6c3a67ddb0f5dc9dae2a0ef2920235d60`. Android build at that commit was blocked during Gradle dependency resolution by Maven Central HTTP 429 throttling. Stage 125B.1 host and Android workflows are pending on the latest commits.
