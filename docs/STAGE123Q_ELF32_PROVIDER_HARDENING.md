# Stage 123Q — ELF32 provider metadata hardening

## Scope

This follow-up hardens the boundary between decoded ELF32 dynamic symbols and
the guest libdl provider catalog. It does not change dependency resolution
order, load bias policy, or relocation application.

## Changes

- Reject symbol records whose fixed-size name field has no NUL terminator.
  This prevents later symbol resolution from reading beyond the record.
- Export only STT_NOTYPE, STT_OBJECT, and STT_FUNC definitions.
  STT_SECTION, STT_FILE, TLS, and other unsupported symbol kinds are not
  callable/data exports.
- Validate provider SONAMEs as printable, non-whitespace identifiers without
  path separators.
- Preserve transactional output behavior: validation failures occur before
  writing any provider entries.

## Regression coverage

The provider-catalog test now covers unterminated symbol names, whitespace in
SONAMEs, and filtering of section/file symbols, in addition to the existing
load-bias, absolute-symbol, visibility, weak/strong, range, and capacity cases.

The guest liblog adapter now preserves newline and other control bytes in log
messages rather than rejecting them as malformed strings. It returns the
platform -EINVAL value for invalid guest strings/format requests. A
multiline-message regression was added.

## Validation status

The source changes are committed to
stage-120z-dependency-graph-resolution. GitHub Actions has repeatedly
terminated these jobs before assigning a runner (runner_id = 0, empty
runner name, and no executed steps). Therefore no passing CI result is claimed.
The source and Android ARM64 build must still be executed on a provisioned
runner before this stage is considered validated.
