# Stage 103 — Android ARM64 liblzma build

This repository requires an Android `arm64-v8a` liblzma library. A host liblzma build is not acceptable: it would have the wrong ABI and cannot be linked into the Android shared object.

The new `tools/build_liblzma_android_arm64.sh` builds an unpacked XZ Utils source tree with the Android NDK CMake toolchain, API 23, `arm64-v8a`, static linking, and position-independent code.

Prerequisites:
- Android NDK with `build/cmake/android.toolchain.cmake`
- CMake and Ninja
- XZ Utils source tree containing `CMakeLists.txt` (5.4 or newer recommended)

Example:

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
export ANDROID_LIBLZMA_ROOT=/path/to/deps/android-arm64
export XZ_SOURCE_DIR=/path/to/xz-source
bash tools/build_liblzma_android_arm64.sh
```

Then run `tools/build_android_arm64.sh`. The main build preflight checks the NDK and liblzma headers/library. The build script accepts a committed `gradlew` or can generate one using an installed system Gradle 8.9+.

No Android NDK or XZ source tree is bundled in the source repository. Do not use a host-architecture liblzma library for the Android target.
