# Project Roadmap

## Current milestone: Stage 120N — Packed and dynamic relocation metadata parity

- Carry bounded symbol size, definition state, section index, and symbol version into APS2 entries.
- Mark DT_SYMTAB namespace unresolved unless dynamic symbol bounds are proven.
- Apply R_ARM_COPY st_size only with complete symbol provenance.
- Correct APS2 decoding to AOSP Bionic semantics: absolute r_info, grouped info reuse, grouped addend applied once per group, per-entry addend deltas, and zero addend when HAS_ADDEND is absent.
- Verify sectionless PT_DYNAMIC/APS2 and ordinary dynamic/packed symbol metadata parity.

See STAGE120N_REPORT.md and docs/STAGE120N_PACKED_DYNAMIC_SYMBOL_PARITY.md.

## Next milestone: Stage 120O — Independent APS2 conformance fixtures and malformed-input hardening

1. Add differential APS2 vectors transcribed from the AOSP/LLVM format examples.
2. Test all legal group-flag combinations for REL and RELA.
3. Validate signed SLEB128 edge cases and ELF32 r_info bit patterns.
4. Exercise multiple groups, cumulative offsets, grouped addends, and mixed grouped/ungrouped fields.
5. Keep malformed streams bounded and reject overflows, trailing bytes, and impossible counts.
6. Retain a read-only parser; never apply relocations.

## Remaining major gates

1. Build liblzma for Android ARM64 and complete a real Android NDK build.
2. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
3. Finish s3eFileRead partial-item/error behavior using version-matched SDK documentation or controlled original-runtime traces.
4. Activate only individually verified guest imports.
5. Expand guest CPU instruction coverage and runtime diagnostics based on observed execution.
6. Establish graphics/audio/input lifecycle.
