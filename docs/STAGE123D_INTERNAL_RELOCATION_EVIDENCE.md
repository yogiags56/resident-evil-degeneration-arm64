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

## Safety boundary
No bytes were modified. No relocation was applied. No game code was executed. The analysis is a read-only reconstruction from the original binary.
