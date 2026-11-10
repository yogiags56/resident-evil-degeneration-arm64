# Stage 119Q — A32 scalar differential reference tests

## Goal

Add an independent, deterministic reference model for the supported Stage 119P A32 scalar transfer subset. This stage increases confidence in instruction decoding and scalar memory semantics without changing production CPU execution.

## Test design

New test: `tests/test_re_guest_cpu_arm_scalar_differential.c`.

- Uses a deterministic xorshift32 generator.
- Runs 512 reproducible instruction/data cases.
- Uses a separate byte-array reference model, not the scalar helper, to calculate addresses, little-endian loads/stores, and sign extension.
- Compares all 16 CPU registers and the entire 256-byte memory region after each operation.
- Covers LDR/STR word, LDRB/STRB, LDRH/STRH, LDRSB, and LDRSH.
- Varies base/destination registers, positive and negative immediate offsets, and memory contents.
- Uses naturally aligned offsets for word/halfword cases so these cases isolate decode/data semantics rather than alignment-policy behavior.
- Existing Stage 119P tests separately cover unaligned Normal-memory access, strict alignment faults, unmapped accesses, and unsupported instruction forms.

## Build integration

The differential executable is registered in `tools/run_tests.sh` and links the guest CPU reset implementation plus the isolated A32 helper and scalar memory dependencies.

## Scope and limitations

This is a software reference-model comparison, not an external ARM hardware oracle and not a comparison against a full legacy instruction interpreter. It covers only the documented AL/immediate/pre-index/no-writeback/non-PC subset. It does not prove architectural correctness for condition execution, PC operands, post-index/writeback, register offsets, unaligned multiple accesses, abort restart behavior, or full system-control semantics.

The helper remains isolated and is not dispatched from `re_guest_cpu_step`.

## Next work

1. Validate Stage 119P's full host suite and Android ARM64 build.
2. Validate this 512-case differential suite in CI and fix any mismatches.
3. Add directed condition/encoding rejection tests and stronger fault/permission cases.
4. Research guest SCTLR reset and write behavior, and establish the guest architecture profile from runtime evidence.
5. Do not enable production decoder integration until the subset's semantics and exception behavior are independently supported.
