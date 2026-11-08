# Stage 119H — Checked CPU step gate

## Implemented
Added `re_guest_cpu_step_checked` as an opt-in execution entry point. The legacy `re_guest_cpu_step` API and the public `REGUESTMEM` / `REGUESTCPU` layouts remain unchanged.

Before invoking the existing decoder, the checked entry point:
1. Fetches the current ARM or Thumb instruction through the memory bus using EXEC permission.
2. For Thumb-2 wide instructions, independently checks the second halfword through the bus.
3. Confirms the fetched instruction bytes match the legacy contiguous memory view consumed by the existing decoder.
4. Returns a distinct CPU permission or fetch-mismatch status without advancing CPU state if the preflight fails.
5. Calls the existing decoder only after the preflight succeeds.

This provides an incremental permission gate without yet rewriting all instruction/data memory operations.

## Tests
Added `tests/test_re_guest_cpu_checked_step.c` for:
- Successful Thumb MOVS and ADDS execution through checked entry.
- EXEC permission denial without PC/register/step mutation.
- Mismatch between bus image and legacy decoder image.
- Invalid CPU argument.

The test is registered in `tools/run_tests.sh`. Existing Android CMake already compiles the CPU, memory bus and adapter.

## Limitations
- This is a transitional gate, not a complete multi-region CPU memory implementation.
- The existing decoder still reads instruction bytes and all data through legacy contiguous `REGUESTMEM`.
- The bus check and legacy decoder read are separate operations; the loader/runtime must keep both views immutable and coherent during a step. A future direct bus-backed decoder will remove this dual-view requirement.
- Runtime-level diagnostic mapping for new CPU statuses has not yet been added.
- No original game boot or physical-device execution is claimed.

## Next
Stage 119I should add checked-step tests for ARM and Thumb-2, verify IT-block behavior, then introduce a bus-backed decoder memory interface so the CPU no longer depends on duplicated contiguous memory. Only after parity and regression tests pass should the runtime select the checked path.
