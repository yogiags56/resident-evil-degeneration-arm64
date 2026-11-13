# Stage 124D — Guest ungetc pushback

## Objective

Provide the original libc `ungetc` import for seekable and forward-only stdio backends without requiring the backend itself to rewind.

## Implementation

- Added `ungetc` as a guest-callable `libc.so` export bound by its exact undefined ELF32 dynamic-symbol name/index.
- Pushes one byte to the front of the per-file bounded unread buffer.
- Returns the pushed unsigned byte on success and EOF (-1) on failure.
- Rejects EOF input, invalid/stale handles, non-readable streams, and a full pushback buffer.
- A successful push clears the stream EOF indicator.
- `fread`, `fgets`, and `fscanf` consume pushed bytes through the shared unread-buffer reader.

## Regression coverage

The guest-dispatch regression checks successful pushback, byte ordering (`Q` before the next backend byte), and rejection of EOF input through the generated Thumb import trampoline.

## Scope and limitations

This is a bounded pushback implementation, not full host-libc conformance. Multiple pushes are supported while capacity remains. Stream update-mode synchronization, wide-character I/O, concurrent access, production loader installation, and original-game boot remain outside this milestone.

## Validation

Host regression and Android ARM64 build/ELF verification are required on the final stage commit. No original-game boot or physical-device execution is claimed.
