# Stage 123F — transactional external import trampoline binding

## Design decision: bypass the original shared resolver

The original XE3U image routes 230 nonzero type-4 import cells through a shared ARM helper at code offset `0x70`, which loads a separate resolver pointer from `0x13B704`. That slot is zero in the original image and its original runtime initialization is not present in the APK.

The ARM64 port does not need to reproduce that proprietary lazy-resolver path. It can replace each type-4 cell with the already-supported symbol-specific Thumb trampoline from `REGUESTIMPORTTable`:
- The original ARM stub executes `LDR pc, [r12, #imm]!`.
- The loaded trampoline pointer is Thumb-tagged (`trampoline_address | 1`).
- ARMv5T+ PC interworking enters Thumb state.
- `re_guest_runtime_step` recognizes the trampoline PC and dispatches the registered AAPCS32 handler.
- The original `0x70` helper and unresolved `0x13B704` slot are bypassed.

The guest CPU now implements the required ARM/Thumb interworking for ARM LDR-to-PC, LDM-to-PC, Thumb-2 LDR-to-PC, and Thumb POP-to-PC paths. It also implements the pre-indexed writeback word transfers used by the original stub/helper and Thumb PUSH/POP register lists.

## Implemented

Added:
- `runtime/guest/re_guest_xe3u_import_plan.h/.c`.

The read-only planner validates every type-4 record:
1. Checks symbol index, patch alignment/range, subsection type, and duplicate cells.
2. Accepts only the two observed original cell forms: `base_address + 0x70` for a verified three-instruction ARM import stub, or zero for a function-pointer data cell.
3. Reconstructs the corresponding ARM stub and verifies that its writeback address equals the external relocation cell.
4. Looks up the symbol index in the existing import table.
5. Produces a Thumb-tagged symbol-specific trampoline pointer for bound APIs.
6. Optionally routes unbound APIs to the runtime's unresolved-import trap; otherwise it fails closed.
7. Publishes the full plan only after all records pass.

The apply routine preflights every mapped guest cell against its expected original value before the first write. If a later write fails, it rolls back earlier cells. It does not silently leave a partially patched import table.

The eight zero-valued records are treated as function-pointer data cells and use the same symbol-specific trampoline mechanism when bound. Six are the two three-slot allocator tables; two GL cells remain semantically unclassified, so they must remain unresolved until handlers are verified.

## Validation

- Local native C harness, using the exact original code blob and all original external records with a synthetic 227-symbol binding table: **238/238 planned**, comprising 230 verified ARM stubs and 8 zero-valued data cells; zero unbound entries. This validates structural planning, not real API implementations.
- Host CI run `42877554684` passed:
  - Guest XE3U external import planning/apply tests: PASS.
  - Guest XE3U direct Thumb import dispatch tests: PASS.
  - Guest XE3U import cell lookup tests: PASS.
  - Guest XE3U internal relocation planning tests: PASS.
  - Stage 60 guest CPU tests, including ARM/Thumb interworking and the original import-thunk instruction sequence: PASS.
- Android ARM64 NDK build/ELF verification passed in run `42877209331`, including the XE3U external plan/apply source.

## Still required

- Implement and verify the actual ARM64-side handlers for the 227 imported symbols; a synthetic binding table is not a production API manifest.
- Bind allocator callbacks and classify the two GL data cells before enabling them.
- Connect internal relocation planning/application, BSS zero-fill, code/data permissions, external import plan/apply, and the XE3U decoder into one rollback-safe image loader.
- Integrate that loader with the ARM32 runtime and run on a physical ARM64 device.

## Safety boundary

No original APK bytes are modified. The plan/apply tests use synthetic mapped guest memory. The local original-image harness only generated a plan and used dummy handler addresses; it did not apply the plan or execute game code. No game boot or gameplay has been achieved.
