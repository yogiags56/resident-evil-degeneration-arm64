# Stage 124A — Guest libc bounded scanf parser

## Scope

Adds a dedicated bounded scanf-family parser. It does not reuse host
`scanf`/ `sscanf` and never passes guest destination pointers to host libc.

## Parser conversions

Implemented conversions:

- Integers: `%d`, `%i`, `%u`, `%o`, `%x`, `%X`, and `%p`.
- Floating point: `%f`, `%e`, `%g`, `%a` families, including `%lf`.
- Text: `%s`, `%c`, and scansets `%[...]` with ranges and inversion.
- Control: literal matching, format whitespace, `%%`, assignment suppression,
  field widths, and `%n`.
- Length handling for ARM32 destination widths: `hh`, `h`, `l`, `ll`,
  `z`, `t`, and `j` where applicable. ARM32 `long`, `size_t`, and
  `ptrdiff_t` are 32-bit; `long long`/intmax destinations are 64-bit.

The parser has a 2048-byte input/format cap and a 256-byte token cap.
Oversized fields/formats are rejected rather than silently overflowing.
Each destination is validated with guest WRITE permission and the exact
conversion width before copying bytes. Matching failure is distinct from
invalid format or guest-memory failure. `%n` stores consumed bytes but is
not included in the assignment count.

## libc imports

The stdio provider now binds thirteen exact imports, adding `sscanf` and
`fscanf` to the eleven Stage 123Z imports.

- `sscanf` reads a bounded guest C string and format string; variadic
  destination pointers begin at AAPCS32 argument word 2.
- `fscanf` reads a bounded snapshot from a seekable backend, parses it, then
  seeks to the exact consumed position so unread lookahead bytes remain
  available to later operations. This implementation therefore requires
  the existing backend tell/seek contract and is not suitable for pipes or
  non-seekable streams.

Both return the number of assignments, or guest EOF (-1) when input failure
occurs before any assignment. Invalid formats and guest memory faults fail
the import and set the stream error indicator for `fscanf`.

## Regression coverage

A dedicated scanner test covers signed decimal, string writes, assignment
suppression, scanset ranges, field width, fixed-width characters, `%n`,
matching failure, input failure, and invalid guest destination ranges.
The stdio dispatcher test binds and invokes both `sscanf` and `fscanf`,
including stream position restoration and EOF state.

## Deferred

Wide-character conversions, locale-specific parsing, positional arguments,
GNU allocation modifiers, long-double conversions, non-seekable `fscanf`,
and complete C/POSIX scanf edge-case compatibility remain out of scope.
The host regression and Android ARM64 CI results are the release gate.
