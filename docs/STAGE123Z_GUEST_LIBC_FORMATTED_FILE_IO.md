# Stage 123Z — Guest libc formatted file output and stream status

## Scope

This stage extends the Stage 123Y guest stdio provider with the formatted
file-output and stream-state imports that can be implemented safely using the
existing bounded AAPCS32 formatter.

## Added imports

The stdio provider now binds eleven exact ELF32 undefined symbols:

- Existing: `fopen`, `fclose`, `fread`, `fwrite`, `fseek`, `ftell`,
  and `fgets`.
- Added: `fprintf`, `feof`, `ferror`, and `fflush`.

`fprintf` reuses `re_guest_format_guest_printf`, starts its variadic
argument stream at AAPCS32 word index 2 (r2/r3 then guest stack), and writes
the fully formatted bounded output through the backend. Short writes and
format/memory failures mark the stream error indicator. Output remains
bounded by the existing formatter output capacity.

`feof` and `ferror` expose per-stream EOF/error indicators. A short read
or end-of-file in `fgets` sets EOF; a successful `fseek` clears EOF.
Backend read/write/flush failures set the error indicator.

`fflush(FILE*)` delegates to the optional backend flush callback. A null
stream flushes all active streams. Backends without a flush callback are
treated as unbuffered. Writable streams are flushed before close; a flush
failure makes `fclose` return -1 while still closing the backend stream.

## ABI and safety

Guest `FILE*` values remain opaque 32-bit generation tokens. No host
pointer or host `FILE*` is exposed. Formatting consumes only guest AAPCS32
argument words and validated guest strings; output is passed to the backend
as a bounded byte span. The optional flush callback is appended to the
backend vtable, preserving source compatibility for zero-initialized
optional members but requiring positional initializers to add the final
field when they want flushing.

## Regression coverage

The stdio host regression now binds all eleven symbols and exercises
`fprintf` with mixed string/integer variadic arguments, output byte count,
EOF detection, EOF clearing after seek, `fflush`, and `ferror` through the
real guest import dispatcher. The test runner links the shared formatter
into this test. The Android CMake target already includes both the stdio
provider and formatter.

## Explicitly deferred

This stage does not claim `fscanf` or `sscanf`. A safe scanf implementation
needs a dedicated bounded format parser, exact destination-width validation,
guest write-span checks for each conversion, assignment-count semantics,
and careful handling of length modifiers and suppressed conversions. It
will be a separate stage rather than an unsafe reuse of host scanf with
guest pointers.

Also not included: `setvbuf`, `ungetc`, `fgetpos`, `fsetpos`,
`perror`, wide-character I/O, or production module-loader lifecycle wiring.
The Android AssetManager backend remains read-only; a writable sandboxed
backend is still required for game save/config output.

## Validation

Host regression and Android ARM64 build/ELF verification are pending for
this stage's final source revision.
