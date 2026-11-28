# Stage 125A.4 — Materialized module identity and dependency preflight

## Scope

Adds a read-only preflight pass over a manifest and its transactionally materialized module set.

For each module, the preflight:
- Requires a non-empty buffer and checks the actual buffer length against that manifest entry's non-zero maximum.
- Verifies ELF32/ARM image identity and ARM EABI version using Stage 125A.3.
- Requires every non-root module's DT_SONAME to match the manifest SONAME.
- If the root has a DT_SONAME, requires it to match the root manifest entry.
- Extracts DT_NEEDED names from the bounded dynamic parser and builds the existing dependency-first module graph.
- Rejects unresolved dependencies, cycles, duplicate module names, and manifest entries unreachable from the root.
- Publishes the result only when all checks succeed and reports a failing module index where available.

## API and integration

- \`runtime/guest/re_guest_module_preflight.h/.c\`
- Host regression test: \`tests/test_re_guest_module_preflight.c\`
- Registered in \`tools/run_tests.sh\`.
- Included in Android native CMake target.

## Tests

The synthetic root DSO test verifies successful preflight and dependency graph completion, manifest byte-limit rejection, and invalid ELF machine rejection. The test does not use proprietary game assets.

## Non-goals and safety boundary

This is still a preflight/validation stage. It does not map guest segments, resolve imports, apply relocations, set page permissions, run constructors, or execute guest code. It does not establish complete ABI compatibility (hard-float/soft-float, CPU attributes, symbol versions, GNU hash, TLS, or relocation semantics). No original game payload or first boot is claimed.

## Validation

Stage 125A.3 passed Host Source Checks and Android ARM64 native build on commit \`d007150290d79973e61233f47609aae399847628\`. Stage 125A.4 validation is pending on the latest branch head.
