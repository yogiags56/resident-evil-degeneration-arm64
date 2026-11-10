# Stage 123B — Native XE3U header and fixup metadata parser

## Format boundary
The original guest executable is an LZMA-alone stream that decodes to an XE3U image. It is not ELF. The separate `libjirobiodg.so` is ELF32 ARM. The ELF32 pipeline must not be used as the main `biohazard.s3e` executable loader.

## Implemented
- Added `re_guest_xe3u_image.h/.c`.
- Added bounded, read-only parsing of the 0x48-byte XE3U header, including the extended data_offset field.
- Validates magic, header size, fixup/code/signature/config/extra ranges, code_file_size <= code_memory_size, entry offset, 32-bit base+code memory range, fixup-before-code ordering, and observed extra-section ordering.
- Parses bounded fixup subsection records for:
  - Type 0: API symbol names.
  - Type 1: internal relocation offsets.
  - Types 2, 3, and 4: external relocation records (combined 32-bit offset, symbol index, subsection type).
- Preserves unknown subsection types as bounded metadata records and skips their opaque payload.
- Uses caller-owned symbol, internal-offset, and external-relocation arrays; no heap allocation.
- Performs a validation pass for all subsection boundaries, symbol strings, capacities, and external symbol indices before publishing output arrays.
- Leaves caller arrays and fixup info unchanged on malformed input/capacity failure.
- Added a synthetic XE3U fixture covering header fields, two symbol names, internal relocation offsets, external Thumb fixups, malformed symbol indices, capacity failures, and invalid header bounds.
- Registered parser in Android native CMake and host test suite.

## Explicit non-goals
- No LZMA decompression in this module; it accepts already-decoded XE3U bytes. The existing bounded liblzma-alone decoder is a separate component.
- No internal relocation application, external import binding, code/data/BSS mapping, executable permission assignment, entry execution, or guess about relocation semantics.
- The observed architecture field value 5 remains uninterpreted.
- The meaning and ownership of data_offset/extra fields beyond bounds/order checks remain unassigned until independently verified.

## Provenance and validation
The parser is based on the recorded Stage 97 observation of the user-supplied APK, including XE3U header offsets and fixup subsection counts. Proprietary APK and assets are not present in the repository; synthetic tests do not replace re-running the parser against the user's original decoded image.

Host CI run 42868579008 passed and explicitly reported `Guest XE3U header and fixup parser tests: PASS`. A follow-up header-ordering hardening change is undergoing its own CI run.
