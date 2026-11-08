# Stage 97 Report — Original S3E Binary and File API Evidence

Stage 97 completed a fresh static audit of the user-supplied original APK and its actual `assets/biohazard.s3e` guest executable.

## Completed

- Extracted `biohazard.s3e`, `main.dz`, `s3e.icf`, and the ARM32 `libjirobiodg.so` from the supplied APK for offline inspection.
- Decompressed the LZMA-alone S3E stream and verified the `XE3U` magic and 1,660,600-byte decoded image.
- Parsed the XE3U header and validated fixup/code boundaries.
- Parsed the fixup table: 227 API symbols, 36,377 internal relocations, 238 external relocations (0 ARM and 238 Thumb).
- Confirmed exact external relocation cells for `s3eFileOpen`, `s3eFileRead`, `s3eFileGetChar`, and `s3eFileClose`.
- Disassembled the code blob and found 35 direct `s3eFileRead`, 11 `s3eFileOpen`, 5 `s3eFileGetChar`, and 13 `s3eFileClose` thunk references.
- Proved from original callers that `s3eFileRead` returns an item/element count, not a byte count. A caller at `0xB0428` requests one element with a variable `item_size` and compares `r0` with `1`.
- Confirmed the guest's `s3eFileGetChar` helper treats `-1` as EOF/error and the shared close helper skips null handles and clears the stored handle after close.
- Added a read-only S3E/APK audit utility and five parser tests.

## Validation

- `tools/s3e_binary_audit.py` completed against the supplied APK.
- Five new S3E parser tests pass.
- Full host test suite passes.
- The original APK and decoded game binary are not copied into the repository.

## Remaining limits

The exact partial-read, partial-item, and error semantics of `s3eFileRead` are not fully established. The close return is ignored by guest callers. No production guest file imports were activated. No Android NDK build, physical-device runtime validation, original game boot, or gameplay is claimed.

See `docs/STAGE97_ORIGINAL_BINARY_AND_FILE_API_EVIDENCE.md` for hashes, header/fixup details, callsite inventory, and the evidence chain.
