# Stage 120Z — Dependency Graph Resolution

## Objective
Turn caller-supplied Stage 120Y adapter inventories into an explicit,
auditable DT_NEEDED dependency graph.

## Implementation
- Requires an explicit root SONAME and a supplied set of SONAME/adapter pairs.
- Rejects duplicate provider SONAMEs and path-like SONAME values.
- Resolves edges only against the supplied inventory; unresolved names are
  reported with the source library and DT_NEEDED ordinal.
- Detects dependency cycles and reports the cycle path.
- Emits a deterministic dependency-first order only when no cycle exists.
- Preserves the original DT_NEEDED arrays, including repeated entries.

## Validation
Fixture tests cover dependency order, missing providers, cycles, duplicate
SONAMEs, path-like names, missing roots, and repeated DT_NEEDED entries.

## Safety boundary
No filesystem lookup, Android linker namespace, search path, host symbol
resolution, library loading, memory mapping, relocation, or execution occurs.
The graph is a validated inventory of only the files the caller supplied.
