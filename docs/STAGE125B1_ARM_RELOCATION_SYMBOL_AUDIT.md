# Stage 125B.1 — ARM relocation symbol compatibility audit

## Scope

Audited the existing bounded ELF32 ARM relocation preparation path and hardened symbol classification before producing relocation records.

## Changes

- Reject reserved ELF section indices (SHN_LORESERVE and above), except SHN_ABS, instead of incorrectly treating special indices such as SHN_COMMON as ordinary section-relative definitions.
- Reject GNU IFUNC symbols alongside TLS symbols. IFUNC requires resolver execution and is outside the current static relocation preparation contract.
- Preserve transactional output semantics: rejected records do not publish partially prepared relocation arrays.
- Added regression coverage for SHN_COMMON and GNU IFUNC rejection.

## Existing supported relocation subset

The current planner accepts R_ARM_RELATIVE, R_ARM_ABS32, R_ARM_GLOB_DAT and R_ARM_JUMP_SLOT. It explicitly rejects unsupported relocation types, versioned dynamic symbol tags, TLS, IFUNC and unsupported special section indices.

## Known gaps

This is not a complete ARM EABI dynamic linker. Remaining work includes ARM/Thumb branch relocations and interworking, COPY relocations, TLS models, GNU symbol version decoding, GNU hash-only symbol bounds, REL/RELA semantic cross-checks, PLT/GOT conventions, IFUNC resolver policy, and real payload relocation inventory. These must be driven by actual ELF metadata, not assumed.

## Validation

Host source checks previously passed at Stage 125A.6 commit `2de75cf6c3a67ddb0f5dc9dae2a0ef2920235d60`. Android build at that commit was blocked by Maven Central HTTP 429 dependency download throttling, not a reported C compilation error. Stage 125B.1 CI is pending on the new commits.
