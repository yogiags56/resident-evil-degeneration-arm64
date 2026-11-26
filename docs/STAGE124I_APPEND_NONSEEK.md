# Stage 124I — append positioning and non-seekable stream audit

## Changes
- Route `fwrite` and `fprintf` through one stream-write helper.
- For append-mode streams, seek to end-of-file before every write operation, rather than relying only on the initial open position.
- Report missing/failed append seek as a stream error and reject the write.
- Add a regression that deliberately moves the backend cursor to the start after opening in append mode, then verifies the next guest write still lands at EOF.

## Remaining audit
- Non-seekable append streams cannot provide strict append semantics with the current backend contract; the write is rejected and `ferror` is set.
- A future backend capability should expose atomic append/write-at-end, or document that append mode requires seek support.
- Update-stream read/write transition constraints and broader Bionic behavior remain to be covered.

## Validation
Host regression and Android ARM64 native build are required on the final branch head. No original game boot or gameplay is claimed.
