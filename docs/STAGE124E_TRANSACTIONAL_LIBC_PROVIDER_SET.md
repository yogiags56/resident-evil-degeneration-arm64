# Stage 124E — Transactional libc provider set

## Goal

Make the implemented libc adapters bindable as one coherent `libc.so`
provider family rather than requiring a future loader to call four unrelated
binding routines and manage partial failure itself.

## Implementation

Added `re_guest_libc_provider_set`, a caller-owned aggregate that binds the
current allocation, memory/string, formatted-output, and stdio providers
against one ELF32 dynamic-symbol table:

- allocation: 4 imports (`malloc`, `free`, `realloc`, `valloc`)
- memory/string: 15 imports
- formatted output: 3 imports
- stdio: 14 imports, including Stage 124D `ungetc`

The aggregate preflights total import-table capacity, snapshots the import
table, binds each family in order, and publishes the export list only after
all four families succeed. If a family fails, the original import table is
restored and the caller's export buffer remains unchanged.

## Regression coverage

A new host test initializes all four providers, verifies the complete 36-name
export set and import count, then removes the final stdio symbol and confirms
that failure rolls back the import table and leaves the output array
untouched. The test is wired into `tools/run_tests.sh`.

The new source is also included in the Android ARM64 native CMake target.

## Scope and limitations

This is an aggregate binder, not yet the production game's loader. The caller
must supply initialized provider instances, a shared ELF32 symbol table, and
stable provider lifetimes. It does not synthesize missing libc functions,
provide a complete Bionic implementation, or claim that all imports required
by the original game are now resolved. The game has not been booted.

## Validation

Host CI and Android ARM64 build/ELF verification are pending for the final
commit.
