# Stage 122H — Bounded AArch32 constructor execution

## Implemented
- Added `re_guest_elf32_init_exec.h/.c`.
- Added a dedicated mapped-import return trampoline binding helper.
- Executes each prevalidated Stage 122G constructor in a fresh mapped guest runtime context.
- Initializes AArch32 entry PC and Thumb state from the constructor function pointer.
- Supplies a caller-owned, 8-byte-aligned guest stack.
- Uses AAPCS32-compatible zero-argument constructor entry (r0-r3 initially zero).
- Constructor returns are detected through a mapped import trampoline using LR.
- Supports guest imports invoked by constructors through the existing mapped import dispatcher.
- Enforces a mandatory per-constructor instruction budget.
- Reports completed count, failing call index, total guest steps, fault PC, CPU status, import status, and runtime status.
- Keeps the return-handler state caller-owned because the import table retains its pointer; caller must keep it alive while the binding exists.
- Added a Thumb smoke test executing MOVS + BX LR and verifying trampoline return.
- Added a non-terminating branch test verifying budget enforcement and current-PC diagnostics.
- Registered source in Android native CMake and the host regression suite.

## Runtime contract and safety limits
- Caller must provide a mapped executable constructor sequence, mapped guest stack with at least 256 writable/readable bytes below stack_top, initialized memory bus, initialized import table, and a persistent return-handler state object.
- Each constructor receives a fresh CPU context; guest memory changes persist across constructors.
- Constructor CPU register state is not preserved between calls except for memory side effects. This matches the intended independent C initializer call model.
- The executor does not provide threads, signals, TLS, Bionic loader globals, process environment, or complete S3E import coverage. A real game constructor may still fail on unresolved imports or unsupported ARM instructions.
- Per-call step budgets prevent a single constructor from hanging the host runtime. A budget failure stops the initialization sequence immediately.
- DT_FINI and DT_FINI_ARRAY are parsed, but shutdown callbacks are not executed by this API.

## Validation
Host CI run 42867445603 passed for commit `4fceaae39de7a81cc5bdcca13b7fb6db5094c504`. Logs explicitly show:
- Guest ELF32 constructor sequence tests: PASS
- Guest ELF32 bounded constructor execution tests: PASS

This is a synthetic runtime regression only. It does not demonstrate that the proprietary game has booted or that an APK is playable.
