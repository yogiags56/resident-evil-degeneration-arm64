# Stage 122B — Mapped runtime integration

## Goal
Connect the runtime coordinator and native import trampoline dispatcher to the existing permission-checked multi-region guest memory bus while preserving the contiguous-memory APIs for existing callers.

## Implemented
- Added `re_guest_runtime_init_mapped` and a mapped-memory field to the runtime coordinator.
- Runtime stepping now selects mapped CPU execution and mapped import dispatch when initialized with a guest memory bus.
- Added mapped import bindings with a handler signature that receives the guest memory bus.
- Added mapped import pointer-slot patching through WRITE-checked guest memory.
- Added mapped AAPCS32 dispatch. Core arguments are copied from r0-r3; stack argument bytes are read through the READ-checked bus into bounded call-local storage.
- Mapped import handlers can use the bus for additional guest-memory access rather than receiving a host pointer into a contiguous guest image.
- Import dispatch preserves CPU state if a mapped handler reports failure.
- Existing contiguous runtime initialization, legacy import handlers, and legacy patch-slot APIs remain available.
- Added a mapped runtime integration regression covering RX code, RW stack/data, import trampoline dispatch, pointer-slot patching, AAPCS32 stack arguments, and unmapped instruction faults.

## Boundaries
- This is the runtime/CPU/import-dispatch integration layer; it does not yet load DT_NEEDED dependencies automatically or create a complete Bionic environment.
- Mapped handlers are explicit native callbacks. Existing legacy handlers are not silently invoked with fabricated contiguous memory.
- The AAPCS32 stack snapshot is bounded to `RE_IMPORT_MAPPED_STACK_BYTES` (256 bytes); handlers needing other guest memory must use the provided bus.
- ELF loading and runtime setup still require caller-owned segment buffers and caller-supplied symbol/provider policy.
- No claim is made that the original game boots.

## Validation
The host regression suite and Android ARM64 native build must pass before this stage is considered complete.
