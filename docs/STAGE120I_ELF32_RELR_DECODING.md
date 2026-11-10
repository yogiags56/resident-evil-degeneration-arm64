# Stage 120I — ELF32 RELR Relative Relocation Decoding

## Goal

Decode the compact ELF32 RELR relative relocation table from PT_DYNAMIC without applying relocations or reading relocation target memory.

## Dynamic tags

- DT_RELRSZ (35): total byte size of the RELR table.
- DT_RELR (36): virtual address of the RELR table.
- DT_RELRENT (37): size of each encoded entry; 4 bytes for ELF32.

## Encoding

Each encoded entry is one 32-bit word:
- An even word is a direct relocation target offset. The next bitmap cursor becomes direct_offset + 4.
- An odd word is a bitmap. Bits 1 through 31 describe the next 31 word-sized locations relative to the current cursor. After processing the bitmap, the cursor advances by 31 * 4 bytes.
- A later direct word resets the cursor to that direct offset plus one word.

The decoder emits one record per target and retains whether the target came from a direct entry or a particular bitmap bit.

## Implementation

- Parses the three RELR dynamic tags as an all-or-nothing set.
- Requires 4-byte table address alignment and DT_RELRENT=4.
- Maps the complete table through a unique PT_LOAD file-backed range.
- Validates table size, entry count, direct target alignment, bitmap ordering, nonempty bitmap payload, cursor arithmetic, and ELF32 address overflow.
- Integrates decoded targets into Stage 120F relocation reconciliation.
- RELR records deliberately have no symbol index and no addend; they are marked as relative-only records.

## Usage

    python3 tools/guest_arm_relr_audit.py /path/to/guest.elf --json /tmp/relr.json
    python3 tools/guest_arm_relocation_reconciliation.py /path/to/guest.elf --json /tmp/relocation-reconciliation.json

## Validation

GitHub Actions run 42600655563 passed:
- 177 Python tests.
- Direct + bitmap + subsequent direct entry decoding.
- Big-endian ELF32 decoding.
- Missing tag, incorrect entry size, misaligned table/direct offsets, bitmap-before-direct, empty bitmap, cursor overflow, out-of-file-backed-range, and malformed-size rejection.
- RELR integration in unified reconciliation with no fabricated symbol index or addend.
- Host C mock compilation and native regression executables.

## Upstream references

- [glibc ELF definitions](https://github.com/bminor/glibc/blob/master/elf/elf.h)
- [Android Bionic ELF definitions](https://github.com/aosp-mirror/platform_bionic/blob/master/libc/include/elf.h)
- [Android Bionic relocation path](https://github.com/aosp-mirror/platform_bionic/blob/master/linker/linker_relocate.cpp)

## Deliberate limitations

- ELF32 EM_ARM only.
- No relocation target is dereferenced, patched, or applied.
- No implicit addend or runtime load bias is inferred.
- RELR is treated as relative-only; it is not converted into a symbol-based REL/RELA record.
- No provider lookup, Android device execution, or original game boot is claimed.
