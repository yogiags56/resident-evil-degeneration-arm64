# Stage 123E — XE3U import-cell identity and ARM thunk execution

## Implemented

### Read-only import-cell identity lookup
Added:
- `runtime/guest/re_guest_xe3u_import_lookup.h`
- `runtime/guest/re_guest_xe3u_import_lookup.c`

Given the guest `r12` value left by the original `LDR pc, [r12, #imm]!` import stub, the helper:
1. Converts the guest cell address to a code-blob offset using the selected runtime image base.
2. Finds the matching type-4 external relocation record.
3. Rejects missing, duplicate, unsupported-type, and invalid-symbol records.
4. Verifies the original cell contains the observed link-time resolver placeholder `base_address + 0x70`.
5. Reconstructs and verifies the corresponding three-instruction ARM stub and exact effective cell address.
6. Returns the external symbol index and cell/stub offsets.

It performs no import binding, guest memory writes, or function calls. It must inspect the immutable original code blob before external cells are patched.

A native Clang harness exercised the committed lookup algorithm against the original APK's exact 238 external records and code blob: **230/238** resolved to verified import stubs; the other eight correctly failed the stub check (six allocator data slots and two unresolved GL data cells). No import target was installed.

### ARM32 import thunk instruction support
The guest CPU now supports the original thunk's pre-indexed immediate word writeback transfers:
- `STR lr, [sp, #-4]!` — writeback is committed only after a successful memory write.
- `LDR pc, [r12, #imm]!` — the loaded ARM-state target is entered and `r12` is updated to the cell address only after a successful aligned read.
- General pre-indexed word load/store writeback is restricted to supported non-PC base forms; unpredictable load base/destination overlap fails closed.

A synthetic end-to-end regression executes the exact observed instruction sequence at the per-symbol stub (`0x84`) and shared helper (`0x70)), follows the resolver indirection cell, restores the saved LR, and returns a known value. It verifies the expected `r12`, SP, PC, and return value. The test does not execute the proprietary game image.

## Validation

- Host source checks passed in GitHub Actions run `42875447333`, including:
  - Guest XE3U import cell lookup tests: PASS.
  - Guest XE3U internal relocation planning tests: PASS.
  - Stage 60 guest CPU tests, including full synthetic import-thunk sequence: PASS.
  - Existing guest CPU, memory, runtime, ELF32, file bridge, and archive regressions: PASS.
- Android ARM64 NDK build and ELF verification passed in run `42875193411`, including the new import lookup and CPU instruction support.

## Remaining blockers

- The shared resolver pointer at XE3U code offset `0x13B704` is zero in the original file. Its runtime initialization path and resolver AAPCS32 contract remain unknown.
- The 230 API names must be connected to verified ARM64-side guest import handlers; unknown APIs must fail closed.
- The six allocator data cells need explicit allocator callback binding; two GL data cells still need classification.
- The original game has not been booted. No device execution, menu, rendering, input, audio, or gameplay validation has occurred.

## Safety boundary

The import lookup is identity-only. The internal relocation planner remains candidate-only. No external import was bound, no original image bytes were modified, and no proprietary game code was executed.
