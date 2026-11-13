# Stage 123R — Guest libz ABI provider

## Evidence and scope

The original ELF32 dependency manifest records exactly four relocation-backed
undefined libz names: `inflate`, `inflateEnd`, `inflateInit2_`, and
`inflateInit_`. They belong to the direct `DT_NEEDED` dependency
`libz.so`. This stage implements only those four interfaces. It does not
confuse zlib with the separate DTRZ/LZMA asset-decoding path.

## ARM32 guest stream contract

The adapter uses the standard 32-bit zlib `z_stream` field layout (14
32-bit words, 56 bytes):

| Offset | Field |
|---:|---|
| 0 | next_in |
| 4 | avail_in |
| 8 | total_in |
| 12 | next_out |
| 16 | avail_out |
| 20 | total_out |
| 24 | msg |
| 28 | state |
| 32 | zalloc |
| 36 | zfree |
| 40 | opaque |
| 44 | data_type |
| 48 | adler |
| 52 | reserved |

The guest `state` field receives a bounded opaque 32-bit token. It is not a
host pointer and must not be dereferenced by guest code. Each active guest
stream is associated with a native host `z_stream` object in a bounded
16-slot provider table.

## Implementation

Added:

- `runtime/guest/re_guest_zlib_provider.h`
- `runtime/guest/re_guest_zlib_provider.c`
- `tests/test_re_guest_zlib_provider.c`

The provider binds exactly the four relocation-backed symbol indices as
guest Thumb import trampolines and exports them under `libz.so`. It calls
the native zlib implementation linked into the ARM64 app, but never passes
guest pointers to native zlib. Input bytes are copied from permission-checked
guest memory into bounded native buffers; output bytes are copied back only
after the guest output range is validated. Each inflate call transfers at
most 32 KiB in either direction and updates the guest's 32-bit pointers,
available counts, totals, Adler value, and data type.

`inflateInit_` and `inflateInit2_` validate the guest version major
character and require the 56-byte ARM32 stream size, then initialize a
separate native stream with the host zlib version and host `sizeof(z_stream)`.
`inflateEnd` releases the corresponding native state and clears the guest
opaque token. Provider shutdown releases any remaining native streams.

## Deliberate limitations

- The standard 56-byte ARM32 `z_stream` layout must still be cross-checked
  against the exact SDK/header version used by the original game.
- Guest custom `zalloc`, `zfree`, and `opaque` callbacks are not invoked;
  native zlib uses its default allocator.
- Guest `msg` remains NULL rather than exposing a native host string pointer.
- At most 16 guest streams may be active. Calls for one stream must be
  externally serialized; the provider does not implement cross-thread locks.
- The version check enforces the zlib major version only; it does not prove
  identical minor-version behavior.
- Only the four manifest-backed symbols are bound. This is not the complete
  Android/Bionic dependency closure and does not install the provider into the
  original game's production module-loader lifecycle.

## Validation

The host regression compresses a deterministic fixture with host zlib, calls
the guest `inflateInit_`, `inflate`, and `inflateEnd` trampolines through
the ARM32 runtime, and checks the decompressed bytes and guest stream
bookkeeping. It separately exercises `inflateInit2_`, rejects an invalid
guest stream size, and rejects an unknown state token.

Host CI passed on 2026-11-13: 312 Python tests, native C mock-header syntax
checks, and all compiled host regressions including
`Guest libz provider tests: PASS`. The Android ARM64 build including this
provider is running separately; no device-side zlib call or original-game
boot is claimed by this document.

## Next

1. Complete the Android ARM64 build and verify the shared object links to
   Android's native `libz`.
2. Validate the exact original zlib ABI/version assumptions against the
   original SDK/runtime evidence.
3. Add provider installation to the real ELF32 module-load lifecycle only
   after the provider lifetime and guest stream ownership are explicit.
4. Continue with the libc/Bionic and ARM EABI compiler-runtime families.
