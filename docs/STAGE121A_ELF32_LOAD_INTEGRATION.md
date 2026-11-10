# Stage 121A — Integrated ELF32 ARM image loading

## Goal
Connect existing ELF32 program-header parsing, PT_DYNAMIC inventory, guest
mapping transactions, segment materialization, relocation application, and
permission finalization into one tested load operation. This stage reuses the
existing components; it does not duplicate their parsers or algorithms.

## Implementation
- Parses ELF32 EM_ARM and rejects overlapping PT_LOAD ranges.
- Accepts caller-owned buffers in PT_LOAD program-header order; validates
  capacity and caller-selected load bias before mapping.
- Requires ET_EXEC load bias to be zero and ET_DYN bias to preserve each
  segment's p_align congruence.
- Temporarily maps every segment writable, then copies file bytes and clears
  BSS through the existing all-segment preflight materializer.
- Applies caller-prepared relocations in bounded batches using the existing
  relocation engine. RELATIVE entries receive the selected load bias.
- Finalizes exact segment permissions from ELF PF_R/PF_W/PF_X flags and
  rejects W+X. The entry point and ARM/Thumb entry state are reported.
- On a post-map error, removes every mapping created by the load operation.

## Caller contract and limits
This is a single-image loader coordinator, not a complete Android/Bionic
dynamic linker. The caller must select/provide the ELF image and host buffers,
load dependencies in a verified order, decode relocation records, resolve
external symbols, and pass relocation targets/symbol values as final guest
addresses. The loader does not search the filesystem or DTRZ archive, infer
symbol scope, parse GNU version indices, allocate memory, execute init arrays,
or start guest code. Buffers are borrowed; after a failed load their bytes
must be discarded or reinitialized even though guest mappings are rolled back.

## Validation
Synthetic ELF32 ARM tests cover ET_DYN load bias, two PT_LOAD regions,
file-backed copy, BSS zero-fill, RELATIVE relocation, final RX/RW permissions,
W+X rejection, insufficient buffer rejection, and mapping rollback after a
relocation error. CI results are recorded in STAGE121A_VALIDATION_LOG.txt.
