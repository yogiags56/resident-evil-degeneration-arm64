# Stage 91 — DTRZ/Derbh Structure Reconstruction & Payload Verification

## Outcome

The `main.dz` archive supplied locally with the examined APK has now been structurally parsed and all 47 top-level entries have been decompressed and size-verified. This closes the Stage 90 metadata-boundary uncertainty for this specific archive profile.

## Corrected table boundaries

The archive uses the standard 9-byte DTRZ header:

- `0x00..0x03`: `DTRZ`
- `0x04..0x05`: file count (`47`, little-endian)
- `0x06..0x07`: folder count including root (`35`, little-endian)
- `0x08`: root placeholder (`0`)

The folder table contains `folder_count - 1` non-root directory strings (34). The zero byte at `0x65C` is the first byte of the attribute table, not an additional empty folder string. This corrects the Stage 90 inspector's one-byte-late metadata boundary.

| Region | Offset | Count / size | Interpretation |
|---|---:|---:|---|
| Header | `0x0000` | 9 bytes | Confirmed DTRZ header |
| Filename strings | `0x0009` | 47 NUL strings | Confirmed |
| Folder strings | after filenames | 34 NUL strings | Root is represented by header placeholder |
| Attribute table | `0x065C` | 47 × 6 bytes | `<u16 folder_index, u16 file_index, u16 flags>` |
| Location header | `0x0776` | 4 bytes | Unknown u16 = 1; location count = 47 |
| Location records | `0x077A` | 47 × 16 bytes | `<u32 offset, u32 size_a, u32 size_b, u32 method>` |
| Payload data | `0x0A6A` | through EOF | Absolute archive offsets |

## Attribute table findings

- All 47 file-index fields match their corresponding filename index.
- Folder indices are in range `0..34`.
- Folder index `0` maps to the archive root; indices `1..34` map to the 34 folder strings.
- All 47 observed attribute flags are `0xFFFF`; their semantics remain unknown.

## Location and compression findings

- The first payload offset is `0x0A6A`, exactly the end of the location table.
- All 47 payload offsets are strictly increasing and lie within the archive.
- Each payload window ends at the next entry's offset; the final window ends at archive EOF.
- All 47 records use method `0x200` (LZMA-alone for this observed profile).
- Both recorded size fields are identical for every entry and equal the decompressed output length.
- The compressed window length is derived from adjacent offsets; it is not either of the duplicated size fields.

## Full-archive validation

- Archive size: 14,552,804 bytes.
- Archive SHA-256: `1de13896c0f2b1805d3808e72d9e95a7f181b2805dfc966d4a606d134f77927e`.
- Payload region: 14,550,138 compressed bytes.
- Entries decompressed: 47 / 47.
- Total decoded bytes: 57,976,257.
- Decompression failures: 0.
- Decoded-size mismatches: 0.
- Method distribution: 47 × `0x200`.

## Entry inventory

