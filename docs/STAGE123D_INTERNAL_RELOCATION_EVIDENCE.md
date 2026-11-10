# Stage 123D — XE3U internal relocation evidence (read-only)

## Input provenance
Analysis uses the re-uploaded original APK and its decoded `assets/biohazard.s3e`. The APK, compressed S3E, decoded XE3U image, and code blob SHA-256 values match Stage 97 exactly. See [Stage 123C original APK revalidation](STAGE123C_ORIGINAL_APK_REVALIDATION.md).

## Observed image layout
- Link-time base: `0x4A000000`.
- Code blob file range: image `0x24D0C`, size `0x170864`.
- Code memory size: `0x18B430`.
- Extended `data_offset`: `0x16C018`.
- File-backed bytes after data_offset: `0x484C`.
- Memory-only tail beyond code_file_size: `0x1ABCC`.
- Extra metadata begins exactly at the end of the code blob.

The exact segment meanings and whether data_offset is the sole code/data boundary still require disassembly/loader evidence; these are observed offsets, not assumptions about protection or section permissions.

## Internal relocation table audit
All 36,377 type-1 entries were read from the original fixup table. For every entry:
1. The patch offset is 4-byte aligned.
2. The 4-byte patch site lies wholly inside the file-backed code blob.
3. Every patch offset is unique.
4. The 32-bit word at that patch site lies in the link-time image address interval `[0x4A000000, 0x4A18B430)`.
5. Therefore, each stored word can be expressed as `link_time_base + in_image_offset`.

Aggregate results:
- Unique, aligned, file-backed patch sites: 36,377 / 36,377.
- Stored words pointing inside the link-time image: 36,377 / 36,377.
- Minimum target RVA: `0xB70`.
- Maximum target RVA: `0x18B41C`.
- Targets below `data_offset`: 32,002.
- Targets at/above `data_offset`: 4,375.
- Stored pointers with bit 0 set (Thumb-state pointers): 18,638.
- Stored pointers with bit 0 clear: 17,739.
- Patch sites at/after `data_offset`: 931.

This is strong binary evidence that type-1 entries identify 32-bit absolute pointer cells and that the likely relocation operation for a runtime load bias `delta = runtime_base - 0x4A000000` is `word := word + delta`, preserving bit 0. It is not yet treated as production proof: validate representative code/data references against ARM/Thumb disassembly and ensure no special cases or alternate relocation domains exist before enabling writes.

## External Thumb relocation cell observations (same read-only pass)

The 238 type-4 records were also inspected as 4-byte little-endian cells in the code blob:
- All 238 cell offsets are unique, 4-byte aligned, and fully file-backed.
- 230 cells contain `0x4A000070` (the link-time image base plus RVA `0x70`).
- 8 cells contain zero.
- The eight zero-valued records are:
  - `glTexParameteri` at `0xC7C4C`.
  - `glTexParameterx` at `0xC7C50`.
  - `s3eMallocBase` at `0x1603B8` and `0x170330`.
  - `s3eReallocBase` at `0x1603BC` and `0x170334`.
  - `s3eFreeBase` at `0x1603C0` and `0x170338`.

The shared `0x4A000070` value is an observed placeholder-like in-image address, not yet a proven import thunk or callable target. The eight zero cells may represent unresolved/optional imports or another linker convention; no such meaning is assigned yet. Do not apply the internal type-1 add-load-bias rule to type-4 records merely because most cells look like in-image pointers. External symbol binding and ARM/Thumb interworking semantics remain a separate analysis task.

## Mixed ARM/Thumb disassembly and external import thunk proof

The code blob is mixed-state ARM32/Thumb code; it must not be disassembled as one ISA mode. The entry region begins with valid ARM instructions. An ARM `BLX` at code offset `0x1C` targets `0x79A00`, where the target disassembles as valid Thumb instructions. This confirms both instruction states are present in the same XE3U code blob.

All 230 nonzero type-4 external relocation cells are served by a repeated three-instruction **ARM-state** import stub. The stub/cell relation is:

`stub_offset = 0x84 + 3 * ((cell_offset - 0x13B708) / 4)`

For every nonzero cell, the audit verifies:
1. Instruction 1 is `0xE28FC601` (PC-relative base calculation).
2. Instruction 2 is one of `0xE28CCA3B` or `0xE28CCA3A` (the two observed address windows).
3. Instruction 3 matches `LDR pc, [r12, #imm]!` (`0xE5BCFxxx`).
4. Decoding the ARM PC value and both address immediates yields the exact external relocation cell offset.

