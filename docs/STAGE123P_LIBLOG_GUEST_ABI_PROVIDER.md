# Stage 123P — Guest liblog ABI provider

## Purpose

Stage 123J identified two relocation-backed liblog candidates:
`__android_log_print` and `__android_log_write`. Stage 123P adds an ARM32
guest ABI adapter which registers exact ELF32 undefined-symbol indices as
mapped import trampolines and exports only guest Thumb addresses under the
virtual SONAME `liblog.so`.

## Implementation

Added:

- `runtime/guest/re_guest_log_provider.h`
- `runtime/guest/re_guest_log_provider.c`
- `tests/test_re_guest_log_provider.c`

The provider is callback-driven. An Android integration can supply a callback
that calls NDK `__android_log_write` in the native ARM64 process. Guest strings
are copied through the permission-checked guest memory bus; guest pointers are
never passed directly to Android's native logging function.

`__android_log_write` is adapted as a fixed three-word call. The variadic
`__android_log_print` path performs its own bounded AAPCS32 argument walk and
formats the message before invoking the fixed callback. Supported conversion
families are integer, character, string, pointer, and floating-point; dynamic
width/precision are supported within limits. Unsupported or unsafe forms,
including `%n`, positional parameters, and long double, fail closed with
`-EINVAL`. The formatter uses a bounded output buffer and rejects malformed,
unterminated, inaccessible, or oversized guest strings.

Binding is transactional. It requires one eligible undefined GLOBAL/WEAK
FUNC/NOTYPE entry for each of the two exact names, installs both mapped
handlers or neither, and emits a two-symbol provider inventory with
`liblog.so` version-library identity.

## Validation

The host regression test is written to cover exact symbol-index binding,
generated guest trampoline exports, missing-symbol rollback, and guest
dispatch of both functions. It also exercises a variadic format containing
signed integer, guest string, zero-padded hexadecimal, and literal percent
conversions. This test has not yet completed in GitHub Actions.

## CI validation status

The Source checks and Android ARM64 workflow jobs for commit
`e5459e140c8af08e4362c92e0702ddcada3e8d25` were retried. Both attempts ended
within approximately two seconds with `runner_id = 0`, no runner name, and
zero executed steps. GitHub Actions did not provision a runner, so these
results do not establish a source compilation or test failure. The workflow
logs were unavailable. The source and Android builds remain unverified until
a runner is provisioned and the jobs actually execute.

## Limitations

This stage does not claim that every printf extension or locale behavior is
implemented. It does not yet implement a production Bionic/liblog dependency
closure or bind the native callback to Android in the application bootstrap.
The caller must supply the NDK logging callback and preserve the provider
context lifetime. The provider handles only the two relocation-backed log
symbols; it does not make the six-library ELF32 dependency closure complete.
