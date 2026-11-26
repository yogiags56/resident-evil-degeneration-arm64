# Stage 124E — stdio EOF and backend read-error semantics

## Objective
Distinguish a clean short read/EOF from a backend read failure, including when `file_read` has already copied bytes from the guest pushback/lookahead buffer.

## Changes
- `file_read` now sets the stream error indicator when the backend returns a negative or impossible oversized read result.
- If buffered bytes were already copied before the backend error, `file_read` returns that partial byte count instead of discarding valid data.
- `fread` no longer sets EOF merely because the returned byte count is short when the stream error indicator was raised.
- Regression coverage injects a one-shot backend read failure after a pushed-back byte and verifies: one byte is returned, `feof` remains clear, and `ferror` is set.

## Scope and limitations
- This stage covers read-side EOF/error distinction and partial buffered reads.
- A public `clearerr` import, write-side partial failure policy, and full Bionic stdio parity remain separate tasks.
- No original game boot or gameplay is claimed.

## Validation
Host regression and Android ARM64 native build must pass on the final Stage 124E commit before acceptance.
