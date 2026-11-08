# Stage 119E — Checked multi-region guest memory bus

## Delivered
- Added `runtime/guest/re_guest_memory_bus.h/.c`.
- Separate 8/16/32-bit APIs for instruction fetch (EXEC), data read (READ), and data write (WRITE).
- Little-endian loads/stores with explicit alignment checks for 16/32-bit operations.
- Each access resolves its entire width through `re_guest_memory_resolve`; cross-region and unmapped accesses fail closed.
- Read output values are cleared before resolution, preventing stale outputs on failed reads.
- Added synthetic bus tests for executable code, read-only data, writable data, unmapped gaps, alignment, boundary crossing, and permission failures.
- Added the bus source to Android CMake and the bus test to the host test runner.

## Complete source files introduced in this stage
- `runtime/guest/re_guest_memory_bus.h`
- `runtime/guest/re_guest_memory_bus.c`
- `tests/test_re_guest_memory_bus.c`

## Important boundary
The CPU interpreter has NOT been migrated. `runtime/guest/re_guest_cpu.c` still uses its legacy contiguous `REGUESTMEM` and direct read/write helpers. This bus is currently a separately tested API, not yet used for instruction execution. The region manager and bus borrow their memory manager/buffers; owners must preserve their lifetime.

## Validation
The GitHub Actions workflows are triggered by the commits that add the bus, test registration, and Android CMake source. Record final Source Checks and Android ARM64 results only after inspecting runs for the latest main commit. A passing compile/link does not establish CPU bus migration, device execution, or game boot.

## Next
Stage 119F will add CPU memory-interface callbacks or an equivalent bus abstraction and migrate instruction fetch plus every data access. Tests must cover each access class, Thumb-2 second-halfword fetch, block-transfer partial-fault semantics, and memory permissions. Keep the legacy CPU path until differential regressions pass.
