# Stage 125A.3 — ELF32 ARM identity verification

## Scope

Adds a read-only identity gate for materialized ELF32 guest modules:

- Reuses the bounded ELF32 image parser for magic, ELFCLASS32, EI_DATA, version, ET_EXEC/ET_DYN, EM_ARM and program-header validation.
- Reads ARM `e_flags` using the image byte order and accepts the unknown legacy value or ARM EABI5; rejects other EABI version encodings.
- Reuses the dynamic-section parser and validates DT_SONAME against the manifest's expected SONAME for shared modules.
- Allows the root executable to omit an expected SONAME and lets the caller require ET_EXEC or ET_DYN.
- Uses a temporary result and publishes identity only after all checks succeed.

## API

- `runtime/guest/re_guest_elf32_identity.h`
- `runtime/guest/re_guest_elf32_identity.c`
- `re_guest_elf32_verify_identity(...)`

## Regression tests

`tests/test_re_guest_elf32_identity.c` builds a synthetic little-endian ARM EABI5 ET_DYN image with PT_LOAD, PT_DYNAMIC, DT_STRTAB, DT_STRSZ, DT_SONAME and DT_NULL. It checks successful SONAME matching, SONAME mismatch, expected-type mismatch, and unsupported EABI version. The test is registered in `tools/run_tests.sh`; Android CMake compiles the new identity module.

## Explicit limitations

This is an identity gate, not a full ABI compatibility certification. It does not yet validate ARM hard-float/soft-float calling convention policy, CPU feature attributes, symbol-version tables, GNU hash, relocation compatibility, dependency graph closure, hashes/signatures, or load the image. Dynamic and link metadata parsers retain their documented format limits. No real game payload or game boot is claimed.

## Validation

Stage 125A.2 passed both host Source checks and Android ARM64 native build on commit `fd8d6954a8cc931d91b0bbb3801510c496ec48a6`. Stage 125A.3 CI is pending on the latest branch head.
