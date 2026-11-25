# Stage 124D — Guest stdio pushback with ungetc

## Goal
Implement guest `ungetc` through the existing bounded unread buffer, including on forward-only stream backends.

## Implemented
- Registered `ungetc` as the fourteenth relocation-backed stdio import.
- Pushes an unsigned byte to the front of the per-file unread buffer, ahead of any buffered lookahead.
- Successful pushback clears the guest EOF indicator and returns the pushed byte as an unsigned-char value.
- Passing EOF returns EOF without modifying the stream; a full buffer also returns EOF.
- The unread buffer permits bounded repeated pushback, rather than relying on backend seek/tell support.
- Regression coverage exercises EOF observation, pushback, EOF clearing, reading the pushed byte, and EOF argument rejection.

## Deliberate limits
- This is a bounded guest stdio compatibility implementation, not a claim of complete Bionic stdio parity.
- Backend read-error semantics and full production loader activation remain separate work.
- No original game boot or gameplay is claimed.

## Validation
The initial host CI run failed because the new import name and enum value were not registered in the provider source. Stage 124D corrects the enum/export registration and expands regression coverage. Host regression and Android ARM64 build/ELF verification must pass on the final PR head before this stage is accepted.
