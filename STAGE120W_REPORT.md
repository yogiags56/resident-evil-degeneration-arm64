# Stage 120W Report — ARM32 Relocation Application

Status: implementation committed; CI validation pending.

## Changes
- Added a bounded relocation application API for R_ARM_RELATIVE, R_ARM_ABS32,
  R_ARM_GLOB_DAT, and R_ARM_JUMP_SLOT.
- Supports REL implicit addends for RELATIVE and ABS32 and explicit RELA addends.
- Requires caller-supplied resolved symbol values for symbol-based relocations.
- Preflights all targets, alignment, permissions, symbol requirements, supported
  types, and duplicate targets before writing any relocation result.
- Added native C regression tests and integrated them into tools/run_tests.sh.
- Added CI SHA-256 provenance coverage.

## Validation
Pending GitHub Actions. No pass is claimed until full Python discovery,
host mock compilation, and native C regression checks succeed.

## Scope
This is not a dynamic linker. No ELF table parsing, symbol lookup, dependency
loading, symbol versioning, COPY/TLS/IFUNC, load-bias selection, PLT/lazy
binding, or guest execution is implemented.
