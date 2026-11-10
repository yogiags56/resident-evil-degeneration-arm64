# Stage 120X — ELF Dynamic Symbol Resolution

## Objective
Resolve already-parsed undefined ELF32 symbol requests against an ordered,
caller-verified set of provider symbols.

## Implemented rules
- Match exact symbol names.
- Match exact version names when requested; optional unversioned fallback is
  explicit on the request and never implicit.
- Match provider SONAME when a version-library constraint is supplied.
- Ignore undefined, LOCAL, INTERNAL, and HIDDEN provider symbols for external
  lookup.
- Prefer one GLOBAL definition over WEAK definitions.
- Use first provider order for weak-only matches.
- Reject multiple matching GLOBAL definitions as ambiguous.
- Resolve batches atomically: output array is unchanged if any request fails.

## Limits
The resolver does not parse ELF, translate GNU version indices, load DT_NEEDED
libraries, search host process symbols, implement copy/TLS/IFUNC semantics,
or construct relocation records. A separate trusted adapter must convert
audited ELF symbol/version records into these descriptors. Provider ordering
and dependency scope must be established by that adapter.
