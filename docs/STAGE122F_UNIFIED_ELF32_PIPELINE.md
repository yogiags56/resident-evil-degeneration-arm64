# Stage 122F — Unified ELF32 multi-module load pipeline

## Objective
Connect the bounded module graph, dynamic metadata parser, ELF link metadata decoder, provider catalog, relocation planner, and transactional batch loader into one caller-driven operation.

## Implemented
- Added `re_guest_elf32_pipeline.h/.c`.
- Accepts a bounded inventory of up to 32 candidate modules and caller-owned per-module scratch arrays and segment buffers.
- Parses each candidate's ELF image, PT_DYNAMIC, DT_NEEDED, DT_SONAME, dynamic symbols, and relocation metadata before guest-memory mutation.
- Resolves the requested root by its inventory module name or DT_SONAME; non-root modules must expose DT_SONAME.
- Builds the root-reachable dependency graph and rejects missing dependencies or cycles.
- Builds a provider catalog from reachable modules, applying each module's load bias to defined exports.
- Prepares all modules' relocations against the complete provider catalog before mapping any module.
- Assembles the batch in dependency-first graph order and invokes the existing transactional batch loader once.
- Keeps batch request records on the caller's stack for the duration of the call; no static mutable request state is used.
- Publishes pipeline result metadata only after the complete batch succeeds.
- Returns stage, module, item index, and inner status for failures.
- Added an integration regression with a root ELF that DT_NEEDED-imports a symbol from a second ELF provider. The test verifies dependency-first order, provider rebasing, cross-module relocation result, and four mapped segments.
- Registered the pipeline source in Android native CMake and its integration test in the host suite.

## Ownership and constraints
- Caller supplies the complete candidate module inventory, load biases, segment buffers, symbol/relocation scratch arrays, and provider scratch array.
- The pipeline does not search the filesystem or APK itself; a separate asset/module discovery layer must supply candidate bytes and SONAME inventory.
- Provider lookup order currently follows caller inventory order; module load order follows dependency-first graph order. The runtime's intended global symbol-scope/interposition policy must be validated against the target game and S3E/Bionic environment.
- GNU symbol versions, GNU-hash-only symbol bounds, TLS, IFUNC, COPY relocations, lazy PLT binding, constructors, TLS setup, and Bionic namespace semantics remain unsupported.
- Batch rollback restores guest mapping metadata, not caller-owned bytes; discard/reinitialize buffers after failed batch loads.
- This stage does not execute the game entry point and does not claim playable status.

## Validation
The integration regression and full host suite must pass. Android ARM64 compilation and real asset-driven boot remain separate gates.
