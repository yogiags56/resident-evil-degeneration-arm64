# Stage 97 — Original S3E Binary and Guest File API Evidence

## Result

Stage 97 re-opened the user-supplied original Android APK and performed a fresh static audit of its actual `assets/biohazard.s3e`. The LZMA-alone stream decoded to a valid `XE3U` image. The XE3U fixup table independently confirmed the four file API symbols and their external relocation cells. A Thumb disassembly of the code blob recovered a much broader set of callsites than the earlier reports.

The most important new finding is that `s3eFileRead` returns an **element/item count**, not a byte count. Multiple original guest callsites compare its `r0` result with `1` after requesting one element, including a call where `item_size` is variable. This closes the largest Stage 89/96 uncertainty, but it does not yet prove every EOF, error, or partial-item edge case.

## Input provenance

The APK was supplied by the user for this project. It is not copied into this repository.

| Input | Size | SHA-256 |
|---|---:|---|
| Original APK | 20,001,777 | `6654b4af083a899b5c75de92a66b485435b3ce6ac65afbe96082d5ede4eb23d2` |
| `assets/biohazard.s3e` compressed | 673,661 | `3b673d9e88071ad6a83ff892eef394447cf4cb81e540a11ae31f9265a181c271` |
| `assets/biohazard.s3e` decoded | 1,660,600 | `66b4987fbc419be8b4c91e0d0ee1f57fd8cb40771e92ee7c38658b799fb5e5fd` |
| XE3U code+initialized-data blob | 1,509,476 | `5fc9a53a90895eff456181357806f27b7566e7841df87328b6459c1715e79006` |
| `assets/main.dz` | 14,552,804 | `1de13896c0f2b1805d3808e72d9e95a7f181b2805dfc966d4a606d134f77927e` |
| `assets/s3e.icf` | 18,490 | `1bad3703e805b557faa2bec7cde91dd0e82e68fb92294b9fbc112592d1f8aa9e` |
| `lib/armeabi/libjirobiodg.so` | 431,528 | `ce6c9f6f7339f226c1f060744ddb89d7b187d01a8b20571cd417896bf4c4fcf0` |

The APK size is 20,001,777 bytes. The file is a ZIP/APK with 15 entries. `libjirobiodg.so` is an ELF32 little-endian ARM EABI5 shared object. It is not the guest game image; the game code is in the separate `biohazard.s3e`.

## XE3U header and layout

The compressed S3E asset starts with the LZMA-alone properties byte `0x5D`; decompression yields `XE3U` at offset zero. The decoded image is 1,660,600 bytes.

| Header field | Observed value |
|---|---:|
| Version raw | `0x00041901` |
| Flags | `0x0002` |
| Architecture field | `5` (not assigned a meaning here) |
| Fixup offset / size | `0x48` / `0x24CC4` |
| Code offset | `0x24D0C` |
| Code file size | `0x170864` |
| Code memory size | `0x18B430` |
| Entry offset | `0` |
| Link-time base address | `0x4A000000` |
| Extended header size | `8` |
| Data offset | `0x16C018` |

The header offsets and fixup-section boundaries are internally consistent: `code_offset == fixup_offset + fixup_size`, and `code_offset + code_file_size == extra_offset`. The code blob is therefore disassembled from `image[0x24D0C : 0x24D0C + 0x170864]`, not from the raw file at the same guest RVA. This corrects an important source of ambiguity when inspecting the packed XE3U image.

## Fixup table evidence

The fixup section parses into four subsections:

| Type | Meaning | Size | Count |
|---:|---|---:|---:|
| 0 | API symbol names | `0xEA8` | 227 symbols |
| 1 | Internal relocations | `0x23870` | 36,377 |
| 2 | ARM external relocations | `0x0C` | 0 |
| 4 | Thumb external relocations | `0x5A0` | 238 |

The external relocation table confirms these exact symbol-to-cell mappings:

| API symbol | External relocation cell / code offset |
|---|---:|
| `s3eFileClose` | `0x13B71C` |
| `s3eFileRead` | `0x13B7E0` |
| `s3eFileOpen` | `0x13BA50` |
| `s3eFileGetChar` | `0x13BA6C` |

These offsets are external relocation cells associated with the API symbols; they are not the guest callsite addresses. The corresponding code uses API thunks at `0xC0`, `0x30C`, `0xA5C`, and `0xAB0`, respectively. This distinguishes the symbol relocation cells from the branch destinations and from the callsites.

## Fresh code xref scan

A Thumb disassembly of the extracted code blob found these direct calls to the four API thunks:

| API | Thunk | Direct call references |
|---|---:|---:|
| `s3eFileRead` | `0x30C` | 35 |
| `s3eFileOpen` | `0xA5C` | 11 |
| `s3eFileGetChar` | `0xAB0` | 5 |
| `s3eFileClose` | `0xC0` | 13 |

The `s3eFileRead` callsites are:

