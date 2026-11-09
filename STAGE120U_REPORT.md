# Stage 120U Report — ELF Segment Materialization

Status: implementation committed; CI validation pending.

## Changes
- Added a bounded ELF segment materialization helper.
- Preflights source ranges, file_size <= memory_size, guest address bounds,
  destination mapping permissions, and inter-segment destination overlap.
- Copies file-backed bytes and zero-fills BSS tails only after all segments
  pass preflight.
- Added native C regression coverage and integrated it into tools/run_tests.sh.

## Validation
Pending GitHub Actions. No pass is claimed until the full Python suite,
host mock compilation, and native C regressions succeed.

## Scope
No allocation, guest mapping, ELF parsing, load-bias selection, host page
permission changes, relocation application, or guest execution.
