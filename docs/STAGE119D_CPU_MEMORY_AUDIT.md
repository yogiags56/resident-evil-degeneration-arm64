# Stage 119D — CPU memory-access audit

## Scope inspected
- `runtime/guest/re_guest_cpu.h`
- `runtime/guest/re_guest_cpu.c`
- `runtime/guest/re_guest_memory.h/.c`
- `runtime/guest/re_guest_memory_view.h/.c`
- `tests/test_re_guest_cpu.c`
- Android native CMake source list

## Findings

### Current CPU memory model
The interpreter accepts one legacy `REGUESTMEM` view containing a host byte pointer, byte length, and guest base. Its private helpers (`range`, `rd8`, `wr8`, `read16`, `write16`, `read32`, `write32`) resolve addresses directly against that contiguous view.

Instruction fetch is performed by `read32` in ARM state and `read16` in Thumb state. Thumb-2 instructions fetch the second halfword separately. Data accesses from ARM single transfers and block transfers, Thumb-1 register/immediate transfers, and Thumb-2 wide transfers all use the same direct helpers.

### Missing protections
- The CPU does not receive `REGUESTMEMORY` and does not call `re_guest_memory_resolve`.
- Instruction fetch does not request `RE_GUEST_MEM_EXEC`.
- Loads do not request `RE_GUEST_MEM_READ`; stores do not request `RE_GUEST_MEM_WRITE`.
- The compatibility view validates permission only once when a view is created. It cannot enforce permissions on subsequent individual accesses.
- One contiguous view cannot represent disjoint code, data, stack, BSS, archive, and trampoline regions.
- A Thumb-2 instruction whose first halfword is at the end of a view may fail on the second-halfword fetch. The failure is currently a generic memory fault, which is acceptable as a stop but needs an explicit cross-boundary test.
- ARM LDM/STM and equivalent multi-access operations may have earlier register or memory effects before a later element faults. Precise partial-completion behavior must be specified and tested rather than assumed atomic.
- The legacy `range` helper is bounded by the supplied view, but the legacy view itself is not a permission-aware guest address-space abstraction.

## Required design before integration
1. Introduce a CPU memory interface with context plus checked callbacks for instruction fetch, data reads, and data writes.
2. Keep guest addresses as `uint32_t`; perform range arithmetic in a wider unsigned type and reject 4 GiB wraparound.
3. Define little-endian 8/16/32-bit accessors, alignment behavior, and whether any unaligned forms are architecturally supported.
4. Fetch each Thumb-2 halfword independently with EXEC permission; no fetch may cross an unmapped or non-executable region.
5. Resolve every data access against the region manager with the exact READ or WRITE permission.
6. Define block-transfer partial-fault semantics and add tests for a fault on the first, middle, and final element.
7. Preserve the legacy API temporarily through an adapter, but make the checked bus the only path used by the new runtime.
8. Add synthetic multi-region tests: executable code, read-only data, writable data, unmapped gaps, stack, and a region ending at the 4 GiB boundary.

## Deliberate non-changes
No CPU execution code was changed in this audit stage. The current contiguous-memory interpreter remains the baseline so existing instruction behavior is not silently altered. The Stage 119C single-region view remains transitional and must not be mistaken for multi-region integration.

## Validation and limits
This stage is a source audit and design checkpoint. It is not evidence of full CPU correctness, full ARMv7/Thumb-2 coverage, XE3U loading, physical-device execution, or game boot. The latest Stage 119C Source Checks run was green; the Android ARM64 build was green for the adapter integration commit. Stage 119D itself adds documentation only, so a new Android build is not required for its source change.

## Exit criteria
Stage 119 is not complete until the checked bus is implemented, CPU accesses are migrated and tested, loader fixtures map code/data/BSS/stack safely, and the guest address map is supported by verified XE3U evidence.