`0x61B2, 0x83A06, 0xADE26, 0xADE4A, 0xADED6, 0xAFF44, 0xB02FC, 0xB0378, 0xB03C6, 0xB03E6, 0xB0428, 0xB926A, 0xB9290, 0xB93F8, 0xB94B2, 0xB9516, 0xB957A, 0xB95EE, 0xB9664, 0xB96D4, 0xB9742, 0xB97C6, 0xB9DD0, 0x13A00E, 0x13A052, 0x13A05E, 0x13A06A, 0x13A07A, 0x13A0C8, 0x13A0EC, 0x13A0F8, 0x13A198, 0x13A1CA, 0x13A216, 0x13A226`.

## `s3eFileRead` return contract — materially improved

### Proven by original guest callsites

The original guest passes the fread-style arguments `(buffer, elemSize, noElems, file)` in `r0-r3`. Several callers immediately compare the result in `r0` with `1` after requesting one element. At `0xB0428`, the caller sets `r2 = 1`, loads a variable item size into `r1`, calls `s3eFileRead`, and compares `r0` with `1`. If the API returned bytes, the expected value would depend on that variable item size; the guest instead expects one item. This is direct evidence that the result counts items/elements, not bytes.

Other return-consuming sites include `0x61B2`, `0x83A06`, `0xADE26`, `0xADE4A`, `0xADED6`, `0xB02FC`, `0xB0378`, `0xB03C6`, and `0xB03E6`. At `0xB9DD0`, a one-byte/one-item read is followed by checks for zero and `-1`; both are rejected by that caller's success path.

The public Marmalade file API tutorial corroborates the signature `uint32 s3eFileRead(void *buffer, uint32 elemSize, uint32 noElems, s3eFile *file)` and demonstrates checking for one returned item when requesting one item. The signature is consistent with the guest binary. This public tutorial is supporting documentation, not a version-matched official SDK header.

### Still not fully closed

- Exact semantics for a short read where fewer than `noElems` complete items are available.
- Whether a partial final item can be copied while excluded from the returned item count.
- Whether `0xFFFFFFFF` is a documented error sentinel for `s3eFileRead` or merely a value this caller rejects.
- Exact SDK error-state side effects and `s3eFileGetError` behavior.

Therefore the return **unit** is now established as items, but the implementation must still preserve conservative handling of EOF/error and partial reads. Do not return a host byte count in guest `r0`.

## Other file API findings

### `s3eFileOpen`

The binary has 11 direct references to its thunk. The two examined reader constructors at `0x139FDC` and `0x13A168` pass a path in `r0` and the NUL-terminated mode string `rb` in `r1`. The result is checked against null and stored as a handle-like value.

### `s3eFileGetChar`

The binary has five direct references to its thunk. Helper `0x139F8C` checks a remaining-byte counter, decrements it, loads the handle from descriptor offset `+4`, calls `s3eFileGetChar`, and converts `-1` into its EOF/error path. This is direct evidence for the `-1` sentinel used by this helper.

### `s3eFileClose`

The binary has 13 direct references to its thunk. The shared helper at `0x13A348` loads the stored handle, skips the API call if it is null, calls `s3eFileClose`, and clears the stored handle to zero afterward. The caller does not consume the API return value. This establishes the guest's null-check/clear pattern, but not close-error semantics.

## New tooling

`tools/s3e_binary_audit.py` accepts either an APK containing `assets/biohazard.s3e` or a compressed/uncompressed S3E file. It:
- decompresses LZMA-alone streams and checks the `XE3U` magic;
- validates header and fixup bounds;
- parses the symbol table and external relocation entries;
- reports the four target file API relocation cells and SHA-256 hashes;
- optionally writes the code+initialized-data blob for independent disassembly with `llvm-objdump`;
- never patches or executes the input binary.

The tool deliberately does not attempt to disassemble code. The callsite inventory above is a separately recorded static-analysis result from the extracted code blob.

## Stage 97 activation decision

**Production guest file imports remain disabled.** Stage 97 closes the `s3eFileRead` return unit (items), confirms the API symbol mappings and expands the xref inventory, but the exact partial-read/error behavior and full handle lifecycle under the original runtime are not yet fully verified. Stage 98 should implement a backend-backed guest file bridge with the item-count return contract, plus tests for full reads, EOF, partial reads, invalid/stale handles, repeated close, and cleanup. Keep any uncertain error sentinel behavior isolated until it is proven.

## Reproduction

```bash
python3 tools/s3e_binary_audit.py /path/to/original.apk --json /tmp/s3e-audit.json --code-out /tmp/biohazard-code.bin
llvm-objdump -D --triple=thumbv7-none-eabi /tmp/biohazard-code.bin
python3 -m unittest discover -s tests -v
bash tools/run_tests.sh
```

The original APK, extracted `biohazard.s3e`, and decoded game binary are intentionally not distributed in this source repository.

## External format/API references

- Binary Backtrace, [Nokia N-Gage and Symbian File Formats — S3E (Marmalade)](https://binarybacktrace.com/formats/n-gage): reference for the XE3U header, fixup subsections, external relocation records, API stub model, and LZMA-compressed S3E detection. The observed APK header/fixup boundaries and symbol offsets were parsed independently from the supplied image.
- DrMop, [Marmalade SDK Tutorial — Dealing with files and the file system](https://www.drmop.com/index.php/2011/10/07/marmalade-sdk-tutorial-dealing-with-files-and-the-file-system/): public API signatures and `s3eFileRead` usage example. It is not treated as a version-matched SDK header.
