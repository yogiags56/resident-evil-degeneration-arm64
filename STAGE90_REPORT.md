# Stage 90 Report — GitHub-Ready Source & DTRZ Metadata Boundary

## Outcome
Created a source-only repository scaffold suitable for GitHub and implemented a conservative, testable DTRZ profile inspector.

## DTRZ results from the locally supplied APK
- `assets/main.dz`: 14,552,804 bytes.
- SHA-256: `1de13896c0f2b1805d3808e72d9e95a7f181b2805dfc966d4a606d134f77927e`.
- Signature: `DTRZ/`.
- Root filename records: 47.
- Path records: 35, including a final empty sentinel.
- Metadata begins at decimal offset 1,629 (`0x65d`).
- Metadata region size: 14,551,175 bytes.

The table counts are explicit profile inputs for this examined archive. They are not asserted as universal DTRZ constants.

## Implemented
- `tools/dtrz_inspect.py`: bounded NUL-string table parser, offsets, archive hash, metadata preview, 16-bit histogram for triage, JSON output.
- `tests/test_dtrz_inspect.py`: malformed header, unterminated strings, empty root, negative count, and normal table-boundary tests.
- `tools/extract_apk_assets.py`: extracts only the three assets needed by the Android probe into a local ignored directory.
- `tools/run_tests.sh`: one-command Python, host-mock C syntax, and Stage89 contract regression.
- `.github/workflows/ci.yml`: GitHub Actions host-check workflow.
- Root documentation, MIT license for project source, notices, contribution rules, and roadmap.
- Existing Stage89 C/JNI/Android probe source carried forward without shipping original game payloads.
- Guest CPU, import dispatcher, AAPCS32 bridge, runtime coordinator, and the two previously verified GL import adapters consolidated from Stage73 source.

## Validation results
- Consolidated Stage60/71/72/73 runtime regressions: CPU PASS, import dispatcher 11 assertions, runtime coordinator 10 assertions, verified GL adapter 12 assertions.
- APK asset extraction helper: passed for `main.dz`, `app.icf`, and `s3e.icf`; extracted hashes match the Stage83 inventory.
- DTRZ unit tests: 5/5 passed.
- Native C host-mock syntax checks: 5 translation units passed with `-Wall -Wextra -Werror -Wconversion -Wshadow`.
- Stage89 ABI contract regression: 10 assertions passed.
- Stage73 guest CPU/import/runtime/GL regression executables: included in the unified runner and individually executed.
- Actual locally supplied `main.dz` parsed successfully with the observed profile counts.
- Android Gradle/NDK build: not run; SDK/NDK/Gradle are not available in this execution environment.
- Physical-device test: not run.

## Important limits
The metadata region is still opaque. No entry record schema, payload offset, packed length, unpacked length, or codec is claimed. No payload reader or extractor is implemented. The original game has not booted or reached gameplay.

## GitHub publishing
The repository intentionally excludes the APK and extracted proprietary assets. Run `tools/extract_apk_assets.py` locally with a legally usable APK before building the Android probe. The extracted files are ignored by Git.
