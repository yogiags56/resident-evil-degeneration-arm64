# Project Roadmap

## Current milestone: Stage 120L — Relocation symbol and target contract cross-validation

Extend the Stage 120K AAELF32 registry with conservative symbol-index policies, target policies, and processing applicability. The unified report annotates each relocation entry and counts observed contract issues. Unknown types remain unknown.

## Next milestone: Stage 120M — Independent ELF-tool cross-check and COPY symbol-size validation

1. Add synthetic ELF fixtures with section-backed and PT_DYNAMIC relocations.
2. Compare relocation offsets, symbol indices, and relocation names against GNU readelf and/or LLVM llvm-readelf when available.
3. Add explicit skip behavior when independent tooling is unavailable; never make host tools a hidden runtime dependency.
4. Resolve R_ARM_COPY width only when a valid referenced symbol st_size is available and namespace identity is proven.
5. Validate malformed symbol references, out-of-range indices, and missing symbol tables.
6. Preserve unknown, reserved, private, and platform-specific types.
7. Keep target bytes read-only and do not apply relocations.

## Remaining major gates

1. Build liblzma for Android ARM64 and complete a real Android NDK build.
2. Validate AssetManager -> DTRZ VFS -> file adapter on a physical ARM64 device.
3. Finish s3eFileRead partial-item/error behavior using version-matched SDK documentation or controlled original-runtime traces.
4. Activate only individually verified guest imports.
5. Expand guest CPU instruction coverage and runtime diagnostics based on observed execution.
6. Establish graphics/audio/input lifecycle.
