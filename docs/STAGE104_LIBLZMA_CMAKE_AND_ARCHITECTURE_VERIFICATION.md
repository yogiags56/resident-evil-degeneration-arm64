# Stage 104 — liblzma CMake and architecture verification

The Android native CMake target uses `find_package(LibLZMA REQUIRED)` and links `LibLZMA::LibLZMA`. Stage 104 now validates that same imported-target usage on the host using the production `re_dtrz_lzma.c`, via `tools/run_host_liblzma_cmake_smoke.sh`.

This host smoke is intentionally separate from Android validation. It found system liblzma 5.8.1, built the production decoder, and passed six decoder scenarios. It does not establish Android ABI compatibility.

`tools/verify_android_liblzma.sh` inspects an object member from the static archive and rejects anything other than ELF64/AArch64. The build scripts invoke it before linking. A negative test with the host x86_64 archive was correctly rejected.

Android NDK and XZ source remain unavailable in the current environment. See `STAGE104_REPORT.md`.
