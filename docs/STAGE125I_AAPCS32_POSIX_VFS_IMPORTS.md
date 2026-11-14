# Stage 125I — AAPCS32 POSIX VFS import adapters

## Purpose

Make the Stage 125H read-only descriptor backend callable by the original
ARM32 guest through the existing mapped import dispatcher. This stage binds
five exact libc import names to provider-owned contexts and marshals their
AAPCS32 arguments without treating guest addresses as host pointers.

## Exports

The provider publishes five relocation-backed `libc.so` exports:

- `open`
- `read`
- `lseek`
- `close`
- `access`

Each export receives a guest Thumb trampoline from the shared import table.
The provider object and its filesystem must remain alive while those
trampolines are callable.

## ABI and memory safety

- `open(path, flags)` reads a bounded NUL-terminated path from guest memory.
- `read(fd, buf, count)` resolves the complete writable guest output range
  before reading archive bytes. The implementation follows the actual
  AAPCS32 argument registers: fd=r0, buf=r1, count=r2.
- `lseek(fd, offset, whence)` uses the even-word aligned 64-bit offset at
  argument words 2/3 (r2/r3) and reads the fifth argument from the guest stack.
- `close(fd)` and `access(path, mode)` dispatch through the same descriptor
  backend.
- Guest path reads are bounded to 512 bytes and each byte is checked through
  the memory bus. No guest pointer is dereferenced directly by the host.
- An optional caller-supplied guest errno address receives a 32-bit errno
  value; the provider also retains `last_errno` for diagnostics. A zero
  address disables guest errno writes.

The current errno mapping covers EFAULT, EBADF, EACCES, ENOENT, EMFILE,
EINVAL, and EIO. The adapter uses a fixed caller-supplied errno cell, not a
thread-local `__errno_location()` implementation.

## Regression

The mapped-runtime regression binds all five exports and invokes them by
traversing the real import trampoline dispatcher. It verifies open/read,
64-bit lseek stack/register marshalling, access checks, guest errno writes,
invalid guest output-buffer rejection, close and double-close behavior, and
exact `libc.so` export identity.

## Integration and remaining work

The source is registered in Android ARM64 CMake and the host test runner.
This provider is not yet included in the combined libc provider set or
combined exact-SONAME catalog, and it does not implement `stat` dispatch,
`write`, `pread`, directory APIs, thread-local errno, or full Bionic
semantics. The archive backend remains read-only. The production XE3U/ELF
loader still does not instantiate this provider automatically. No original
game boot is claimed.

## Validation

Host source checks and Android ARM64 native build are required for the final
commit.
