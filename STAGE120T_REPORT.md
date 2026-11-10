# Stage 120T Report — Guest Memory Mapping Transaction

Status: implementation committed; CI validation pending.

## Changes
- Added a multi-region guest memory mapping transaction API.
- Delegates individual mapping validation to the existing guest memory manager.
- On failure, rolls back mappings created by this transaction in reverse order.
- Preserves mappings that existed before the transaction.
- Reports failing request index and underlying guest-memory status.
- Added native C regression coverage and integrated it into tools/run_tests.sh.

## Validation
Pending GitHub Actions. No test pass is claimed until the full Python suite,
host mock compilation, and native C regressions succeed.

## Scope
The API uses borrowed host buffers. It does not allocate memory, load ELF
segments, copy file bytes, clear BSS, change host page permissions, apply
relocations, or execute guest code.
