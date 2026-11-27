# Stage 124M — EOF and short-read semantics

## Finding
The stdio provider previously inferred EOF from any positive short backend read (`got < requested`). That is not valid for non-seekable streams and other backends that may return partial data without reaching end-of-file.

## Change
- `file_read` now sets the EOF indicator only when the backend explicitly returns zero bytes.
- A positive short read returns the bytes received and leaves EOF clear.
- A later zero-byte read sets EOF.
- Backend read errors continue to set the error indicator and do not set EOF.

## Regression coverage
A deterministic one-shot short-read limit makes the fake backend return one byte from a three-byte request. The test verifies the first read leaves EOF clear, the remaining bytes can be read, and only a subsequent zero-byte read sets EOF.

## Scope and limitations
This stage does not yet add a separate per-stream pushback slot or implement all ISO C `ungetc` details. Existing pushback is represented in the provider's buffered-byte array. Host regression and Android ARM64 build are required before acceptance. No original game boot or gameplay is claimed.
