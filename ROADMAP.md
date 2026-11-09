## Stage 119T — Guest ELF and ARM ABI metadata audit

- Added a read-only ELF32/ELF64 metadata auditor.
- Reports class, endianness, machine, flags, section names, and selected AEABI ARM attributes.
- Added synthetic little-/big-endian ELF fixtures and malformed-input tests.
- Does not execute or modify the supplied file; original game executable remains absent.
- Does not disassemble code or prove runtime SCTLR/MMU state.

### Next — Stage 119U: ARM attribute coverage and disassembly evidence

1. Validate remaining ARM attribute tag types against the ABI specification and expand fixtures.
2. Add optional tool-versioned readelf / llvm-readobj evidence capture for program headers, symbols, and relocations.
3. Add a pinned disassembler path for ARM/Thumb instruction inventory when a guest binary is available.
4. Preserve source binary SHA-256 and tool versions in reports; never commit proprietary binaries.
5. Keep runtime system-control state unknown unless actual startup/runtime traces establish it.
