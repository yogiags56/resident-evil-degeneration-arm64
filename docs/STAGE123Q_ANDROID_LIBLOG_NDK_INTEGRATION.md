# Stage 123Q — Android NDK liblog callback integration

## Goal

Connect the Stage 123P guest `liblog.so` ABI provider to the Android ARM64
application bootstrap without allowing guest pointers to cross into native
Android APIs.

## Changes

- Add the Android NDK `<android/log.h>` dependency to the native probe.
- Provide a native callback that forwards validated, copied guest tag/message
  strings to `__android_log_write`.
- Add a synthetic guest-memory/runtime smoke path which binds the exact
  `__android_log_print` and `__android_log_write` symbol indices, dispatches
  both guest Thumb trampolines, and verifies callback invocation and guest
  return flow.
- Add a host mock declaration for the Android logging API so native-probe
  translation-unit syntax checks remain possible without an Android SDK.

## Safety properties

- Guest strings are copied and bounded by Stage 123P before the native callback.
- Guest 32-bit pointers are never cast to host pointers for Android logging.
- Only the two exact undefined ELF32 symbol names are bound.
- The integration smoke test uses synthetic symbol metadata and synthetic
  guest strings; it is not proof that the original APK's complete dependency
  closure is satisfied.
- The native callback context is stack-owned only for the duration of this
  probe invocation; a production runtime must give the provider a lifetime
  covering every possible guest call.

## Validation

- GitHub Actions host validation passed on 2026-11-13: 312 Python tests passed, native C translation-unit mock-header syntax checks passed, and the complete compiled host-test executable suite passed, including `test_re_guest_log_provider`.
- Android ARM64 NDK build was started on the same Stage 123Q branch and is still pending at the time of this report.

The existing Stage 123P host regression remains the primary test for provider
binding, formatter behavior, guest memory access, and transactional rollback.
The Android ARM64 workflow must compile and link the new NDK callback. A
successful CI build proves compilation/linkage only; physical-device logcat
observation and original-game execution remain separate validation gates.

## Remaining work

- Build and run the host test suite and Android ARM64 workflow.
- Confirm both guest log calls appear in Android logcat on a physical ARM64
  device.
- Install the provider into the real ELF32 module-load/provider lifecycle
  before guest constructors or JNI_OnLoad can call logging imports.
- Continue with the remaining Bionic/libc, libm, libdl, libz, C++ ABI, and GL/EGL
  provider families. This stage does not close the six-library dependency set.
