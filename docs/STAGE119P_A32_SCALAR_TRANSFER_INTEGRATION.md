# Stage 119P — Isolated A32 scalar transfer integration

## Scope

Add an opt-in A32 instruction decoder helper that routes ordinary scalar data transfers through the Stage 119O checked scalar memory helper. The production `re_guest_cpu_step` decoder is not modified.

## Supported subset

- Condition AL only.
- Immediate offsets only.
- Pre-indexed addressing only.
- No writeback.
- Base and destination registers cannot be PC.
- Ordinary LDR/STR word and LDRB/STRB.
- LDRH/STRH, LDRSB, and LDRSH using the A32 extra load/store immediate encoding.
- Positive and negative immediate offsets.

## Semantics

- Word and byte operations delegate to Stage 119O scalar reads/writes.
- Halfword operations use the scalar 16-bit helpers.
- Signed byte and signed halfword loads sign-extend into the destination register.
- The destination register is updated only after a successful load.
- Store operations resolve and permission-check the complete access before mutation.
- Unaligned ordinary scalar operations inherit Stage 119N policy: strict by default; eligible in explicitly Normal memory only when SCTLR.A is clear.
- No change is made to instruction-fetch alignment.

## New files

- `runtime/guest/re_guest_cpu_arm_scalar.h`
- `runtime/guest/re_guest_cpu_arm_scalar.c`
- `tests/test_re_guest_cpu_arm_scalar.c`

Registered in Android CMake and `tools/run_tests.sh`.

## Regression coverage

- LDR/STR word and LDRB/STRB.
- LDRH/STRH, LDRSB, LDRSH.
- Positive and negative immediate offsets.
- Permissive unaligned word transfer in Normal memory.
- Strict-mode unaligned fault and destination preservation.
- Unmapped load/store fault behavior.
- Unsupported condition, post-index, writeback, PC operands, register offset, and unsupported halfword store variants.

## Safety boundary

This is still an isolated helper. It is not called by `re_guest_cpu_step`, does not replace Stage 119K/L helper, and does not alter production instruction behavior. No complete differential parity suite has been established yet. The guest SCTLR system-register model, reset state, MMU/page semantics, and precise exception model remain unimplemented.

## Next stage

Stage 119Q should expand differential tests against the legacy interpreter and verify architectural corner cases for immediate transfer addressing, condition handling, sign extension, memory faults, and CPU-state commit. Only after the evidence is sufficient should a narrow checked-step dispatch gate be considered. Do not connect this helper to production execution yet.
