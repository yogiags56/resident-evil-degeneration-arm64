# Stage 120T — Guest Memory Mapping Transaction

## Objective
Provide an atomic mapping operation for a set of guest memory regions. If any
requested mapping fails, mappings created by the current operation are removed
in reverse order. Mappings that existed before the operation are preserved.

## Contract
- Requests use 32-bit guest virtual addresses and borrowed host buffers.
- Each mapping delegates range, permission, overlap, and memory-type checks to
  the existing guest memory manager.
- Failure reports the failing request index and underlying map status.
- Rollback removes only mappings successfully created by this transaction.
- Zero requests is a successful no-op.
- The caller owns all host buffers and must keep them alive while mapped.

## Explicit non-goals
This is not an ELF parser or ELF loader. It does not allocate host memory,
copy segment bytes, clear BSS, choose ET_DYN load bias, apply relocations,
change host page permissions, or execute guest code.
