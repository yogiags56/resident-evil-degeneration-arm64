# Stage 124K — fflush(NULL) stream synchronization audit

## Changes
- `fflush(NULL)` now iterates over every active stream and invokes the optional backend flush callback.
- A stream whose flush fails receives its error indicator and the aggregate call returns EOF (`-1`).
- A stream whose flush succeeds has its update-mode I/O direction reset, allowing a subsequent input/output operation to begin a new sequence.
- Regression coverage sets a stream direction to output, calls `fflush(NULL)`, and verifies the direction state is reset.

## Compatibility notes
- This provider currently treats a missing backend flush callback as an unbuffered backend, consistent with its backend contract.
- The implementation does not yet model all ISO C `fflush` input-stream semantics; those need a separate contract and tests for seekable and non-seekable streams.
- Multiple-stream failure behavior, including continuing to flush remaining streams after one failure, is retained.

## Validation
Host regression and Android ARM64 native build must pass on the final PR head. No original game boot or gameplay is claimed.
