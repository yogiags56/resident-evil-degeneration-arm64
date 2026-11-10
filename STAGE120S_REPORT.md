# Stage 120S Report — ELF32 ARM PT_LOAD Load-Plan Preparation

Status: implementation committed; CI validation pending.

## Changes
- Added a read-only ELF32 EM_ARM PT_LOAD layout planner.
- Reports file-backed, memory, BSS zero-fill, and page-rounded virtual ranges.
- Reports page-rounded PT_LOAD overlaps and combined permissions, including W+X.
- Keeps ET_EXEC fixed-address and ET_DYN load-bias policy explicit.
- Adds generated regression fixtures for BSS, page overlap, address limits,
  entry-point coverage, page-size constraints, and malformed ELF types.

## Validation
Not yet run. Full Python test discovery, host C mock compilation, and native
C regression checks must pass before this stage can be marked complete.

## Scope
No guest memory allocation or modification, no file copy, no BSS clearing,
no relocation application, no load-bias calculation, and no entry execution.
