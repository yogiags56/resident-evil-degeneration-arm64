# Current Status

**Current source milestone: Stage 100 complete.** The Stage 98 Open/Read/GetChar/Close handlers now run through the synthetic AAPCS32 guest import dispatcher and a DTRZ-backed file backend. The Stage 99 test passes 31 assertions, including item-count return in guest r0, bytes written to guest memory, GetChar EOF, stale-handle rejection, and stream cleanup. The full host test suite passes. Stage 100 additionally records exact XE3U symbol indices for the four file APIs and adds an Android ARM64 + liblzma CMake preset/preflight script. No Android NDK is installed in this environment, so no Android build is claimed. Production guest file imports remain disabled.

**Still blocked / unverified:** exact original Marmalade partial-item and error behavior, Android NDK build with Android-ABI liblzma, physical-device runtime, production guest file import activation, and original game boot/gameplay. The original APK and game executable are not distributed in the repository.


## Stage 101 update

Android packaging gap identified and addressed: the probe expected game assets that were not present in the Android assets directory. A safe local-only staging script now extracts the three required assets from the user-supplied APK and validates their presence/content. Host tests pass (27 Python tests plus the existing C suite). Android NDK build and device execution are still not performed; `ANDROID_NDK_HOME` is unavailable. Production file imports remain quarantined.


## Stage 102 update

Added a reproducible Gradle build entry point and ELF64/AArch64/JNI post-build verifier. Gradle now explicitly requires LZMA for the native target. Host validation remains green. Android build is blocked by missing NDK, ARM64 liblzma prefix, and Gradle wrapper; no Android binary or device test was produced. Guest production imports remain quarantined.


## Stage 103 update

Added reproducible Android ARM64 liblzma cross-build automation and improved Gradle wrapper/system Gradle handling. The current environment still lacks NDK, XZ source, liblzma prefix, and Gradle. Host-side validation passes; no Android binary or runtime test is claimed.


## Stage 104 update

Host CMake integration of the production LZMA decoder passed using system liblzma 5.8.1 (6 decoder scenarios). An archive verifier now requires ELF64/AArch64 and correctly rejects a host x86_64 archive. Python suite: 35 passed. Android build remains blocked by missing NDK and XZ source; no Android binary/device run is claimed.
