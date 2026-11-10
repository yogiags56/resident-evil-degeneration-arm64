# Stage 122D — Transactional multi-image ELF32 loading

## Objective
Load a caller-selected dependency-first sequence of ELF32 EM_ARM images into one guest address space, with batch-level rollback if any image fails.

## Implemented
- Added `re_guest_elf32_batch_loader.h/.c`.
- Added a bounded batch request API for up to 32 images.
- Preflights every image's ELF headers, PT_LOAD count, load bias, buffer capacity, segment flags, W+X rejection, guest address range, existing-map overlap, inter-image overlap, and aggregate region capacity before mutation.
- Loads images in caller-provided dependency-first order using the existing single-image transactional loader.
- Captures the complete guest mapping table before the first image and restores it if any later image fails, removing all mappings created by the batch while preserving pre-existing mappings.
- Publishes result metadata only after the complete batch succeeds.
- Reports failing module, segment, relocation, and inner single-image status.
- Added host regression for two-image success and second-image relocation failure rollback.
- Registered the batch loader in the host suite and Android native CMake target.

## Important boundaries
- The caller must supply modules in dependency-first order; the dependency graph stage supplies that order.
- This API accepts already-prepared relocation records. It does not yet parse all modules first, resolve cross-module symbols automatically, or build provider symbol tables.
- Relocation values for providers must be prepared by the caller before batch loading.
- Mapping-table rollback does not restore bytes in caller-owned segment buffers. Buffers for a failed batch must be discarded or reinitialized.
- Constructors, DT_INIT/DT_INIT_ARRAY, GNU symbol versioning, TLS, IFUNC, Bionic behavior, and guest entry execution are not implemented here.
- No claim is made that the original game boots.

## Validation
Host regression and Android ARM64 build must pass before the stage is complete.
