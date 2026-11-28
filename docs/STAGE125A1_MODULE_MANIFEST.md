# Stage 125A.1 — bounded guest-module manifest

## Format v1

The parser accepts a bounded UTF-8/ASCII-compatible text manifest:

```text
RE-GUEST-MODULES 1
root game.so
module libc.so system/libc.so 16777216
module game.so payload/game.elf 67108864
```

- Header is mandatory and versioned.
- Exactly one `root <soname>` record is required.
- Each module record contains an exact SONAME, relative Android asset path, and maximum permitted byte size.
- Blank lines and full-line comments are allowed.
- Maximum manifest size is 16 KiB; maximum module count is 32; each module size limit is 1..128 MiB.
- SONAMEs are printable ASCII and cannot contain path separators.
- Asset paths reject absolute paths, traversal components, empty/repeated components, trailing separators, control characters, and colon.
- Duplicate SONAMEs and duplicate asset paths are rejected.
- The root SONAME must resolve to exactly one declared module.

## Implementation

Added `re_guest_module_manifest.h/.c`. Parsing is transactional: the caller's output manifest remains byte-for-byte unchanged on failure. The parser reports the failing line for line-scoped errors. It does not read assets, trust filenames as SONAMEs, or bypass ELF header validation; those are subsequent loader stages.

Registered the parser in Android native CMake and the host test runner.

## Regression coverage

- Valid CRLF manifest, root resolution, exact SONAME/path and byte limits.
- Duplicate SONAME rejection.
- Path traversal and trailing separator rejection.
- Missing root module rejection.
- Numeric overflow/invalid size rejection.
- Unsupported manifest version rejection.
- Oversized manifest rejection.
- Transactional output preservation on all tested failures.

## Validation boundary

Host CI and Android ARM64 native build must pass on the final commit. This slice defines and parses inventory metadata only; it does not materialize guest assets, calculate cryptographic digests, validate the ELF machine/class against records, or start the guest runtime. No game boot is claimed.
