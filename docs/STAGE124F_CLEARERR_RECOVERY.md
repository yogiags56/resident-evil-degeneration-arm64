# Stage 124F — Guest clearerr and stdio error recovery

## Changes
- Add the fifteenth relocation-backed stdio import, `clearerr`.
- Clear both EOF and error indicators for a valid guest stream.
- Return partial/zero byte counts from `file_read` after backend read errors while preserving `ferror`.
- Keep `fread` from turning a backend error into an import-dispatch failure or EOF.
- Keep `fgets` from setting EOF when a zero-byte read was caused by a backend error.
- Extend regression coverage for buffered and unbuffered backend failures, indicator queries, and `clearerr` recovery.

## Validation
Host regression and Android ARM64 native build must pass on the final branch head before acceptance. This stage does not claim full Bionic stdio parity or original game boot.
