# Stage 123U — Shared guest heap and libc allocation provider

## Evidence and scope

The Stage 123O original ELF32 manifest identifies relocation-backed
`malloc`, `free`, `realloc`, and `valloc` references in the direct
`libc.so` dependency. Stage 123S previously implemented C++ `new`/`delete`
using a separate arena. This stage consolidates allocation ownership so
libc and C++ allocation families share one guest heap and can free each
other's returned pointers.

## Implementation

Added:

- `runtime/guest/re_guest_heap.h`
- `runtime/guest/re_guest_heap.c`
- `runtime/guest/re_guest_libc_alloc_provider.h`
- `runtime/guest/re_guest_libc_alloc_provider.c`
- `tests/test_re_guest_libc_alloc_provider.c`

Refactored the Stage 123S C++ provider to use the shared heap implementation.

The heap uses a caller-reserved contiguous guest RW arena and bounded
host-side allocation records. It returns only 32-bit guest addresses; no
host allocator pointer crosses the guest ABI. The arena supports 512 active
records, 8-byte minimum allocation alignment, and page-aligned `valloc`
requests (4096-byte alignment). Allocation is first-fit with overlap checks
that account for existing blocks which begin before an aligned candidate.

The libc provider binds exactly the four relocation-backed symbol indices
and exports them under the evidenced direct dependency `libc.so`:

- `malloc(size)`
- `free(pointer)`
- `realloc(pointer, size)`
- `valloc(size)`

`realloc` preserves the original allocation if growth fails. When it moves
an allocation, it copies the old requested byte range through validated
guest memory before releasing the old record. Null and unknown frees are
harmless; only exact pointers returned by this heap release records.

The C++ provider's scalar/array `new` and `delete` now use the same heap
records, so libc `free` can release a C++ allocation and C++ `delete` can
release a libc allocation. This removes the separate-allocator ownership
hazard documented in Stage 123S.

## Deliberate limitations

- The runtime/loader must reserve and map the heap arena before either
  provider is used. This stage does not create or grow guest mappings.
- Calls are externally serialized. The reentrancy guard is not a
  cross-thread mutex.
- `malloc(0)` returns a minimal one-byte allocation; `realloc(p, 0)`
  frees and returns null. These are explicit provider policies.
- `calloc`, aligned allocation APIs, `posix_memalign`, and the broader
  libc/POSIX family are not implemented here.
- The allocator is fixed-capacity and does not coalesce bytes; freeing
  releases exact records and first-fit reuse recovers their ranges.
- No original-game allocation sequence or physical-device heap lifecycle
  has been validated. The provider is not yet installed in the production
  ELF32 module-load path.

## Validation

The shared-allocation regression binds both C++ and libc providers to the
same guest heap. It checks cross-family free/reuse, realloc data preservation,
null and unknown frees, out-of-space behavior, and page alignment for
`valloc`. A targeted regression also covers a live allocation crossing the
first page-aligned candidate so `valloc` cannot overlap an earlier block.

## Next

1. Complete Android ARM64 compilation and ELF verification with both
   providers linked.
2. Reserve the heap arena in the actual guest runtime/loader and define its
   lifetime relative to all DSO constructors and destructors.
3. Continue with bounded guest memory/string functions from the remaining
   relocation-backed libc imports.
