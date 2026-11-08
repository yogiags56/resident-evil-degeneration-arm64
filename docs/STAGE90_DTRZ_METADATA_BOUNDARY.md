# Stage 90 — DTRZ Metadata Boundary Inspector

## Objective
Turn the previously observed DTRZ name/path table boundary into a repeatable, source-distributed inspection tool without inventing a payload schema.

## Evidence carried forward
For the APK examined in this project:
- `assets/main.dz` size: 14,552,804 bytes.
- SHA-256: `1de13896c0f2b1805d3808e72d9e95a7f181b2805dfc966d4a606d134f77927e`.
- Signature: `DTRZ/`.
- The observed profile has 47 root filename records beginning at offset 9.
- The observed profile has 35 path records beginning after the root filename table.
- The path table's final record is an empty sentinel.
- The metadata region begins at offset 1,629 (`0x65d`) and occupies the remainder of the archive.

## What Stage 90 implements
`tools/dtrz_inspect.py`:
- validates the observed signature;
- reads a caller-specified number of NUL-terminated root names and path records;
- records each string's original byte offset;
- identifies the metadata boundary;
- emits a bounded metadata hex preview and a 16-bit value histogram for triage;
- writes a machine-readable JSON report.

## What it does not implement
- It does not decode metadata records.
- It does not map any of the 47 filenames to payload offsets.
- It does not determine packed/unpacked sizes.
- It does not identify compression codecs.
- It does not extract any payload.
- It does not claim that the 47 root filenames correspond one-to-one with records in the opaque metadata region.

The count values 47 and 35 are profile parameters based on the examined archive, not a universal DTRZ format specification.

## Stage 90 decision
Keep the metadata region opaque. Stage 91 should correlate the archive bytes with parser routines in the original guest executable and test candidate record layouts against multiple independent files before implementing any payload reader.


## Superseded by Stage 91

The initial Stage 90 inspection counted the final root-folder placeholder as an extra empty folder string and therefore reported the following boundary one byte late. Stage 91 corrected this using the DTRZ header semantics: `folder_count` includes the root folder, so only `folder_count - 1` non-root folder strings are read. The attribute table begins at `0x65C`; see `../STAGE91_REPORT.md`.
