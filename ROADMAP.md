# Project Roadmap

## Current milestone: Stage 120M — Independent ELF cross-check and COPY symbol-size validation

GitHub Actions run 42605382359 passed 218 Python tests (including a real readelf cross-check when available) and host C/native regression checks. See STAGE120M_VALIDATION_LOG.txt.

- Compare section-backed ELF32 EM_ARM relocation offsets/types against GNU readelf or LLVM llvm-readelf.
- Keep tool absence as an explicit skip; no host tool is a runtime dependency.
- Resolve R_ARM_COPY width only from a defined, nonzero, namespaced symbol with positive st_size.
- Keep COPY width unknown when symbol-size provenance is incomplete; APS2 size provenance remains unresolved.
- Preserve unknown external relocation names and comparison mismatches.

## Next milestone: Stage 120N — Packed and dynamic relocation symbol metadata parity

1. Carry bounded dynamic symbol size, definition state, and version metadata consistently into APS2 normalized records.
2. Prove symbol namespace identity before using a symbol index to resolve COPY metadata.
3. Add sectionless PT_DYNAMIC and APS2 fixtures for symbol metadata parity.
4. Cross-check dynamic relocation tags and records without requiring section headers.
5. Preserve explicit unknown/unresolved states for missing hashes, symbol bounds, and provider resolution.
6. Keep all target bytes read-only; do not apply relocations.

## Remaining major gates

1. Build liblzma for Android ARM64 and complete a real Android NDK build.
2. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
3. Finish s3eFileRead partial-item/error behavior using version-matched SDK documentation or controlled original-runtime traces.
4. Activate only individually verified guest imports.
5. Expand guest CPU instruction coverage and runtime diagnostics based on observed execution.
6. Establish graphics/audio/input lifecycle.
