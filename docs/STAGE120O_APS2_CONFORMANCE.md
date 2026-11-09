# Stage 120O — Independent APS2 Conformance and Malformed-Input Hardening

## Objective

Validate the ELF32 ARM APS2 decoder against Android Bionic's packed relocation iterator semantics and harden malformed input handling. This stage is an offline, read-only parser improvement; it does not implement runtime relocation.

## Reference semantics

Android Bionic defines four group flags:
- GROUPED_BY_INFO (1): one absolute r_info value is read for the group.
- GROUPED_BY_OFFSET_DELTA (2): one offset delta is read for the group.
- GROUPED_BY_ADDEND (4): valid only together with HAS_ADDEND.
- HAS_ADDEND (8): RELA addends are present; with GROUPED_BY_ADDEND one delta is read per group, otherwise one delta per relocation. When absent, RELA addend state resets to zero for the group.

The decoder's tests are grounded in Bionic's packed relocation iterator and for_all_packed_relocs implementation:
https://android.googlesource.com/platform/bionic/+/ac62e8285630592b1bf060711fa13e654c36db20/linker/linker_reloc_iterators.h
https://android.googlesource.com/platform/bionic/+/refs/heads/aml_con_341810060/linker/linker_reloc_iterators.h

## Implementation changes

- SLEB128 now validates the exact legal payload range of the fifth byte for signed 32-bit values.
- A fifth-byte continuation is rejected as overlong.
- Existing 64 MiB packed-stream, 4,000,000 relocation, and 1,000,000 group limits remain enforced.
- Group flag legality is validated separately for REL and RELA.
- No target bytes are read or written; unknown relocation types remain preserved.

## Conformance tests

- All four legal REL flag combinations: 0, 1, 2, 3.
- All twelve legal RELA flag combinations: 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15.
- Illegal REL combinations and RELA GROUPED_BY_ADDEND without HAS_ADDEND.
- Grouped/ungrouped offsets and r_info.
- Grouped and per-relocation addend deltas.
- Addend reset and accumulation across multiple groups.
- Signed 32-bit SLEB128 fifth-byte boundaries, overlong continuation, truncation.
- Zero relocation stream, negative/excessive counts, excessive group sizes, unknown flags, truncated group fields, and every truncated prefix of a valid packed stream.

## Scope boundaries

- ELF32 EM_ARM Android APS2 only.
- No symbol provider inference, runtime load-bias calculation, or relocation application.
- RELR remains a separately audited encoding.
- No Android device execution or game boot is claimed.

## Next

Stage 120P — APS2/RELR coexistence, dynamic-tag cross-checks, and independent toolchain differential validation.
