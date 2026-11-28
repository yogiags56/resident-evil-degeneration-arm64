# Stage 125B.1 — ARM relocation symbol and control-flow audit

## Scope

Audited the bounded ELF32 ARM relocation preparation and application paths. The engine currently applies 32-bit data-word relocations; it does not rewrite ARM/Thumb instruction encodings.

## Changes

- Reject reserved ELF section indices (SHN_LORESERVE and above), except SHN_ABS.
- Reject GNU IFUNC and TLS symbols; resolver execution and TLS models are not implemented.
- Planner returns a dedicated control-flow status for ARM PC24, PLT32, CALL, JUMP24 and Thumb CALL, JUMP24, JUMP19, JUMP6, JUMP11 and JUMP8.
- Independently harden the relocation apply boundary: the same ARM/Thumb control-flow relocation types are rejected before any target lookup or write, even if a malformed/preconstructed relocation record bypasses the planner.
- Preserve transactional output and memory semantics. Rejected planner records do not publish output; rejected apply records do not modify guest memory.
- Add regression coverage for SHN_COMMON, IFUNC and all listed branch relocation types at both planner and apply boundaries.

## Supported relocation subset

The planner and apply engine support R_ARM_RELATIVE, R_ARM_ABS32, R_ARM_GLOB_DAT and R_ARM_JUMP_SLOT only. This is a bounded subset, not a complete ARM EABI dynamic linker.

## Remaining work

Branch relocation implementation still requires ARM/Thumb opcode validation, correct PC bias, displacement alignment/range checks, instruction bitfield encoding, Thumb-2 wide instruction validation and interworking policy. Additional gaps include COPY, TLS, GNU symbol versioning, GNU hash bounds, REL/RELA semantic cross-checks, PLT/GOT conventions, IFUNC policy and actual payload relocation inventory.

## Validation

Commit `32ea7b753d518d808fc701ae99b643f4b8063458` passed both Host Source Checks and Android ARM64 Native Build. Stage 125B.1 apply-boundary hardening CI is pending on commit `1469efb34371795f237a25ac8a3bd0ceac987244`.
