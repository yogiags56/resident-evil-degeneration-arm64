# Stage 123C — Original APK revalidation

## Result

The user re-uploaded `Resident Evil Degeneration by JawaDroidNew.apk`. The APK and all relevant binary assets were independently re-opened and hashed. Every recorded Stage 97 SHA-256 and size matches exactly. This confirms that the re-upload is the same original APK used during Stage 97.

## Exact provenance match

| Asset | Size | SHA-256 |
|---|---:|---|
| APK | 20,001,777 | `6654b4af083a899b5c75de92a66b485435b3ce6ac65afbe96082d5ede4eb23d2` |
| `assets/biohazard.s3e` compressed | 673,661 | `3b673d9e88071ad6a83ff892eef394447cf4cb81e540a11ae31f9265a181c271` |
| `assets/biohazard.s3e` decoded | 1,660,600 | `66b4987fbc419be8b4c91e0d0ee1f57fd8cb40771e92ee7c38658b799fb5e5fd` |
| XE3U code+initialized-data blob | 1,509,476 | `5fc9a53a90895eff456181357806f27b7566e7841df87328b6459c1715e79006` |
| `assets/main.dz` | 14,552,804 | `1de13896c0f2b1805d3808e72d9e95a7f181b2805dfc966d4a606d134f77927e` |
| `assets/s3e.icf` | 18,490 | `1bad3703e805b557faa2bec7cde91dd0e82e68fb92294b9fbc112592d1f8aa9e` |
| `lib/armeabi/libjirobiodg.so` | 431,528 | `ce6c9f6f7339f226c1f060744ddb89d7b187d01a8b20571cd417896bf4c4fcf0` |

All values match the original Stage 97 evidence exactly.

## Revalidated XE3U header

- Magic: `XE3U`
- Version: `0x00041901`
- Flags: `0x0002`
- Architecture field: `5` (still intentionally uninterpreted)
- Fixup range: `0x48 + 0x24CC4`
- Code blob: `0x24D0C + 0x170864`
- Code memory size: `0x18B430`
- Entry offset: `0`
- Base address: `0x4A000000`
- Extra range: `0x195570 + 0x148`
- Extended header size: `8`
- Data offset: `0x16C018`

The fixup range ends exactly at code_offset. The code file range ends exactly at extra_offset.

## Revalidated fixup table

| Type | Meaning | Size | Count |
|---:|---|---:|---:|
| 0 | API symbols | `0xEA8` | 227 |
| 1 | Internal relocation offsets | `0x23870` | 36,377 |
| 2 | ARM external relocation | `0x0C` | 0 |
| 4 | Thumb external relocation | `0x5A0` | 238 |

All 238 external relocation records reference valid symbol indices. The four file API relocation cells match Stage 97:
- `s3eFileClose`: `0x13B71C`
- `s3eFileRead`: `0x13B7E0`
- `s3eFileOpen`: `0x13BA50`
- `s3eFileGetChar`: `0x13BA6C`

## Decision

The original APK provenance is now closed: the uploaded APK is byte-for-byte identical to the Stage 97 input, and its compressed and decoded XE3U images, code blob, fixup counts, and API relocation cells all reproduce the recorded evidence.

No game bytes were modified. No relocations were applied. No guest code was executed.

## Next: Stage 123D — reconstruct relocation semantics

Proceed with the exact decoded image and code blob:
1. Recover the internal relocation encoding from the original guest loader/toolchain evidence and validate candidate semantics against all 36,377 offsets.
2. Determine which offsets refer to code, initialized data, or other address domains using the observed data_offset and code memory layout.
3. Disassemble and classify external Thumb fixup cells and their associated API thunk patterns.
4. Build a read-only relocation report and invariants first; only then implement a transactional relocation planner/applier.
5. Keep the separate ELF32 `libjirobiodg.so` path distinct from the XE3U main executable.

The proprietary APK and extracted assets remain excluded from Git.
