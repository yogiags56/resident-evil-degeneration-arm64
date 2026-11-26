# Stage 124H — stdio seek and logical-position audit

## Objective
Verify that `ftell` reports the logical guest stream position when pushback bytes are pending, and that `fseek(..., SEEK_CUR)` accounts for unread bytes before discarding them.

## Regression additions
- Read four bytes from the stream, push one byte back with `ungetc`, and verify `ftell` reports the logical position one byte earlier.
- Seek by zero relative to the current logical position and verify the next read returns the expected backend byte, not the stale pushback byte.
- Force an invalid backend seek origin, verify `fseek` returns `-1` and sets `ferror`, then verify `clearerr` resets the error indicator.

## Known limitations / audit scope
- This is a focused regression extension, not a complete Bionic stdio conformance suite.
- `fseek`/`ftell` on non-seekable streams must fail consistently; additional tests are needed.
- Read/write update-stream transition rules and append-mode positioning require separate audit.

## Validation
Host regression and Android ARM64 native build are required on the final branch head. No original game boot or gameplay is claimed.
