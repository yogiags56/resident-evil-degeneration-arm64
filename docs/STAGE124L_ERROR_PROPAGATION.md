# Stage 124L — stdio backend error propagation audit

## Scope
Exercise backend failures at the guest stdio boundary and verify that guest-visible return values and stream indicators remain consistent.

## Regression coverage added
- Backend `write` failure: `fwrite` returns zero complete items and sets the stream error indicator.
- `clearerr` clears the write error indicator.
- Explicit backend `flush` failure: `fflush(FILE*)` returns EOF (`-1`) and sets `ferror`.
- `clearerr` clears the flush error indicator.
- Backend `close` failure: `fclose` returns EOF (`-1`) while still invalidating the FILE token and releasing the slot after the close attempt.

## Existing behavior reviewed
- Read backend failures are distinguished from EOF; partial buffered data can still be returned while `ferror` is set.
- Failed seek and failed update-stream transition operations set `ferror`.
- `fflush(NULL)` aggregates failures and marks the affected streams.

## Validation and limits
Host regression and Android ARM64 native build are required on the final branch head. The provider does not expose a guest `errno` symbol yet; this stage audits the existing FILE error indicator and C stdio return-value contract, not full Bionic `errno` integration. No original game boot or gameplay is claimed.
