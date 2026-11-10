# Stage 121C — ELF32 relocation planning bridge

## Goal
Connect the bounded ELF32 dynamic metadata decoder to the existing symbol resolver and relocation-application contract without silently turning the research loader into an incomplete Android dynamic linker.

## Implemented
- Converts decoded REL/RELA/PLT relocation records into caller-prepared REGUESTRELOCATION batches.
- Adds the selected load bias to relocation target addresses with ELF32 overflow checks.
- Resolves undefined named symbols against an ordered, caller-supplied set of already-loaded provider definitions using the existing strong/weak symbol resolver.
- Resolves defined symbols within the current image, preserving absolute (SHN_ABS) values and applying load bias to ordinary section-defined values.
- Handles unresolved weak imports as zero; unresolved strong imports fail.
- Rejects unsupported relocation types, TLS symbols, invalid symbol indices, versioned dynamic metadata, and invalid target arithmetic.
- Keeps caller output unchanged if any relocation cannot be prepared.
- Adds host regression tests and includes the bridge in the Android ARM64 native target.

## Deliberate limits
This bridge does not locate or load DT_NEEDED libraries, allocate/map provider images, implement Android linker namespaces or global symbol interposition, decode GNU symbol versions, perform lazy PLT binding, handle TLS/COPY/IFUNC, or execute constructors. Provider values must already be final guest virtual addresses and must be ordered by the caller. Defined symbols are resolved within the current image; this is not full ELF interposition semantics.

## Validation
- Host regression suite, including `test_re_guest_elf32_reloc_plan`: PASS on commit `04ad19c2c17b4e000a0dd2f113435e91d5bf3686` (workflow run `42815789032`).
- Android ARM64 native build/ELF verification: running on commit `04ad19c2c17b4e000a0dd2f113435e91d5bf3686` (workflow run `42815791052`).

Stage 121C remains pending until the Android build finishes successfully.
