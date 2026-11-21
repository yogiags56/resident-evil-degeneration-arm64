# Stage 119N — Guest memory attributes and alignment policy

## Implemented

- Added `REGUESTMEMType`: UNKNOWN, NORMAL, DEVICE, STRONGLY_ORDERED.
- Extended each guest memory region with its memory type.
- Added `re_guest_memory_map_ex` for explicit attributes.
- Preserved `re_guest_memory_map`; it assigns UNKNOWN conservatively.
- Added `re_guest_memory_query_type` for bounded range queries.
- Added `REGUESTALIGNMENTPOLICY`, whose initializer enables strict alignment (SCTLR.A=1).
- Added `re_guest_alignment_check` for instruction-family-aware alignment decisions.
- Registered the policy in host tests and Android CMake.

## Policy behavior

| Access | Naturally aligned | Unaligned with SCTLR.A=1 | Unaligned with SCTLR.A=0 |
|---|---|---|---|
| Byte scalar | Allowed | Allowed | Allowed |
| Halfword scalar | Allowed | Alignment fault | Allowed only in NORMAL |
| Word scalar | Allowed | Alignment fault | Allowed only in NORMAL |
| Exclusive / doubleword / multiple / SWP / coprocessor | Allowed when width-aligned | Alignment fault | Alignment fault |

The policy helper only decides whether an address is architecturally eligible. It does not yet perform an unaligned load/store. The bus remains strict and no production instruction behavior has changed.

## Safety and compatibility

- Existing mapping callers retain their source-level API and now receive UNKNOWN attributes.
- UNKNOWN is not treated as NORMAL for permissive unaligned accesses.
- Device and Strongly-ordered regions remain strict for unaligned accesses.
- Aligned exclusive and doubleword operations are not incorrectly rejected merely because their instruction family has special unaligned restrictions.
- No guest SCTLR register emulation is introduced. The policy initializer therefore defaults to strict alignment; the explicit setter is a test/runtime seam until system-control state is integrated.
- The checker does not replace full MMU/page-fault semantics or model memory-type transitions.

## Tests

`tests/test_re_guest_alignment.c` covers:
- default strict policy;
- aligned and unaligned byte/halfword/word accesses;
- Normal memory permissive case with SCTLR.A clear;
- Device and UNKNOWN conservative rejection;
- exclusive, doubleword, multiple, SWP, and coprocessor unaligned rejection;
- aligned exclusive/doubleword acceptance;
- width mismatch, unmapped range, and null arguments.

The test is included in `tools/run_tests.sh`; implementation is included in Android CMake.

## Not yet done

- The memory bus still rejects unaligned 16/32-bit accesses. This stage does not bypass it.
- No halfword or unaligned scalar load/store instruction helper is enabled.
- No guest SCTLR system-register implementation or target reset-value evidence exists.
- No production interpreter migration has occurred.

## Next: Stage 119O

Implement checked scalar transfer operations that consult this policy, with safe byte-assembled Normal-memory reads/writes and tests for endianness, boundary, permission, and failure behavior. Keep strict behavior for UNKNOWN/Device/Strongly-ordered memory and for all instruction families that require alignment.
