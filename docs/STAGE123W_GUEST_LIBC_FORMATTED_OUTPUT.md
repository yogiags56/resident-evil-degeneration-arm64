# Stage 123W — Guest libc formatted output

## Evidence and scope

The Stage 123O ELF32 import manifest contains relocation-backed
`snprintf` and `sprintf` references in the direct `libc.so` dependency.
This stage adds guest-callable adapters for exactly those two functions and
extracts the existing liblog AAPCS32 variadic formatter into a shared module.

## Implementation

Added:

- `runtime/guest/re_guest_format.h`
- `runtime/guest/re_guest_format.c`
- `runtime/guest/re_guest_libc_format_provider.h`
- `runtime/guest/re_guest_libc_format_provider.c`
- `tests/test_re_guest_libc_format_provider.c`

The shared formatter now serves both the Stage 123P/Q liblog provider and
Stage 123W libc formatted output. It takes an explicit first variadic
AAPCS32 word index:

- `snprintf(dest, size, format, ...)`: first variadic word index 3 (r3).
- `sprintf(dest, format, ...)`: first variadic word index 2 (r2).

The formatter handles core-register and guest-stack arguments, including
even-word alignment for 64-bit arguments. It reads `%s` values from guest
memory and never passes guest pointers to host stdio as dereferenceable
pointers. Formatting is first completed into a bounded native buffer; only
then is the validated guest destination written.

`snprintf` returns the full formatted length within the provider's bounded
format limit and writes at most `size-1` bytes plus NUL. A zero size performs
no destination access. `sprintf` uses a fixed 2048-byte output ceiling and
fails closed if the result would exceed it.

## Supported format subset

- Integer: `%d`, `%i`, `%u`, `%o`, `%x`, `%X`.
- Character/string/pointer: `%c`, `%s`, `%p`.
- Floating point: `%f`, `%F`, `%e`, `%E`, `%g`, `%G`, `%a`, `%A`.
- Flags: `-`, `+`, space, `#`, `0`, and the apostrophe grouping flag.
- Numeric or dynamic width and precision, each capped at 256.
- Length modifiers: `hh`, `h`, `l`, and `ll`.

Positional arguments, `%n`, `z`/`t`/`j` length modifiers, `L` long
double, and unknown conversions are rejected. The current `%p` conversion
uses the host formatter's pointer rendering on the zero-extended 32-bit guest
address; exact textual parity with the original 32-bit Bionic formatter has
not yet been verified.

## Validation

The host regression checks both calling conventions, register/stack
variadics, 64-bit even-word alignment, dynamic width, truncation, zero-sized
`snprintf`, and deterministic missing/duplicate-symbol diagnostics with
transactional import-table rollback. Existing guest liblog formatter tests
continue to exercise the shared implementation.

Host CI passed in run `43418862945`: 312 Python tests, native C mock-header
syntax checks, and all compiled host regressions, including
`Guest libc formatted-output provider tests: PASS` and
`Guest liblog provider tests: PASS`. Android ARM64 build and ELF verification
passed in run `43418672203`, including artifact upload.

## Deliberate limitations

- Only `snprintf` and `sprintf` are provided. `printf`, `fprintf`,
  `sscanf`, and FILE-backed output remain future work.
- Output is capped at 2048 bytes and width/precision at 256.
- The provider is compiled into the Android ARM64 target but is not yet
  installed by the production ELF32 module loader.
- The original game’s formatting call sites and exact Bionic output behavior
  remain unverified.
