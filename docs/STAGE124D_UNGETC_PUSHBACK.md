# Stage 124D — Guest stdio pushback with ungetc

## Goal
Support the original guest libc's one-character pushback operation on seekable and forward-only stream backends without requiring backend seek/tell callbacks.

## Implemented
- Added the relocation-backed guest `ungetc` import to the stdio provider export table.
- Reuses the per-file bounded unread buffer; pushback prepends one byte so it is consumed before previously buffered lookahead.
- Successful pushback clears the guest EOF indicator and returns the pushed byte as an unsigned-char value.
- Passing EOF, an invalid/non-readable stream, or a full unread buffer fails without changing stream state.
- Added guest-dispatch regression coverage for EOF observation, pushback, EOF clearing, and reading the pushed byte back from a non-seekable backend.

## Deliberate limits
- This stage implements one-character `ungetc` semantics, not a general-purpose multi-byte pushback queue.
- It does not claim complete Bionic stdio compatibility, production loader installation, or original game boot.
- Backend read errors and short-read/EOF distinctions remain separate follow-up work.

## Validation
Host regression and Android ARM64 build/ELF verification are required before marking this stage complete.
