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


## Stage 119U — ARM attribute scopes and evidence capture

- Extended AEABI parser for file, section, and symbol scope subsections.
- Implemented Tag_compatibility's ULEB128 + vendor-string payload and argument-free Tag_nodefaults.
- Preserved unknown vendor-private payload bytes without interpreting them.
- Added safe readelf/llvm-readobj metadata capture with tool version, timeout, source SHA-256, exit status, and raw output.
- Added synthetic regression coverage; CI pending.
- No actual game ELF is present; runtime system-control state remains unknown.

### Next — Stage 119V: instruction inventory and executable metadata cross-check

1. Add an optional pinned disassembler path (Capstone or LLVM objdump) with explicit ARM/Thumb mode selection.
2. Distinguish decoded instruction counts from raw byte coverage and decode failures.
3. Cross-check ELF attributes against observed instruction classes without treating attributes as proof of use.
4. Record tool versions and hashes; do not store proprietary binaries or full symbol dumps in the repository.
5. Use evidence to prioritize missing guest CPU instructions before enabling production dispatch.
