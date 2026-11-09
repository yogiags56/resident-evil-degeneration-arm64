# Stage 120Q — RELR Differential Validation and Bionic Metadata Parity

## Objective

Validate the ELF32 RELR expansion independently, correct Android RELR metadata handling against Bionic, and preserve the static audit's read-only scope.

## Reference behavior

Current Android Bionic recognizes both standard and Android RELR address/size/entry-size tags. It treats DT_ANDROID_RELRCOUNT as ignored metadata; it is not a required RELR table member and is not used as the table length or authoritative decoded relocation count.

Primary source:
- https://android.googlesource.com/platform/bionic/+/master/linker/linker.cpp
- https://android.googlesource.com/platform/bionic/+/c10d064b5c44cc7fe461501d66ead5c814110823/linker/linker.cpp

GNU binutils readelf is used as an independent output oracle for a generated ELF32 ARM shared-object fixture containing SHT_RELR.

## Implementation and tests

- Corrected standalone DT_ANDROID_RELRCOUNT handling: the tag is preserved as a non-authoritative hint and does not cause rejection when no RELR table tags exist.
- Added top-level android_relr_count_hint metadata to both dynamic REL/RELA and APS2 audit reports.
- Added a generated ELF32 ARM DYN fixture with a SHT_RELR section.
- Added a differential test that compares decoded targets against GNU readelf -rW output when readelf is installed.
- Existing malformed-input, bounds, mixed-tag, APS2 coexistence, and cross-format collision tests remain enabled.

## Validation criteria

- Parser output and readelf expanded target addresses must match exactly.
- The optional external-tool test skips only if GNU readelf is unavailable.
- Full host test suite, C mock compile checks, native C regression checks, and SHA-256 source manifest must pass.

## Scope limits

This remains offline ELF inspection. It does not apply relocations, compute runtime load bias, inspect target memory, or claim Android device execution or original game boot.

## Validation evidence

GitHub Actions run 42640031158 passed all 273 Python tests in 5.135 seconds, including GNU readelf differential comparison of the generated ELF32 SHT_RELR fixture. Host C mock compilation, native C regression checks, and CI SHA-256 reporting also passed. See STAGE120Q_VALIDATION_LOG.txt.
