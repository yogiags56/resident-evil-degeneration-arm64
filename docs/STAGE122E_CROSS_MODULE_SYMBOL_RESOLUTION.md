# Stage 122E — Cross-module ELF32 symbol provider catalog

## Objective
Build an ordered, caller-owned provider catalog from decoded ELF32 dynamic symbol tables so a consumer module's relocation planner can resolve definitions exported by another module.

## Implemented
- Added `re_guest_elf32_provider_catalog.h/.c`.
- Accepts up to 32 already-decoded module symbol tables.
- Preserves caller module order and symbol order as the global provider lookup order.
- Exports defined GLOBAL and WEAK symbols with DEFAULT or PROTECTED visibility.
- Excludes undefined, LOCAL, HIDDEN, INTERNAL, TLS, and unsupported binding entries.
- Applies each provider module's load bias to ordinary defined symbols.
- Preserves SHN_ABS values without rebasing.
- Carries provider SONAME into the provider record.
- Rejects invalid SONAMEs and 32-bit address overflow.
- Preflights capacity and all records before publishing output; provider array remains unchanged on error.
- Added tests for export filtering, load bias, SHN_ABS, provider ordering, capacity rollback, and overflow.
- Added a cross-module regression: consumer relocation for `s3eFileOpen` resolves to a provider in a different module using the provider's rebased address.
- Registered both tests in host suite and provider catalog in Android native CMake target.

## Integration contract
The dependency graph supplies module order. Each ELF image's dynamic symbol table must be decoded first. The provider catalog is then built from all module symbol tables, and each consumer's relocation planner receives that catalog. The existing transactional batch loader can then map the modules using their already-prepared relocation records.

## Boundaries
- This is a provider catalog primitive, not yet an automatic all-module ELF linker coordinator.
- GNU symbol version tables are not decoded; versioned imports remain explicitly unsupported by the relocation planner.
- Symbol interposition policy is currently strict: multiple matching strong definitions are reported ambiguous by the existing resolver.
- TLS, IFUNC, COPY, GNU-hash-only symbol bounds, PLT lazy binding, Bionic symbol namespaces, constructors, and game entry execution remain out of scope.
- No claim is made that the game boots.

## Validation
Host CI must pass for the provider catalog and cross-module relocation regressions. Android ARM64 compilation remains a separate validation requirement.
