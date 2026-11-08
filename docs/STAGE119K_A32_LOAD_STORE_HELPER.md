# Stage 119K — Isolated A32 immediate load/store transfer helper

## Implemented
Added:
- `runtime/guest/re_guest_cpu_transfer.h`
- `runtime/guest/re_guest_cpu_transfer.c`
- `tests/test_re_guest_cpu_transfer.c`

The helper executes a deliberately narrow A32 subset through the CPU data-memory facade:
- LDR (word), immediate offset, pre-index, no writeback, AL condition
- STR (word), same addressing constraints
- LDRB and STRB under the same constraints

The helper rejects PC operands, register-offset addressing, post-indexing, writeback, and non-AL condition codes. Loads commit the destination register only after a successful bus read. Stores are delegated to the bus write operation.

## Safety boundary
This is an isolated helper, not yet wired into `re_guest_cpu_step`. The production interpreter still uses the legacy contiguous-memory decoder. This avoids silently changing execution semantics before differential coverage exists.

The implementation deliberately leaves Thumb transfers, halfword transfers, LDM/STM, PC-relative forms, and unaligned word behavior unsupported. Base-plus/minus-offset arithmetic is 32-bit guest-address arithmetic; region resolution rejects out-of-range accesses.

## Tests
The new host test covers LDR, STR, LDRB, STRB, mapped memory writes, unmapped load failure without destination-register mutation, and fail-closed rejection of unsupported forms.

## Next
- Validate the new host test and Android ARM64 build.
- Add differential fixtures comparing isolated helper results with the legacy decoder for supported encodings.
- Research ARMv7 unaligned word semantics before allowing broader instruction coverage.
- Only then consider connecting the helper to an opt-in interpreter path.
