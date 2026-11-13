# Stage 123S — Guest C++ new/delete heap provider

## Evidence and scope

The original ELF32 relocation manifest records four C++ allocation-family
imports: `_Znwj`, `_Znaj`, `_ZdlPv`, and `_ZdaPv`. This stage provides
guest-callable adapters for scalar/array allocation and deallocation. It does
not bind the broader C++ runtime or exception ABI.

## Implementation

Added:

- `runtime/guest/re_guest_cxx_heap_provider.h`
- `runtime/guest/re_guest_cxx_heap_provider.c`
- `tests/test_re_guest_cxx_heap_provider.c`

The provider uses a dedicated, caller-reserved contiguous guest RW arena.
Its base and size must be 8-byte aligned and remain mapped for the provider's
lifetime. Host-side metadata tracks up to 512 active allocations. Allocation
uses an 8-byte-aligned first-fit scan; freeing an exact returned guest address
releases its record so the range can be reused. No host pointer is returned
to the guest and no host allocator is used for guest objects.

The provider binds exactly the four manifest-backed undefined ELF32 symbol
indices as guest Thumb trampolines and exports them under `libstdc++.so`.
Scalar and array new share the same allocator; scalar and array delete share
the same exact-address release path.

## Deliberate limitations

- At the original Stage 123S checkpoint, C `malloc/free/realloc` were not
  connected. Stage 123U supersedes the separate allocator internals with a
  shared guest heap and adds libc `malloc/free/realloc/valloc` adapters.
- On allocation failure, new returns a null guest pointer. It does not throw
  `std::bad_alloc`; the C++ exception/unwind ABI is not implemented here.
- Sized delete, nothrow new, aligned new/delete, placement new, and other
  C++ runtime symbols are not provided unless separately evidenced and added.
- Unknown and null delete pointers are ignored. Only exact addresses returned
  by this provider release allocation records.
- The arena is fixed and must be reserved by the guest loader/runtime. The
  provider does not grow mappings or invoke host allocation.
- Calls must be externally serialized. The small reentrancy guard is not a
  cross-thread mutex.
- The original game's allocator ownership and exact call sequence still need
  runtime validation before installing this provider in the production
  module-load lifecycle.

## Validation

The host regression binds all four exact symbol indices and exercises scalar
new, array new, 8-byte alignment, non-overlap, writes into returned guest
memory, delete/reuse, null/unknown delete, and out-of-space rejection.

Host CI passed on 2026-11-13 in run `43413952682`: 312 Python tests, native
C mock-header syntax checks, and all compiled host regressions including
`Guest C++ heap provider tests: PASS`. Android ARM64 build and ELF verification
also passed in run `43413952565`. This does not prove the original game's C++
runtime or heap lifecycle is compatible.

## Next

1. Complete Android ARM64 compilation and ELF verification for this provider.
2. Establish the guest heap arena's ownership and lifetime in the ELF32 loader.
3. Reserve the shared heap arena in the guest runtime and validate its
   lifetime across DSO constructors/destructors. Stage 123U now makes C++
   new/delete and libc malloc/free use the same allocation records.
4. Add exact C++ ABI exception and allocation-failure handling only if the
   original binary demonstrates that those paths are required.


## Stage 123U follow-up

The C++ provider now delegates to the shared `REGUESTHEAP` implementation.
The libc provider uses the same heap records, so `free` and `delete` can
release allocations from either family. See
[Stage 123U](STAGE123U_SHARED_GUEST_HEAP_LIBC_ALLOC.md).
