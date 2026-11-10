# Stage 119O — Checked scalar transfer semantics

## Scope

Add an isolated CPU-facing scalar memory helper that executes byte, halfword, and word reads/writes using the Stage 119N memory-type metadata and SCTLR.A alignment policy. It is intentionally not wired into the production CPU interpreter or the legacy bus API.

## New files

- `runtime/guest/re_guest_cpu_scalar.h`
- `runtime/guest/re_guest_cpu_scalar.c`
- `tests/test_re_guest_cpu_scalar.c`

The implementation is registered in Android CMake and the host test runner.

## Behavior

- Byte accesses have no alignment restriction.
- Naturally aligned halfword and word accesses are allowed, subject to memory bounds and READ/WRITE permissions.
- With strict alignment enabled (the default), unaligned halfword/word accesses return `RE_CPU_E_ALIGNMENT`.
- With SCTLR.A explicitly disabled, unaligned halfword/word accesses are allowed only when the complete access range belongs to one NORMAL memory region.
- UNKNOWN, DEVICE, STRONGLY_ORDERED, unmapped, and cross-region unaligned accesses are rejected.
- Reads/writes assemble/disassemble bytes explicitly in little-endian order; host CPU alignment and host endianness are not relied upon.
- The helper resolves the entire access and checks permission before touching bytes. Failed stores do not mutate backing memory. Failed reads do not update the caller's destination.
- No LDRD, exclusive, multiple-register, SWP, VFP, or coprocessor transfer is implemented here.

## Tests

The scalar regression test covers aligned byte/halfword/word transfers, strict-mode faults, permissive NORMAL-memory unaligned reads/writes, little-endian byte layout, DEVICE/UNKNOWN rejection, read-only permission rejection, unmapped and cross-region failures, destination preservation, address-space edge behavior, and restoring strict policy.

## Safety boundary

This is a decision-and-transfer helper only. The checked memory bus remains unchanged and strict. The A32 instruction decoder from Stage 119K/L is not changed and still uses the legacy strict CPU-memory facade. The new helper is not yet called by `re_guest_cpu_step`. Guest SCTLR system-register emulation and reset-value evidence remain unimplemented.

The emulator uses a single-region checked resolve as its current safe memory abstraction. Exact architectural partial-abort and multi-access behavior across page boundaries is not modeled; no guest MMU/page-table implementation exists yet.

## Next stage

Stage 119P should add isolated A32 transfer instruction variants that explicitly opt into this scalar helper, starting with ordinary LDR/STR word and then halfword forms. Differential tests against the legacy interpreter must be expanded. Production CPU migration remains gated.
