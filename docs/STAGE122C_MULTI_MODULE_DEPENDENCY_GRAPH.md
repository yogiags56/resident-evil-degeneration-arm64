# Stage 122C — Bounded multi-module dependency graph

## Objective
Introduce a C runtime dependency-graph primitive so the guest ELF loader can validate the caller-selected set of ELF modules and derive deterministic dependency-first load order before mapping any module.

## Implemented
- Added `re_guest_elf32_module_graph.h/.c`.
- Accepts a bounded inventory of module SONAMEs and their parsed DT_NEEDED strings.
- Requires unique basename-only SONAMEs and an explicitly selected root.
- Traverses only the root-reachable dependency closure; unrelated supplied modules are not added to the load order.
- Preserves repeated DT_NEEDED entries in the caller's input while traversing each provider edge once.
- Reports unresolved dependency edges by source module index and DT_NEEDED ordinal.
- Detects dependency cycles and rejects a cycle-bearing graph.
- Produces deterministic dependency-first postorder for an acyclic graph.
- Uses fixed-capacity storage; no allocation, filesystem access, host linker search, mapping, relocation, or execution occurs.
- Added host regression coverage for dependency ordering, repeated dependencies, unresolved edges, cycles, duplicate SONAMEs, path-like names, and missing roots.

## Capacity
- Maximum 32 modules.
- Maximum 32 DT_NEEDED entries per module.
- Maximum 256 bytes per SONAME including terminator capacity.

## Deliberate limitations
This stage resolves graph topology only. It does not select files, map module images, resolve symbols across modules, apply relocations, create import trampolines, process GNU symbol versions, or execute constructors. Those operations require a separate transactional multi-image loader and explicit provider policy.

## Validation
The new regression is registered in `tools/run_tests.sh`. Stage completion is contingent on the current host CI run passing.
