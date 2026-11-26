# Stage 124J — update-stream transition rules

## Scope
Track whether a stream was opened in update mode (`+`) and remember the last I/O direction. Apply transition checks consistently to `fread`, `fgets`, `fscanf`, `fwrite`, and `fprintf` through shared preparation helpers.

## Implemented behavior
- Input following output invokes the optional backend flush callback; a flush failure sets the stream error indicator and rejects the read.
- Output following input requires a positioning operation unless the input operation reached EOF. The provider seeks relative to the current position while accounting for pending pushback bytes, then clears that buffer.
- A successful explicit `fseek` resets the I/O direction and EOF/pushback state.
- Non-update streams do not incur update-transition checks.

## Backend contract / limitations
- A backend without `flush` is treated as unbuffered for output-to-input transition purposes.
- Read-to-write transitions require `seek` unless EOF has been reached; non-seekable update streams cannot support that transition yet.
- This is a focused compatibility layer, not a complete ISO C or Android Bionic stdio certification. More tests are needed for `fflush(NULL)`, append-update edge cases, and failure injection.

## Validation
Host regression and Android ARM64 native build must pass on the final branch head. No original game boot or gameplay is claimed.
