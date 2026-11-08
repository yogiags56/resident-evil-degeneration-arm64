# Stage 119F — ABI-preserving CPU bus adapter

## Implementation
Added a separate CPU-facing adapter over the Stage 119E checked memory bus:
- `runtime/guest/re_guest_cpu_bus_adapter.h`
- `runtime/guest/re_guest_cpu_bus_adapter.c`
- `tests/test_re_guest_cpu_bus_adapter.c`

The adapter exposes separate instruction fetch (EXEC), data read (READ), and data write (WRITE) entry points for 8/16/32-bit data and 16/32-bit instruction fetch. It does not modify `REGUESTMEM`, `REGUESTCPU`, or the interpreter implementation. The existing interpreter still uses its legacy contiguous memory helpers.

The adapter is included in Android CMake and the host test runner. The test covers executable code, read-only data, writable data, permission denial, unmapped memory, and alignment errors.

## Validation
The new test is registered but the latest Source Checks and Android ARM64 workflows are pending at the time this report is written. Do not treat this stage as green until both latest runs complete successfully.

## Explicit limitations
- This is an adapter/API milestone, not CPU migration.
- `re_guest_cpu_step` does not yet call these adapter functions.
- Thumb-2 second-halfword fetch, ARM/Thumb data operations, LDM/STM partial-fault behavior, and precise CPU fault reporting remain unmodified.
- No XE3U loading, original game boot, or physical-device execution is claimed.

## Next
Stage 119G will integrate the adapter into a test-only CPU memory interface and prove fetch permission enforcement before any production interpreter path is switched. Then migrate data access families separately.