Result: **230/230** nonzero type-4 records resolve to the exact cell addressed by their corresponding stub. All 230 cells currently contain the placeholder word `0x4A000070`. The four previously mapped file APIs reproduce the recorded stub addresses exactly: `s3eFileClose 0xC0`, `s3eFileRead 0x30C`, `s3eFileOpen 0xA5C`, and `s3eFileGetChar 0xAB0`.

The other eight type-4 records are zero-valued cells outside this repeated stub table:
- `glTexParameteri` at `0xC7C4C`, `glTexParameterx` at `0xC7C50`.
- `s3eMallocBase` at `0x1603B8` and `0x170330`.
- `s3eReallocBase` at `0x1603BC` and `0x170334`.
- `s3eFreeBase` at `0x1603C0` and `0x170338`.

Their zero values and symbol names are established; whether these are optional function-pointer variables, special imports, or another linker convention is still unresolved. They must not be populated by guessing.

## Six zero-valued allocator imports are data slots, not ARM code stubs

The six zero cells for `s3eMallocBase`, `s3eReallocBase`, and `s3eFreeBase` form two three-word function-pointer tables:
- Base allocator table at `0x1603B8`, `0x1603BC`, `0x1603C0`.
- Active allocator table at `0x170330`, `0x170334`, `0x170338`.

The ARM disassembly around `0x931F8–0x932E4` provides direct evidence:
- At `0x93214`, code selects the base table at `0x1603B8` when no replacement table is supplied, then copies three words to the active table at `0x170330`.
- At `0x93244`, an indirect call loads active-table slot 0.
- At `0x93264`, an indirect call loads active-table slot +8.
- At `0x93284`, an indirect call loads active-table slot +4.
- At `0x932A8–0x932D0`, code copies the base table into a temporary structure, calls helper `0x660`, and copies the resulting three words into the active table.

The slot order is consistent with the symbol sequence malloc, realloc, free. This resolves the six zero-valued records as imported allocator function-pointer data cells; they are not entries in the repeated three-instruction ARM thunk table. Their actual host function addresses still need to be bound by the runtime, and no guessed values are installed.

The two remaining zero cells (`glTexParameteri` at `0xC7C4C` and `glTexParameterx` at `0xC7C50`) are adjacent data cells near a GL error-checking string/table. Their exact ownership and access path remain unresolved; they are deliberately not classified as ordinary ARM import stubs.

## Shared import resolver indirection observed at code offset 0x70

All 230 per-symbol ARM stubs load the same placeholder target, `0x4A000070`. At code offset `0x70`, a separate ARM sequence:
- pushes the caller LR;
- loads a PC-relative literal `0x0013B67C`;
- adds it to the ARM PC value to form address `0x13B6FC`;
- performs a pre-indexed `LDR pc, [lr, #8]!`, which reads the word at `0x13B704`.

In the original file-backed code blob:
- `0x13B6FC` contains `0x4A18B430`, exactly `base_address + code_memory_size` (the image memory end).
- `0x13B700` contains zero.
- `0x13B704` contains zero.
- Per-symbol import cells begin at `0x13B708`.

The per-symbol stub's `LDR pc, [r12, #imm]!` also writes the selected import-cell address back into `r12` before entering the shared code at `0x70`. This strongly suggests a shared runtime resolver receives the import-cell address in `r12`, and the pointer at `0x13B704` is a special resolver entry slot initialized by the S3E runtime. However, the exact resolver ABI and who initializes that slot have not yet been proven. The separate original `lib/armeabi/libjirobiodg.so` is ELF32 ARM EABI5 and exports a family of `s3eStackSwitchCallS3EFuncR12*` wrappers (for example `s3eStackSwitchCallS3EFuncR12` at ELF value `0x48AFC`, `..._4Args_Fast_Void` at `0x48C6C`, and `..._4Args_Fast_Int32Ret` at `0x48CA4`). Disassembly shows these wrappers treat `r12` as the final callable function address (`mov r10, r12` / `bx r10`, or `bx r12`) while performing stack switching. Therefore they cannot be assumed to be the resolver target while `r12` still contains the import-cell address; a resolver must first translate that cell key into the final function pointer. The library exports `glTexParameterx` but not `glTexParameteri`, so the two zero GL cells also cannot both be assumed to map directly to this library. Do not patch the 230 cells directly to host functions or invent a value for `0x13B704`; first recover the special resolver initialization path and AAPCS32 call contract.


