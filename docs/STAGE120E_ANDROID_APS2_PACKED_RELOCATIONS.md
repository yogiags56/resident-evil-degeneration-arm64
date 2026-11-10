# Stage 120E — Android APS2 Packed Relocations

## Goal

Decode Android's APS2 packed dynamic relocation streams for ELF32 EM_ARM, using DT_ANDROID_REL/DT_ANDROID_RELSZ and DT_ANDROID_RELA/DT_ANDROID_RELASZ from PT_DYNAMIC. This complements the ordinary REL/RELA and PLT tables decoded by Stage 120D.

## Usage

```bash
python3 tools/guest_arm_aps2_relocation_audit.py /path/to/guest.elf --json /tmp/aps2-relocations.json
```

## Implementation

- Reads APS2 streams beginning with the exact `APS2` magic.
- Decodes signed 32-bit SLEB128 values with a five-byte bound.
- Decodes relocation count, initial offset, group size, group flags, offset deltas, relocation info, and cumulative addend deltas.
- Supports GROUPED_BY_INFO, GROUPED_BY_OFFSET_DELTA, GROUPED_BY_ADDEND, and HAS_ADDEND.
- Supports Android packed REL and RELA dynamic tag pairs.
- Maps packed blobs only through a unique file-backed PT_LOAD range.
- Reuses Stage 120D's bounded SysV DT_HASH or DT_GNU_HASH dynamic symbol correlation.
- Preserves unknown ARM relocation types and never applies relocations.

## Safety invariants

- Rejects malformed magic, truncated/overlong SLEB128, invalid group flags, zero/oversized groups, count mismatch, negative offset deltas, ELF32 offset overflow, addend overflow, oversized streams, missing paired tags, and trailing bytes.
- REL streams may not contain addends.
- GROUPED_BY_ADDEND requires HAS_ADDEND.
- The parser is read-only: it reports relocation targets and symbol identity but does not dereference targets, calculate runtime addresses, patch memory, or infer providers.

## Deliberate limitations

- ELF32 EM_ARM only.
- GNU hash-only symbol resolution derives a bounded count from bucket/chain metadata and validates bloom membership and chain hashes against dynamic symbol names.
- Android RELR, runtime load-bias calculation, relocation application, and dependency-provider resolution remain future work.
- The parser reports APS2 streams independently from ordinary DT_REL/DT_RELA/DT_JMPREL tables; it does not deduplicate overlapping relocation inventories.

## Tests

Synthetic fixtures cover SLEB128 signed boundaries, truncation and overlong encodings, grouped REL and RELA, cumulative addends, malformed group flags/sizes, negative offsets, overflow, tag pairing, file-backed PT_LOAD mapping, and GNU-hash-only resolution through the shared symbol parser.