| # | Resolved path | Payload offset | Compressed window | Decoded size | Method |
|---:|---|---:|---:|---:|---:|
| 00 | `console.bin` | `0x000A6A` | 6,098 | 18,200 | `0x200` |
| 01 | `localisation/english.dat` | `0x00223C` | 16,902 | 48,097 | `0x200` |
| 02 | `localisation/french.dat` | `0x006442` | 17,196 | 52,278 | `0x200` |
| 03 | `localisation/german.dat` | `0x00A76E` | 18,114 | 52,936 | `0x200` |
| 04 | `localisation/italian.dat` | `0x00EE30` | 17,565 | 52,047 | `0x200` |
| 05 | `localisation/japanese.dat` | `0x0132CD` | 16,841 | 62,009 | `0x200` |
| 06 | `localisation/spanish.dat` | `0x017496` | 16,733 | 49,462 | `0x200` |
| 07 | `localisation/LocaliseHashes.dat` | `0x01B5F3` | 4,499 | 6,918 | `0x200` |
| 08 | `common/loading_wav.group.bin` | `0x01C786` | 116,072 | 493,533 | `0x200` |
| 09 | `common/fixed_wav.group.bin` | `0x038CEE` | 4,379,008 | 11,603,956 | `0x200` |
| 10 | `common/splash.group.bin` | `0x465E6E` | 22,772 | 295,555 | `0x200` |
| 11 | `common/frontend_wav.group.bin` | `0x46B762` | 52,944 | 181,456 | `0x200` |
| 12 | `common/characters/characters_main.group.bin` | `0x478632` | 418,475 | 748,845 | `0x200` |
| 13 | `common/designObjects/designObjects.group.bin` | `0x4DE8DD` | 441 | 864 | `0x200` |
| 14 | `common/mercenarymode.group.bin` | `0x4DEA96` | 1,952 | 4,280 | `0x200` |
| 15 | `common/storymode.group.bin` | `0x4DF236` | 4,528 | 10,639 | `0x200` |
| 16 | `levels/baggage/baggage.group.bin` | `0x4E03E6` | 496,759 | 3,357,612 | `0x200` |
| 17 | `levels/baggage/baggage_mercenary.group.bin` | `0x55985D` | 2,109 | 7,585 | `0x200` |
| 18 | `levels/baggage/baggage_story.group.bin` | `0x55A09A` | 4,148 | 16,826 | `0x200` |
| 19 | `levels/dutyfreeeast/dutyfreeeast.group.bin` | `0x55B0CE` | 1,086,204 | 3,315,942 | `0x200` |
| 20 | `levels/dutyfreeeast/dutyfreeeast_mercenary.group.bin` | `0x6643CA` | 2,051 | 8,388 | `0x200` |
| 21 | `levels/dutyfreeeast/dutyfreeeast_story.group.bin` | `0x664BCD` | 3,914 | 12,449 | `0x200` |
| 22 | `levels/dutyfreewest/dutyfreewest.group.bin` | `0x665B17` | 901,763 | 3,243,436 | `0x200` |
| 23 | `levels/dutyfreewest/dutyfreewest_story.group.bin` | `0x741D9A` | 2,199 | 6,850 | `0x200` |
| 24 | `levels/generatortunnels/generatortunnels.group.bin` | `0x742631` | 544,610 | 3,308,117 | `0x200` |
| 25 | `levels/generatortunnels/generatortunnels_story.group.bin` | `0x7C7593` | 2,209 | 6,457 | `0x200` |
| 26 | `levels/infirmary/infirmary.group.bin` | `0x7C7E34` | 678,128 | 3,360,172 | `0x200` |
| 27 | `levels/infirmary/infirmary_story.group.bin` | `0x86D724` | 5,520 | 22,278 | `0x200` |
| 28 | `levels/planecargo/planecargo.group.bin` | `0x86ECB4` | 125,486 | 837,318 | `0x200` |
| 29 | `levels/planecargo/planecargo_story.group.bin` | `0x88D6E2` | 1,917 | 4,909 | `0x200` |
| 30 | `levels/planecrashed/planecrashed.group.bin` | `0x88DE5F` | 1,062,875 | 3,287,207 | `0x200` |
| 31 | `levels/planecrashed/planecrashed_story.group.bin` | `0x99163A` | 1,309 | 3,830 | `0x200` |
| 32 | `levels/planeentrance/planeentrance.group.bin` | `0x991B57` | 433,303 | 3,187,818 | `0x200` |
| 33 | `levels/planeentrance/planeentrance_story.group.bin` | `0x9FB7EE` | 1,643 | 4,520 | `0x200` |
| 34 | `levels/planeexit/planeexit.group.bin` | `0x9FBE59` | 533,330 | 3,259,069 | `0x200` |
| 35 | `levels/planeexit/planeexit_story.group.bin` | `0xA7E1AB` | 3,610 | 11,108 | `0x200` |
| 36 | `levels/security/security.group.bin` | `0xA7EFC5` | 948,037 | 3,348,489 | `0x200` |
| 37 | `levels/security/security_story.group.bin` | `0xB6670A` | 4,140 | 14,054 | `0x200` |
| 38 | `levels/staffarea/staffarea.group.bin` | `0xB67736` | 407,211 | 3,271,241 | `0x200` |
| 39 | `levels/staffarea/staffarea_story.group.bin` | `0xBCADE1` | 2,785 | 8,603 | `0x200` |
| 40 | `levels/vip/vip.group.bin` | `0xBCB8C2` | 746,950 | 3,362,386 | `0x200` |
| 41 | `levels/vip/vip_story.group.bin` | `0xC81E88` | 5,715 | 20,334 | `0x200` |
| 42 | `levels/vipplane/vipplane.group.bin` | `0xC834DB` | 932,761 | 3,215,661 | `0x200` |
| 43 | `levels/vipplane/vipplane_story.group.bin` | `0xD67074` | 1,743 | 5,139 | `0x200` |
| 44 | `levels/warehouse/warehouse.group.bin` | `0xD67743` | 258,249 | 3,239,131 | `0x200` |
| 45 | `levels/warehouse/warehouse_story.group.bin` | `0xDA680C` | 3,144 | 8,651 | `0x200` |
| 46 | `common/environment.group.bin` | `0xDA7454` | 236,176 | 539,602 | `0x200` |

## Implementation

- `tools/dtrz_extract.py` parses the archive tables, validates indices and bounds, derives payload windows, verifies decompression, and optionally extracts files to a user-selected directory.
- `tests/test_dtrz_extract.py` contains 12 synthetic tests, including malformed headers, bad indices, invalid offsets, path traversal, unsupported methods, and output-size mismatch.
- The tool is source-only in this repository. No APK, `main.dz`, extracted group, localization, audio, or other game asset is distributed.

## Remaining unknowns and limits

- The meaning of the location-header first u16 (`1`) is not established.
- The meaning of attribute flags `0xFFFF` is not established.
- The semantic distinction between `size_a` and `size_b` is not established; for this archive they are identical and match decoded size.
- Only method `0x200` is exercised by this archive. Other known DTRZ methods are not claimed as implemented by this project tool.
- This is an offline archive parser/extractor, not yet the Android runtime's DTRZ virtual filesystem.
- `s3eFileRead` guest return semantics remain unresolved and quarantined.
- The original game has not booted or reached gameplay.

## Reproducible commands

```bash
python3 tools/dtrz_extract.py /path/to/main.dz --list
python3 tools/dtrz_extract.py /path/to/main.dz --verify
python3 tools/dtrz_extract.py /path/to/main.dz --verify --json dtrz-verify.json
python3 tools/dtrz_extract.py /path/to/main.dz --extract /path/outside/repository
./tools/run_tests.sh
```
