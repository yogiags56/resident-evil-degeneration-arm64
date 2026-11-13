# Stage 123Y — Guest libc stdio and file I/O foundation

## Evidence and scope

The Stage 123O relocation manifest identifies these imports in the original
ELF32 module's direct `libc.so` dependency: `fopen`, `fclose`,
`fread`, `fwrite`, `fseek`, `ftell`, and `fgets`. This stage adds
a guest-callable provider for exactly those seven names.

## Architecture

Added:

- `runtime/guest/re_guest_libc_stdio_provider.h`
- `runtime/guest/re_guest_libc_stdio_provider.c`
- `tests/test_re_guest_libc_stdio_provider.c`

The provider owns a fixed table of 32 stream records. A guest `FILE *`
value is an opaque 32-bit token with a generation field, not a host pointer
and not a host `FILE *`. Stale or malformed tokens are rejected. Provider
contexts and stream records have stable addresses when the provider remains
at its initialized owner address.

All host I/O is delegated through an explicit backend vtable. The provider
does not call host stdio and does not expose backend stream pointers to the
guest. Backend lifetime and synchronization remain the caller's
responsibility.

## Implemented imports

- `fopen`: bounded guest path/mode reads, `r`, `w`, and `a` modes,
  optional `+` and `b`, and bounded slot allocation.
- `fclose`: closes the backend stream and invalidates its token.
- `fread` / `fwrite`: checked guest spans, overflow-safe byte counts,
  and standard complete-item return counts.
- `fseek`: signed 32-bit offset and `SEEK_SET/CUR/END` values, using
  AAPCS32 r0/r1/r2 argument placement.
- `ftell`: returns a signed 32-bit-compatible position or -1.
- `fgets`: bounded guest destination, newline termination, and NUL
  termination.

Guest paths are normalized before backend open: absolute paths, drive/colon
syntax, and parent traversal components are rejected; repeated separators
and dot components are folded. The backend must still enforce its own
sandbox and mount policy.

## Regression coverage

The host regression binds all seven exact ELF symbol indices and executes
guest Thumb calls through the real import dispatcher. It exercises open,
multi-item read, seek and tell, bounded line read, write, and close against
a deterministic in-memory backend. It verifies the guest-visible token
rather than a host pointer and checks file contents after writing.

The host regression is registered in `tools/run_tests.sh`; the provider is
included in the Android ARM64 CMake target.

## Deliberate limitations

- This is a provider foundation, not production module-loader installation.
- The Android AssetManager backend is read-only and is not directly adapted
  to this read/write stdio vtable. A writable private-storage/sandbox backend
  and an explicit adapter are still required.
- `fprintf`, `printf`-to-file, `sscanf`, `fscanf`, `feof`,
  `ferror`, `fflush`, and the remaining POSIX file APIs are not included.
- No host `FILE *` or guest `FILE *` structure is dereferenced.
- Original-game file access sequences, device execution, JNI initialization,
  and game boot are not validated by this stage.

## Validation

Host and Android ARM64 CI for the final Stage 123Y source revision are
pending. Earlier source checks passed before the final path-normalization
hardening; do not treat those runs as validation of the current head.

## Next

1. Complete host regression and Android ARM64 build for the final source SHA.
2. Add a bounded guest file-output sink and implement relocation-backed
   `fprintf` using the shared AAPCS32 formatter.
3. Add a sandboxed writable Android backend and explicit AssetManager /
   private-storage adapters.
4. Integrate the provider into the module-load lifecycle only after the
   guest heap, backend, provider, and callback lifetimes are explicit.
