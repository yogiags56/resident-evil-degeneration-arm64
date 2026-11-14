# Stage 125E — Shared-heap libstdc++.so catalog integration

## Goal

Bring the four relocation-backed C++ allocation imports into the same exact SONAME provider catalog as the implemented libc, liblog, libz, and libdl providers. The critical correctness condition is allocator identity: C++ new/delete must operate on the same guest allocation records as libc malloc/free/realloc.

## Implementation

The C++ heap provider now supports two explicit ownership modes:

- Owned mode: re_guest_cxx_heap_provider_init creates and owns its embedded guest heap, preserving the standalone provider API.
- Shared mode: re_guest_cxx_heap_provider_init_shared attaches to a caller-owned REGUESTHEAP, without copying or taking ownership of it.

The C++ import handlers now dereference the selected heap pointer. Shutdown clears only an owned heap; shared mode detaches without destroying the libc-owned heap.

The combined provider catalog initializer now accepts the C++ provider and publishes exact libstdc++.so exports for:

- _Znwj — scalar new
- _Znaj — array new
- _ZdlPv — scalar delete
- _ZdaPv — array delete

It binds the four symbols into the same import table, appends a fifth virtual provider to the combined catalog, and rolls back all import-table changes if the C++ bind or catalog validation fails. The caller must initialize the C++ provider in shared mode using the exact heap already referenced by the libc allocation provider when cross-family allocation/free is required.

## Regression coverage

The combined catalog regression now supplies the four C++ symbols, attaches the C++ provider to the libc allocator's heap, and verifies:

- the combined catalog contains six entries including a sample module;
- libstdc++.so has four nonzero guest trampoline exports;
- all four export names and version-library identities match the original relocation-backed manifest;
- the C++ provider's heap pointer is exactly the libc allocator's heap;
- the combined import table includes all 14 provider additions beyond the implemented libc set (2 log + 4 zlib + 4 dl + 4 C++).

The pre-existing C++ heap regression continues to exercise scalar/array allocation, alignment, non-overlap, deletion, reuse, and allocation failure.

## Scope and remaining gaps

This closes the catalog wiring for all four C++ allocation operators found in the original relocation-backed manifest. It does not implement C++ exceptions/unwinding, std::bad_alloc, sized/aligned/nothrow new/delete, or the complete libstdc++ runtime. It also does not complete libc/POSIX imports, the declared libm.so dependency policy, GLES/EGL, or the production XE3U-to-ELF loader boot sequence. No original-game boot or device execution is claimed.

## Validation

Host and Android ARM64 CI are pending for the final source commit.
