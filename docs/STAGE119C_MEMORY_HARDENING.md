# Stage 119C — Memory range hardening and CPU compatibility view

## Delivered
- Hardened guest address arithmetic by validating sizes against the remaining 32-bit guest address space before addition.
- Mapping rejects host-buffer sizes larger than SIZE_MAX, invalid permission flags, zero permissions, zero sizes, overlap, and capacity exhaustion.
- Resolution clears the output pointer on failure and rejects requests crossing the 4 GiB boundary without overflow.
- Added a narrow adapter exposing exactly one mapped region through the legacy contiguous REGUESTMEM CPU view.
- Added tests for the final guest byte, 4 GiB wraparound, SIZE_MAX request, invalid permissions, capacity, adapter permissions, and output clearing.
- Added adapter source to host tests and Android CMake.

## Safety boundary
The adapter is not a multi-region memory bus. It borrows one region's buffer and does not dynamically enforce permissions on every CPU read/write. The CPU still consumes contiguous REGUESTMEM. The memory manager is therefore not yet connected to guest execution; XE3U loading remains blocked pending a CPU memory-access abstraction that resolves every access and enforces permissions.

## Validation
The latest GitHub Actions runs must validate the latest main commit. A green host test and Android ARM64 link prove compilation/integration only; they do not prove CPU correctness, physical-device execution, or game boot.

## Remaining Stage 119 exit criteria
1. Audit every CPU memory access and PC fetch.
2. Introduce a checked read/write/fetch bus or callbacks, with width, alignment, permission, and cross-region rules explicitly tested.
3. Integrate memory manager through that bus; remove unsafe contiguous assumptions.
4. Add loader fixtures for code, data, BSS, stack, and trampoline mappings.
5. Document the guest virtual address map from verified XE3U evidence.

## Project state
The app remains a bootstrap/probe APK. No original game loader, full relocation engine, complete CPU coverage, full S3E import surface, renderer, audio, input, physical-device runtime, or playable game is claimed.
