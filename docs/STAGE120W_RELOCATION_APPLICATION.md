# Stage 120W — ELF32 ARM Relocation Application Engine

## Objective
Apply a deliberately small, explicit subset of already-audited ELF32 ARM
relocations to mapped guest memory, with all-or-nothing preflight.

## Supported operations
- R_ARM_RELATIVE (23): B + A; symbol index must be zero.
- R_ARM_ABS32 (2): S + A.
- R_ARM_GLOB_DAT (21): S.
- R_ARM_JUMP_SLOT (22): S.

For REL records, implicit addends for RELATIVE and ABS32 are read from the
original target word. For RELA records, the caller supplies the explicit
32-bit addend. GLOB_DAT and JUMP_SLOT use the supplied resolved symbol value.

## Safety contract
- Only four-byte-aligned, four-byte guest targets are accepted.
- All target words must be mapped and writable.
- Symbol-based relocations require a caller-supplied resolved symbol value.
- R_ARM_RELATIVE requires symbol index zero.
- Duplicate target words and unsupported relocation types are rejected.
- Every entry is preflighted and every result computed before any target word
  is modified.
- The caller must provide exclusive access to guest memory during the call.

## Limits
This is not a dynamic linker. It does not parse ELF tables, resolve symbols,
load shared libraries, handle symbol versions, apply COPY/TLS/IFUNC relocations,
select load bias, or implement PLT/lazy binding. Symbol resolution must be
performed by a separate trusted layer. 32-bit word arithmetic is intentional.
