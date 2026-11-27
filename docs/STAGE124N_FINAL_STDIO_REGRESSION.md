# Stage 124N — final stdio regression expansion

## Coverage added
- Multiple `ungetc` calls are consumed in reverse insertion order (last pushed byte first).
- `ftell` reports the logical position including outstanding pushback bytes.
- Reading pushed bytes consumes them before backend data resumes.
- `ungetc` after EOF clears the EOF indicator.
- Consuming the pushed byte does not immediately restore EOF; only a subsequent read that observes backend zero-byte result sets EOF again.

## Regression baseline
This stage builds on Stage 124K `fflush(NULL)` synchronization, Stage 124L backend error propagation, and Stage 124M short-read/EOF semantics.

## Validation and limits
Host source checks/regression and Android ARM64 native build are required on the final branch head. This is provider-level stdio regression coverage, not full ISO C/Bionic conformance. No original game boot or gameplay is claimed.
