# Stage 123V — Guest libc memory and string provider

## Evidence and scope

The Stage 123O ELF32 manifest records the following relocation-backed names
in the direct `libc.so` dependency. This stage binds exactly 15 of them:
`memcpy`, `memmove`, `memset`, `strlen`, `strcmp`, `strncmp`,
`strcpy`, `strncpy`, `strncat`, `strchr`, `strstr`, `strcasecmp`,
`strncasecmp`, `atoi`, and `strtok`.

## Implementation

Added:

- `runtime/guest/re_guest_libc_string_provider.h`
- `runtime/guest/re_guest_libc_string_provider.c`
- `tests/test_re_guest_libc_string_provider.c`

The provider validates exact undefined ELF32 symbol indices and binds all
15 or none. Its exports are scoped to the evidenced direct `libc.so`
dependency.

Bulk memory operations use `re_guest_memory_resolve` to verify guest
permissions and contiguous mapped ranges before obtaining temporary host
views. The host pointers are used only inside the native callback and are
never returned to the guest. Byte-oriented string operations read and write
through the guest memory bus, with checked 32-bit guest-address arithmetic.

String scans are capped at 65,536 bytes. Unterminated or unmapped strings
fail the import callback rather than running unbounded. `strncpy` preserves
zero padding; `strncat` enforces a bounded destination length; `strstr`
returns a guest address; and `strchr` can return the address of a terminating
NUL when searching for byte zero.

`strtok` keeps its continuation address in provider-owned state and
terminates tokens by writing NUL into guest memory. The provider object and
its embedded handler contexts must remain at a stable address for the
lifetime of the import table.

## Defined compatibility policies

- `strcasecmp` and `strncasecmp` use ASCII-only case folding. Locale-aware
  behavior is not emulated.
- `atoi` parses an optional sign and decimal digits, returning zero when no
  digits are present. Out-of-range values saturate to signed 32-bit limits to
  avoid undefined host C overflow; exact invalid-input behavior of the
  original runtime remains unverified.
- `strtok` uses one continuation slot per provider instance. It is not a
  per-thread `strtok_r` implementation; callers must serialize access.
- Invalid guest pointers, arithmetic overflow, and strings without a bounded
  terminator return an import-handler error. This is a fail-closed runtime
  policy rather than a claim to reproduce undefined C behavior.
- `memcpy` assumes the guest caller obeys the C non-overlap contract.
  `memmove` is the overlap-safe alternative.

## Deliberate limitations

- Only the 15 exact relocation-backed symbols listed above are implemented.
  `strtok_r`, `strdup`, `strtol`, `strlcpy`, formatted I/O, and the
  remaining libc/POSIX family are not included.
- Bulk `memcpy`/`memmove`/`memset` require each requested range to resolve
  as one contiguous mapped guest region.
- This provider is linked into the Android ARM64 target but is not yet
  installed by the production ELF32 module loader.
- No original-game string/memory call sequence or physical-device execution
  has been validated.

## Validation

The host regression binds all 15 exact symbol indices and exercises bulk
memory operations, overlapping `memmove`, string length/comparison/copy/
concatenation/search, integer parsing, and stateful tokenization. The Android
ARM64 target compiles and links the provider as part of the native app.

## Next

1. Confirm the latest host regression run passes with all 15 provider tests.
2. Wire the shared heap and libc providers into the production module-load
   lifecycle only after the guest arena and callback lifetime are defined.
3. Continue with formatted I/O and file-backed libc imports using explicit
   guest FILE abstractions; never pass guest `FILE *` values to host stdio.
