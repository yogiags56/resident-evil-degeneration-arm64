# Stage 119K — Isolated A32 immediate load/store helper

## Implemented
Added:
- `runtime/guest/re_guest_cpu_transfer.h`
- `runtime/guest/re_guest_cpu_transfer.c`
- `tests/test_re_guest_cpu_transfer.c`

The helper executes a deliberately narrow A32 subset through the CPU data-memory facade:
- LDR (word), immediate offset, pre-index, no writeback, AL condition
- STR (word), same addressing constraints
- LDRB and STRB under the same constraints

The helper rejects PC operands, register-offset addressing, post-indexing, writeback, and non-AL condition codes. Loads commit the destination register only after a successful memory read. Stores are delegated to the checked bus write operation.

## Differential validation
The regression test now compares seven supported instruction cases against the legacy interpreter. The legacy fixture maps code at guest address 0x1000 and places its contiguous data backing at guest address 0x8000, while the helper uses the checked region bus at 0x8000. It compares destination-register results and resulting data bytes.

The fixture was corrected after CI exposed two test-harness issues: the first version did not include the legacy instruction fetch region in the contiguous memory fixture, and an attempted negative word offset was unaligned. The corrected negative-offset case uses a four-byte aligned displacement. These are fixture corrections; no claim is made that the new differential suite has passed until CI reports success.

## Safety boundary
This is an isolated helper, not yet wired into `re_guest_cpu_step`. The production interpreter still uses the legacy contiguous-memory decoder. This avoids silently changing execution semantics before differential coverage exists.

The implementation deliberately leaves Thumb transfers, halfword transfers, LDM/STM, PC-relative forms, and unaligned word behavior unsupported. Base-plus/minus-offset arithmetic is 32-bit guest-address arithmetic; region resolution rejects out-of-range accesses.

## Next
- Confirm the differential regression test and Android ARM64 build are green.
- Expand tests to compare condition flags, fault behavior, and write-protection cases.
- Research ARMv7 unaligned word semantics before allowing broader instruction coverage.
- Only then consider connecting the helper to an opt-in interpreter path.
