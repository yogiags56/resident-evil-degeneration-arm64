# Stage 98 — Guest File Bridge Contract Implementation

## Result

Stage 98 adds a backend-backed, host-testable guest file bridge over the existing `REFileAdapter`. The bridge connects AAPCS32 argument words to the existing guest-memory-safe file adapter and DTRZ/AssetManager backend contract.

The bridge implements host-side operations for:
- `s3eFileOpen(path, mode)` — validates guest strings and supports the observed read-only `rb` mode; returns an opaque 32-bit generation/slot handle on success.
- `s3eFileRead(buffer, itemSize, itemCount, file)` — reads the requested byte span while returning **complete item count** in the guest result, not byte count.
- `s3eFileGetChar(file)` — returns bytes as 0–255 and `-1` at EOF.
- `s3eFileClose(file)` — releases the stream and invalidates the handle.

The bridge is intentionally a host-side operation layer. It is not yet bound to production game symbol indices and does not authorize production guest imports.

## Important contract boundaries

### Read return value

The Stage 97 original-binary analysis established that `s3eFileRead` returns an item count. Stage 98 implements:

`items_read = floor(bytes_transferred / item_size)`

The byte adapter can transfer a trailing partial item, while the guest return counts only complete items. This matches conventional `fread` item-count behavior, but the original Marmalade runtime's exact partial-item side effects still require version-matched documentation or controlled traces. It is therefore a compatibility implementation under test, not a claim that every partial-read edge case is fully proven.

The bridge rejects zero item size/count, null destination/handle, invalid guest-memory spans, and stale/bad handles before calling the backend. The byte adapter's current 32-bit span limit is preserved; very large products that cannot be represented by a guest memory span fail through a host-side error status.

### Error mapping

The bridge keeps host operation status separate from guest result registers:
- successful Open returns an opaque nonzero handle;
- ordinary Open failure returns a null handle in the import-handler wrapper, consistent with original guest callers checking Open against null;
- successful Read returns complete item count;
- GetChar returns `-1` at EOF;
- Close returns zero in the unused result register.

Read/backend errors and invalid handles are returned as host dispatcher errors by the wrapper. No undocumented Marmalade error sentinel is invented. This intentionally prevents uncertain error semantics from being mistaken for a proven guest return value.

## Handle lifecycle

The existing adapter uses generation+slot tokens, not host pointers. Stage 98 tests:
- successful open and read;
- close and stale-handle rejection;
- repeated close rejection;
- reopening produces a distinct generation token;
- invalid token rejection;
- cleanup through `re_file_adapter_close_all`;
- backend stream accounting.

The adapter remains single-threaded/not internally synchronized; callers must serialize access until a threading model is explicitly designed and validated.

## Files added/changed

- `app/src/main/cpp/re_s3e_file_bridge.h`
- `app/src/main/cpp/re_s3e_file_bridge.c`
- `tests/test_re_s3e_file_bridge.c`
- `app/src/main/cpp/CMakeLists.txt`
- `tools/compile_host_mock.sh`
- `tools/run_tests.sh`

The new host test has 48 assertions. The full host suite passes: 22 Python tests, host mock syntax checks, and all native C test binaries including Stages 94–98.

## Activation decision

**Production guest imports remain disabled.** The new handler wrappers can be registered in a test dispatcher, but Stage 98 deliberately does not bind the original executable's production symbol table. Exact `s3eFileRead` partial/error semantics, full SDK-version compatibility, and Android NDK/physical-device validation remain open. The bridge wrappers currently return dispatcher errors for backend read failures rather than inventing a guest API error value.

## Next stage

Stage 99 should:
1. Recover the original SDK/runtime error and partial-read behavior or collect controlled traces.
2. Resolve the actual original symbol indices for the four imports from the parsed XE3U symbol table and compare them with all relocation cells.
3. Add a synthetic AAPCS32 dispatcher test that calls the real Stage 98 bridge handlers end-to-end.
4. Add a DTRZ-backed integration test that reads a real archive entry through guest API calls and verifies returned item counts, bytes, EOF, and close lifecycle.
5. Keep production import activation disabled until the remaining error/partial-read contract is closed.
