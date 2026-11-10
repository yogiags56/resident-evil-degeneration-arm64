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

## Required next checks
1. Decode and inspect the instruction/data context around representative patch sites from each region, especially patch sites in the initialized-data tail.
2. Verify that all post-relocation targets remain within the chosen runtime image mapping and that signed/unsigned overflow is rejected before any writes.
3. Establish whether the XE3U `data_offset` partitions code and initialized data, and whether the memory-only tail is BSS.
4. Analyze all 238 type-4 external Thumb cells separately; do not conflate their thunk construction with type-1 internal pointer relocation.
5. Implement a read-only relocation planner with a complete preflight pass. Only after that passes should a transactional applier be added.

## Safety boundary
No bytes were modified. No relocation was applied. No game code was executed. The analysis is a read-only reconstruction from the original binary.
