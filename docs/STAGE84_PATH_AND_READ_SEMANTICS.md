# Stage 84 — Original Path Strings and File-Read Semantics Audit

## Scope
This stage reopens the original APK and its compressed `biohazard.s3e` guest image, then checks path-related string constants and the already recovered file API callsites. It does not enable any guest import handler.

## 1. Path strings recovered from the original guest image
The original `assets/biohazard.s3e` stream is LZMA-alone compressed. Its decompressed XE3U image is 1,660,600 bytes. The following NUL-terminated strings are present in that real image:

| String | Offset in decompressed image | Evidence / interpretation |
|---|---:|---|
| `.group.bin` | `0x178194` | Group archive filename suffix; a suffix alone does not prove the full runtime path. |
| `common/environment` | `0x1781B0` | Forward-slash resource path string. |
| `main.dz` | `0x17822F` | Matches the real APK asset name `assets/main.dz`. |
| `localisation/%s.dat` | `0x17E204` | Format string for locale data filenames. |
| `localisation/LocaliseHashes.dat` | `0x17E218` | Explicit localization resource path. |

These are static string observations. This stage does not claim that every string was passed directly to `s3eFileOpen`; complete xref/call-argument proof is still outstanding. The safe, evidence-based mapping is that `main.dz` is a root-level APK asset and the archive contains its own internal directory/path records. Do not prepend `assets/` to a path passed to an Android `AAssetManager` rooted at the APK assets directory.

## 2. Original DTRZ index structure
The original `assets/main.dz` is 14,552,804 bytes and begins with `DTRZ/`. Re-parsing the real APK bytes confirms:
- 47 NUL-terminated root filename records beginning at byte offset 9;
- 35 path-table records beginning at byte offset 1036 (the final record is an empty sentinel in this parse);
- the following metadata/index bytes begin at offset 1629.

The first 47 names include localization data and `.group.bin` archives for loading, frontend, characters, environments and story/mercenary areas. Folder records use backslashes for nested paths (for example `levels\\baggage`), while the guest image also contains forward-slash resource strings. This mixed separator evidence is why path normalization must remain explicit and tested rather than guessed.

The metadata area after the path strings has not yet been fully decoded into a verified per-file payload table. Stage 84 deliberately records its boundary and does not claim all 47 payloads were individually decompressed or opened.

## 3. What the original guest proves about `s3eFileRead`
The recovered callsites pass `(destination, item_size, item_count, handle)` in `r0-r3`. Examples include `0x13A00E`, `0x13A04C`, `0x13A05E`, `0x13A06A`, and `0x13A07A`. In the examined metadata-reader path, the result in `r0` is not consumed after these calls.

Consequently, the original callsites do **not** establish whether `s3eFileRead` returns bytes, complete items, a status, or another value. They also do not establish its short-read or error convention. The adapter's internal `read_bytes` result (bytes transferred) must not be copied into guest `r0` as an assumed API contract.

A separate parser path uses `s3eFileGetChar`: helper `0x139F8C` reads a remaining-byte counter at descriptor `+8`, decrements it, obtains the handle from `+4`, calls `s3eFileGetChar`, and treats `-1` as EOF/error. This is evidence for that byte-reader helper only, not a definition of `s3eFileRead`.

## 4. Stage 84 decision
- Guest path constants: recovered and checked against the real decompressed guest image.
- Root APK asset names and DTRZ path-table names: inventoried from the real APK.
- `s3eFileRead` argument order: supported by the existing callsite trace.
- `s3eFileRead` return/short-read semantics: **still unknown**.
- `s3eFileGetChar` EOF sentinel: `-1` is supported by the inspected helper.
- New guest file handlers activated: **0**.
- Android `AAssetManager` implementation: **not yet implemented**.

## 5. Next engineering step
Stage 85 should implement the Android `AAssetManager` backend behind the existing injected adapter contract, using asset-root-relative names, explicit path alias rules, and tested lifetime/close behavior. Keep the `s3eFileRead` guest return register quarantined until a stronger API contract is recovered from additional callers or authoritative SDK documentation.

## Runtime limitation
This is offline APK and static guest-binary analysis with host tests and an AArch64 object compile. It does not execute the original game, open Android assets at runtime, or demonstrate boot/gameplay.
