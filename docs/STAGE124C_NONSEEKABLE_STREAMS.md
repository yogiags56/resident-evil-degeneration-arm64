# Stage 124C — Non-seekable stdio streams

## Objective

Remove the requirement that every guest stdio backend implement seek/tell.
Archive readers, pipes, and other forward-only sources can now support
`fscanf` without rewinding the underlying source.

## Implementation

- Added a bounded per-open-file unread buffer
  (`RE_GUEST_LIBC_STDIO_BUFFER_CAP`).
- `fscanf` fills that buffer through sequential backend reads, parses the
  buffered bytes, and removes only the number of bytes actually consumed.
- Bytes beyond the parser's stopping point remain available to later
  `fread`, `fgets`, or `fscanf` calls.
- `fread` and `fgets` drain unread buffered bytes before reading the backend.
- Backend `seek` and `tell` callbacks are now optional. `fseek`/`ftell`
  return failure when unavailable.
- `SEEK_CUR` compensates for bytes read ahead into the unread buffer; a
  successful seek clears the buffer and EOF indicator.
- The scanner remains bounded; this is not an unbounded stream snapshot.

## Regression coverage

The stdio provider test now creates a second backend with NULL seek/tell
callbacks. It verifies that `fscanf("%d")` reads the integer prefix from
`123XYZ`, then `fread` receives the preserved `XYZ` bytes. It also verifies
that unsupported `ftell` and `fseek` return -1 without breaking the stream.

## Limitations

- Lookahead is capped at the fixed buffer capacity.
- A backend read error during lookahead leaves the buffered bytes intact and
  sets the stream error indicator.
- Full POSIX pushback (`ungetc`), wide-character streams, and thread-safe
  concurrent access are not part of this stage.
- Read/write direction switching still requires the usual stream positioning
  rules and is not broadened here.

## Validation

Host regression and Android ARM64 native build must pass on the latest commit
before this stage is considered complete.
