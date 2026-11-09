# Stage 120U — ELF Segment Materialization and BSS Initialization

## Objective
Copy validated ELF file-backed segment bytes into already mapped guest memory
and zero the memory-only tail (BSS), while ensuring that all segment requests
are preflighted before the first destination byte changes.

## Contract
- Caller supplies source ELF bytes and a bounded list of segment descriptors.
- Each destination guest range must already be mapped and writable.
- file_size must not exceed memory_size.
- Source offsets and sizes must fit within the supplied ELF byte buffer.
- Guest ranges must fit the 32-bit guest address space and must not overlap.
- Every segment is resolved before commit; a preflight failure leaves all
  destination buffers unchanged.
- File-backed bytes are copied, then BSS tails are zero-filled.

## Explicit non-goals
No memory allocation, guest mapping, host page-protection changes, relocation,
load-bias selection, permission finalization, ELF parsing, or guest execution.
This helper must receive descriptors from a separately validated ELF parser.
