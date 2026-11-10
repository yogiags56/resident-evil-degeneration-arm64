# Stage 122G — ELF32 constructor metadata and sequence planning

## Implemented
- Extended PT_DYNAMIC decoding with DT_INIT, DT_FINI, DT_INIT_ARRAY, DT_INIT_ARRAYSZ, DT_FINI_ARRAY, and DT_FINI_ARRAYSZ.
- Rejects duplicate constructor/destructor tags, mismatched array address/size pairs, non-4-byte array sizes, and array/function metadata outside file-backed PT_LOAD ranges.
- Added `re_guest_elf32_init.h/.c` to build a constructor call sequence from a successfully loaded Stage 122F pipeline.
- Preserves dependency-first module order.
- Orders each module's DT_INIT before its DT_INIT_ARRAY entries.
- Reads init-array function pointers from guest memory after relocation, not from the unrelocated ELF file.
- Applies each module's load bias to DT_INIT and DT_INIT_ARRAY addresses.
- Validates every constructor target against executable guest memory and preserves AArch32 Thumb state in bit 0.
- Skips null and all-ones array sentinels.
- Uses caller-owned output with capacity preflight and leaves the call array unchanged on failure.
- Added parser tests for init/fini metadata, malformed array sizes, and duplicate tags.
- Added constructor sequence tests for dependency order, DT_INIT/INIT_ARRAY ordering, Thumb pointers, executable permissions, and transactional output behavior.
- Registered source in Android native CMake and tests in host suite.

## Explicit boundary
This stage builds and validates the constructor call list. It does NOT execute constructors yet. Constructor execution requires a bounded AArch32 runtime call mechanism with a safe return trampoline, stack ownership, per-call instruction budget, and fault reporting. DT_FINI and FINI_ARRAY are decoded but shutdown execution is deferred.

## Validation
Host CI run 42866899433 completed successfully for commit `23cda1b2c2af3852cccf5b3a1981ec134251c0b3`. The logs explicitly show:
- Guest ELF32 PT_DYNAMIC parser tests: PASS
- Guest ELF32 constructor sequence tests: PASS
- Unified ELF32 pipeline test executed as part of the full host suite.

Android ARM64 compilation and execution against actual game assets remain separate gates.
