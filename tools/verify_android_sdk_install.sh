#!/usr/bin/env bash
set -euo pipefail
SDK="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
[[ -n "$SDK" ]] || { echo "ERROR: set ANDROID_SDK_ROOT or ANDROID_HOME" >&2; exit 2; }
fail=0
check_file() {
  local label="$1" path="$2"
  if [[ -f "$path" ]]; then echo "PASS: $label ($path)"
  else echo "FAIL: $label missing ($path)"; fail=1; fi
}
check_exec() {
  local label="$1" path="$2"
  if [[ -x "$path" ]]; then echo "PASS: $label ($path)"
  else echo "FAIL: $label missing/not executable ($path)"; fail=1; fi
}
check_file "Android API 35 platform" "$SDK/platforms/android-35/android.jar"
check_exec "aapt2 Build Tools 35.0.0" "$SDK/build-tools/35.0.0/aapt2"
check_exec "zipalign Build Tools 35.0.0" "$SDK/build-tools/35.0.0/zipalign"
check_exec "SDK CMake 3.22.1" "$SDK/cmake/3.22.1/bin/cmake"
check_exec "ADB platform-tools" "$SDK/platform-tools/adb"
NDK="$SDK/ndk/30.0.16248370"
check_file "NDK source.properties" "$NDK/source.properties"
if [[ -f "$NDK/source.properties" ]]; then
  revision="$(sed -n 's/^Pkg.Revision[[:space:]]*=[[:space:]]*//p' "$NDK/source.properties" | head -n1)"
  if [[ "$revision" == "30.0.16248370" ]]; then echo "PASS: NDK revision $revision"
  else echo "FAIL: expected NDK 30.0.16248370; found ${revision:-unknown}"; fail=1; fi
fi
case "$(uname -s)-$(uname -m)" in
  Linux-x86_64) host=linux-x86_64 ;;
  Darwin-x86_64) host=darwin-x86_64 ;;
  Darwin-arm64) host=darwin-arm64 ;;
  *) host="" ;;
esac
if [[ -n "$host" ]]; then
  check_exec "NDK host clang" "$NDK/toolchains/llvm/prebuilt/$host/bin/clang"
  check_exec "NDK llvm-ar" "$NDK/toolchains/llvm/prebuilt/$host/bin/llvm-ar"
  check_exec "NDK llvm-readelf" "$NDK/toolchains/llvm/prebuilt/$host/bin/llvm-readelf"
  check_file "NDK CMake toolchain" "$NDK/build/cmake/android.toolchain.cmake"
else
  echo "FAIL: unsupported host platform $(uname -s)-$(uname -m)"
  fail=1
fi
if (( fail )); then
  echo "BLOCKED: one or more pinned Android packages are incomplete." >&2
  exit 1
fi
echo "PASS: all pinned Android SDK/NDK components verified."
