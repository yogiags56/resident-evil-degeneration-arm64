# Stage 120O Report — APS2 Conformance and Malformed-Input Hardening

Status: implementation committed; CI validation pending.

## Delivered

- Enforced exact fifth-byte SLEB128 signed 32-bit payload bounds.
- Added full legal REL and RELA group flag matrix tests.
- Added invalid flag matrix tests.
- Added multi-group addend state and offset accumulation tests.
- Added zero count, count bound, group size bound, negative flags, unknown flags, truncation and malformed prefix tests.
- Preserved the bounded, read-only decoder contract.

## Test design

The expected group state transitions are taken from Android Bionic's packed relocation iterator, not inferred from prior test fixtures:
https://android.googlesource.com/platform/bionic/+/ac62e8285630592b1bf060711fa13e654c36db20/linker/linker_reloc_iterators.h

## Safety

No relocation is applied, no target bytes are dereferenced or changed, and no provider is inferred.
