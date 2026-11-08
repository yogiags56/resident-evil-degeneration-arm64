# Stage 119J — CPU data-memory bus facade

## Implemented
Added a CPU-facing data-memory facade in:
- `runtime/guest/re_guest_cpu_memory.h`
- `runtime/guest/re_guest_cpu_memory.c`

The facade wraps the existing guest memory bus and exposes byte, halfword, and word reads/writes. Reads require READ permission; writes require WRITE permission. Memory bus statuses are translated into CPU statuses:
- OK -> RE_CPU_OK
- permission -> RE_CPU_E_PERMISSION
- range/alignment -> RE_CPU_E_ALIGNMENT
- invalid arguments -> RE_CPU_E_ARGUMENT
- unmapped addresses and other memory failures -> RE_CPU_E_MEMORY

Added `tests/test_re_guest_cpu_memory.c`, registered in `tools/run_tests.sh`, and included the implementation in the Android native CMake target.

## Important limitation
This stage adds and tests the facade only. The ARM/Thumb interpreter still performs its existing data accesses through legacy contiguous `REGUESTMEM`. No load/store opcode has been migrated yet, and production runtime behavior is unchanged.

The facade currently inherits the bus's strict alignment policy for 16-bit and 32-bit accesses. ARMv7 unaligned access behavior and instruction-specific alignment rules need architecture research before any decoder migration.

## Next
Stage 119K will add CPU-side load/store integration helpers and migrate one carefully bounded instruction family behind an opt-in checked execution API, preserving legacy behavior for differential tests. Multi-register transfers must remain out of scope until partial-fault/writeback semantics are documented and tested.
