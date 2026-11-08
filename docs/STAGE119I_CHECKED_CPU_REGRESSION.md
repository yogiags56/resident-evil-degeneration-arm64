# Stage 119I — Checked CPU ARM/Thumb regression expansion

## Scope
Expanded the checked CPU integration test to exercise the checked execution gate in ARM state, Thumb-1, Thumb-2 wide instruction fetch, and Thumb IT-block execution.

## Test cases
- Thumb-1 MOVS and ADDS execute through the checked entry point.
- ARM MOV immediate executes through checked entry.
- Thumb-2 MOVW executes through checked entry, requiring a checked second halfword fetch.
- IT EQ executes the following instruction when Z is set.
- IT EQ skips the following instruction when Z is clear.
- EXEC permission denial leaves PC/register/step count unchanged.
- A mismatch between bus-backed bytes and the legacy contiguous view is rejected.
- A Thumb-2 second halfword in a non-executable region is rejected.
- Null CPU argument is rejected.

## Architectural status
The checked gate is still opt-in and is not yet selected by the runtime. The decoder still uses legacy contiguous memory for instruction and data accesses after preflight. This stage adds regression evidence, not a complete bus-backed CPU memory implementation.

## Next
- Review and run the expanded host suite.
- Add ARM/Thumb alignment and boundary cases.
- Add runtime mapping for checked CPU error statuses.
- Introduce a bus-backed decoder access layer and migrate data loads/stores incrementally.
