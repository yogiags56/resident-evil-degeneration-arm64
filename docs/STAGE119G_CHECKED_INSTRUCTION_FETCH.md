# Stage 119G — Checked instruction fetch prototype

## Implemented
Extended the ABI-preserving CPU bus adapter with `re_guest_cpu_fetch_instruction`:
- ARM state fetches one aligned 32-bit instruction using EXEC permission.
- Thumb state fetches the first aligned 16-bit halfword using EXEC permission.
- Thumb-2 32-bit prefixes (11101, 11110, 11111) cause a second independently checked 16-bit EXEC fetch at address + 2.
- Failed fetch clears both instruction and width outputs.
- Address overflow before the second halfword is rejected.
- Existing `REGUESTMEM` and `REGUESTCPU` layouts and interpreter execution path are unchanged.

Added `tests/test_re_guest_cpu_instruction_fetch.c` and registered it in `tools/run_tests.sh`. The test covers ARM word fetch, Thumb 16-bit fetch, Thumb-2 two-halfword fetch, non-executable data, cross-region EXEC denial, and invalid arguments.

## Important limitation
The production interpreter `re_guest_cpu_step` still fetches through its legacy contiguous-memory helper. This stage proves the checked fetch API independently; it does not yet switch actual instruction execution to the bus. CPU fault diagnostics and Thumb-2 fetch side effects remain unchanged until the next migration stage.

## CI
The final commit's Source Checks and Android ARM64 workflow must be inspected after completion. Do not infer a green result from earlier intermediate commits.

## Next
Stage 119H will introduce an opt-in CPU memory context or an alternate step entry point that accepts the checked bus, initially migrating only instruction fetch while preserving the legacy entry point for regression comparison.
