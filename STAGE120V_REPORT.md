# Stage 120V Report — Guest Memory Permission Finalization

Status: implementation committed; CI validation pending.

## Changes
- Added an atomic permission finalization API for exact existing guest mappings.
- Preflights all requests before updating permission metadata.
- Rejects W+X, duplicate requests, missing mappings, invalid flags, and invalid ranges.
- Added native C regression coverage and integrated it into tools/run_tests.sh.
- Included implementation, tests, and reports in CI SHA-256 provenance.

## Validation
Pending GitHub Actions. No test pass is claimed until full Python discovery,
host mock compilation, and native C regression checks succeed.

## Scope
This updates guest software permission metadata only. It does not call
mprotect/VirtualProtect, alter host page protections, flush instruction
caches, apply relocations, or execute guest code.
