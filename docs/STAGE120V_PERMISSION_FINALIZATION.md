# Stage 120V — Guest Memory Protection and Permission Finalization

## Objective
Finalize software guest permissions after segment materialization, using an
all-or-nothing operation over exact pre-existing guest memory mappings.

## Contract
- Every request identifies one exact existing mapping by guest base and size.
- All requested permissions are validated before any region is changed.
- READ, WRITE, and EXEC are the only accepted permission bits.
- Empty permissions and W+X are rejected.
- Missing, duplicate, malformed, and out-of-range requests fail atomically.
- Successful finalization updates only the guest memory manager's permission
  metadata; subsequent guest resolve operations enforce the new permissions.

## Host protection boundary
This stage does not call mprotect or VirtualProtect, alter host page-table
permissions, allocate memory, flush instruction caches, apply relocations,
or execute guest code. Host-page protection and instruction-cache coherence
require a separate platform-specific design and validation stage.

## Validation
Native C regression coverage is integrated into tools/run_tests.sh.
GitHub Actions validation remains pending until the workflow reports results.
