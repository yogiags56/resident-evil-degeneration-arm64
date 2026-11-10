# Stage 119R — Directed A32 scalar decoder and fault tests

## Scope

Add directed regression tests around the isolated Stage 119P A32 scalar transfer helper. The helper remains opt-in and is not connected to production CPU dispatch.

## New test

`tests/test_re_guest_cpu_arm_scalar_faults.c`

Coverage:
- Read-only Normal memory: LDR succeeds; STR returns permission failure and leaves backing bytes unchanged.
- Word load/store crossing two adjacent guest regions: the checked scalar facade rejects the operation rather than partially mutating or reading across regions.
- 32-bit guest address addition wrap and subtraction underflow: both fail closed as unmapped memory accesses.
- Unsupported conditional, post-index, writeback, PC base/destination, register-offset, LDRD, STRSB, and STRSH encodings are rejected before CPU or memory mutation.

## Safety and semantics

The tests verify emulator-side transactional safety for the supported scalar helper. They do not claim exact ARM abort micro-architecture, page-fault restart behavior, or single-copy atomicity. Cross-region accesses remain rejected by the current region-resolve model.

## Validation

The test is registered in `tools/run_tests.sh`. Host CI passed on code commit `46a8d8a` (run 42584904344): 75 Python checks, native C syntax checks, Stage 119P integration tests, 512 differential cases, and the directed fault/decoder test all passed. The latest documentation-only revision is undergoing CI. Stage 119Q host CI passed on commit `c4325e6`, including 512 independent reference-model cases and the Stage 119P integration suite.

## Next

Stage 119S should add further directed condition/encoding coverage and investigate system-control state, especially evidence for guest architecture profile and SCTLR reset behavior. Do not route the helper into production `re_guest_cpu_step` until the system-control and exception semantics are justified.
