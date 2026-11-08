# Stage 98 Report — Guest File Bridge Contract Implementation

Stage 98 adds a backend-backed host-side bridge for the original guest file APIs over the existing generic file adapter.

## Implemented

- `s3eFileOpen`: guest path/mode pointer capture, bounds-checked strings, `rb` mode validation, normalized path handling, and opaque generation/slot handles.
- `s3eFileRead`: checked `item_size * item_count`, guest destination bounds, backend byte transfer, and return value expressed as complete items rather than bytes.
- `s3eFileGetChar`: byte values 0–255 and `-1` EOF.
- `s3eFileClose`: close and stale-handle invalidation.
- Host-side handler wrappers that keep dispatcher errors separate from guest result registers.
- Open failure maps to a null handle, consistent with original guest callsites that compare the return with null.
- Production authorization gate remains false.

## Validation

- New Stage 98 bridge test: 48 assertions PASS.
- Python tests: 22 PASS.
- Host mock C syntax checks: PASS.
- DTRZ file backend/VFS/LZMA tests: PASS.
- Guest CPU/import/runtime/GL tests: PASS.
- Stage 95/96 audit and synthetic dispatcher tests: PASS.
- Full `bash tools/run_tests.sh`: PASS.

## Limits

The item-count return is established by Stage 97. The implementation uses conventional complete-item counting and transfers a trailing partial item if the backend returns it, but exact original Marmalade partial-item/error side effects are not yet fully proven. Backend read errors are not translated into a guessed guest error value. Production import registration, Android NDK build, physical-device validation, original game boot, and gameplay remain unclaimed.

See `docs/STAGE98_GUEST_FILE_BRIDGE_IMPLEMENTATION.md`.