## Refined internal target classification

The 36,377 internal pointer targets split by the observed image extents:
- 32,002 targets in the pre-`data_offset` code-side range.
- 1,549 targets in file-backed initialized data (`data_offset` through `code_file_size`).
- 2,826 targets in the memory-only tail (`code_file_size` through `code_memory_size`), consistent with pointers into the region that must be zero-filled as BSS during materialization.

Patch sites themselves remain entirely file-backed: 35,446 before `data_offset` and 931 at/after it. This distinction is important: a relocation cell may be file-backed while the pointer it stores targets BSS. The loader must materialize the full memory extent and zero-fill the tail before the relocation transaction.

A reusable read-only implementation is in `tools/xe3u_relocation_audit.py`; its tests are in `tests/test_xe3u_relocation_audit.py`. It reports these facts and verifies stub arithmetic without applying any relocation or import binding.

## Required next checks
1. Decode and inspect the instruction/data context around representative patch sites from each region, especially patch sites in the initialized-data tail.
2. Verify that all post-relocation targets remain within the chosen runtime image mapping and that signed/unsigned overflow is rejected before any writes.
3. Establish whether the XE3U `data_offset` partitions code and initialized data, and whether the memory-only tail is BSS.
4. Analyze all 238 type-4 external Thumb cells separately; do not conflate their thunk construction with type-1 internal pointer relocation.
5. Implement a read-only relocation planner with a complete preflight pass. Only after that passes should a transactional applier be added.

## Candidate-only transactional relocation planner

Added `runtime/guest/re_guest_xe3u_reloc_plan.h/.c`. It accepts the decoded code blob, parsed XE3U header, type-1 patch offsets, and a caller-selected 4 KiB-aligned runtime base. It performs a complete preflight:
- Validates the original and runtime image address extents without 32-bit overflow.
- Checks every patch is aligned and fully file-backed.
- Detects duplicate patch offsets using a caller-owned open-addressed scratch table.
- Verifies each original pointer (ignoring only its state bit for range classification) targets the original image memory extent, including the memory-only tail.
- Computes candidate `original_word + (runtime_base - link_time_base)`, preserving bit 0 and checking the new target remains inside the runtime image.
- Publishes the plan only after every record passes; output remains unchanged on failure.

This is a **planner only**. It does not mutate the decoded image or guest memory, does not bind external imports, and is not wired into the production loader. Runtime base selection and final code/data/BSS mapping policy remain external inputs. Tests cover positive/negative load bias, ARM/Thumb state preservation, BSS targets, duplicates, invalid patch/target bounds, capacity, and alignment.

The read-only Python audit and C planner are registered in CI. Latest source-check run `42873066679` passed, including `Guest XE3U internal relocation planning tests: PASS`, `Guest XE3U header and fixup parser tests: PASS`, and all existing host regressions. This is host validation, not an Android NDK build or device/game boot.

## Native C planner exercised against the original binary

A local native harness compiled the Stage 123D C planner with Clang using `-std=c11 -Wall -Wextra -Werror`, then fed it the exact code blob and all 36,377 type-1 offsets extracted from the re-uploaded original APK. The candidate runtime base `0x50000000` was used only as a validation input; it is not a production base decision.

Result:
- Planner status: `RE_GUEST_XE3U_RELOC_PLAN_OK`.
- Planned entries: 36,377 / 36,377; failed index equals count (no failure).
- Thumb-state entries preserved: 18,638.
- ARM-state entries preserved: 17,739.
- Targets in the memory-only/BSS tail accepted after full range validation: 2,826.
- Example first entry: `0x4A170868 -> 0x50170868`.
- Example last entry: `0x4A13B018 -> 0x5013B018`.

This exercised the C planner against the real game offsets and words, not a synthetic fixture. The harness emitted a plan only; it did not modify the original code blob, map guest memory, apply relocations, or execute the game. The separate `tools/xe3u_native_audit` CLI still has not been run against the original asset in the repository build environment.

## Safety boundary
No bytes were modified. No relocation was applied. No game code was executed. The analysis is a read-only reconstruction from the original binary.
