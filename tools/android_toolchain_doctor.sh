#!/usr/bin/env bash
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STATUS=0
JSON=0
if [[ "${1:-}" == "--json" ]]; then JSON=1; fi
declare -a KEYS=() VALUES=() DETAILS=()
add() { KEYS+=("$1"); VALUES+=("$2"); DETAILS+=("$3"); [[ "$2" == "PASS" ]] || STATUS=1; }
check_cmd() { command -v "$1" >/dev/null 2>&1; }
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
if [[ -n "$NDK" && -f "$NDK/source.properties" ]]; then
  ver="$(sed -n 's/^Pkg.Revision[[:space:]]*=[[:space:]]*//p' "$NDK/source.properties" | head -n1)"
  if [[ "$ver" == "30.0.16248370" ]]; then add ndk_root PASS "$NDK (pinned revision $ver)"
  else add ndk_root FAIL "expected NDK 30.0.16248370; found ${ver:-unknown}"; fi
else add ndk_root FAIL "ANDROID_NDK_HOME/ROOT unset or source.properties missing"; fi
HOST=""
case "$(uname -s)-$(uname -m)" in
  Linux-x86_64) HOST=linux-x86_64 ;;
  Darwin-x86_64) HOST=darwin-x86_64 ;;
  Darwin-arm64) HOST=darwin-arm64 ;;
esac
if [[ -n "$NDK" && -n "$HOST" && -x "$NDK/toolchains/llvm/prebuilt/$HOST/bin/clang" ]]; then
  add ndk_clang PASS "$NDK/toolchains/llvm/prebuilt/$HOST/bin/clang"
else add ndk_clang FAIL "NDK host clang missing or host platform unsupported"; fi
if [[ -n "$NDK" && -f "$NDK/build/cmake/android.toolchain.cmake" ]]; then
  add ndk_cmake_toolchain PASS "$NDK/build/cmake/android.toolchain.cmake"
else add ndk_cmake_toolchain FAIL "android.toolchain.cmake not found"; fi
if [[ -n "$NDK" && -n "$HOST" && -x "$NDK/toolchains/llvm/prebuilt/$HOST/bin/llvm-readelf" && -x "$NDK/toolchains/llvm/prebuilt/$HOST/bin/llvm-ar" ]]; then
  add ndk_llvm_tools PASS "llvm-readelf and llvm-ar available"
else add ndk_llvm_tools FAIL "NDK llvm-readelf/llvm-ar missing"; fi
if check_cmd cmake && check_cmd ninja; then add cmake_ninja PASS "$(cmake --version | head -n1); $(ninja --version)"
else add cmake_ninja FAIL "cmake and/or ninja missing"; fi
if check_cmd java; then
  jline="$(java -version 2>&1 | head -n1)"
  jver="$(awk -F '"' '{print $2}' <<<"$jline" | cut -d. -f1)"
  if [[ "$jver" =~ ^[0-9]+$ ]] && (( jver >= 17 )); then add java PASS "$jline"
  else add java FAIL "JDK 17+ required; found $jline"; fi
else add java FAIL "Java runtime missing"; fi
if [[ -x "$ROOT/gradlew" && -f "$ROOT/gradle/wrapper/gradle-wrapper.jar" ]]; then
  if grep -Fq 'gradle-8.9-bin.zip' "$ROOT/gradle/wrapper/gradle-wrapper.properties" && grep -Fq 'd725d707bfabd4dfdc958c624003b3c80accc03f7037b5122c4b1d0ef15cecab' "$ROOT/gradle/wrapper/gradle-wrapper.properties"; then
    add gradle PASS "pinned Gradle 8.9 wrapper and checksum"
  else add gradle FAIL "wrapper distribution pin/checksum mismatch"; fi
elif check_cmd gradle; then
  gv="$(gradle --version | sed -n 's/^Gradle //p' | head -n1)"
  if [[ "$gv" =~ ^[0-9]+\.[0-9]+ ]] && awk -v v="$gv" 'BEGIN {split(v,a,"."); exit !((a[1]>8)||(a[1]==8 && a[2]>=9))}'; then
    add gradle PASS "system Gradle $gv available for the Android build"
  else add gradle FAIL "Gradle 8.9+ required to provision wrapper; found ${gv:-unknown}"; fi
else add gradle FAIL "gradlew+wrapper JAR and system Gradle both missing"; fi
SDK="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
ANDROID_CLI="${ANDROID_CLI:-$(command -v android 2>/dev/null || true)}"
if [[ -n "$ANDROID_CLI" && -x "$ANDROID_CLI" ]]; then
  add android_cli PASS "$ANDROID_CLI"
elif [[ -n "${ANDROID_SDKMANAGER:-}" && -x "${ANDROID_SDKMANAGER}" ]]; then
  add android_cli PASS "legacy sdkmanager fallback: $ANDROID_SDKMANAGER"
elif [[ -n "$SDK" && -x "$SDK/cmdline-tools/latest/bin/sdkmanager" ]]; then
  add android_cli PASS "legacy sdkmanager fallback: $SDK/cmdline-tools/latest/bin/sdkmanager"
elif check_cmd sdkmanager; then
  add android_cli PASS "legacy sdkmanager fallback: $(command -v sdkmanager)"
else
  add android_cli FAIL "Android CLI missing; install from developer.android.com/tools/agents/android-cli/download"
fi
if [[ -n "$SDK" && -f "$SDK/platforms/android-35/android.jar" ]]; then add sdk_platform PASS "Android SDK Platform 35 present"
else add sdk_platform FAIL "ANDROID_SDK_ROOT/HOME must contain platforms/android-35/android.jar"; fi
if [[ -n "$SDK" && -x "$SDK/build-tools/35.0.0/aapt2" ]]; then add sdk_build_tools PASS "Android Build Tools 35.0.0 present"
else add sdk_build_tools FAIL "Android Build Tools 35.0.0 missing"; fi
if [[ -n "$SDK" && -x "$SDK/cmake/3.22.1/bin/cmake" ]]; then add sdk_cmake PASS "SDK CMake 3.22.1 present"
else add sdk_cmake FAIL "SDK CMake 3.22.1 missing"; fi
if [[ -n "${ANDROID_LIBLZMA_ROOT:-}" ]]; then
  lib="$(find "$ANDROID_LIBLZMA_ROOT" -type f -name liblzma.a -print -quit 2>/dev/null)"
  hdr="$(find "$ANDROID_LIBLZMA_ROOT" -type f -name lzma.h -print -quit 2>/dev/null)"
  if [[ -n "$lib" && -n "$hdr" ]]; then add liblzma_files PASS "header and static archive found under $ANDROID_LIBLZMA_ROOT"
  else add liblzma_files FAIL "ANDROID_LIBLZMA_ROOT lacks lzma.h and/or liblzma.a"; fi
else add liblzma_files FAIL "ANDROID_LIBLZMA_ROOT unset"; fi
if [[ -n "$NDK" && -n "$HOST" && -n "${ANDROID_LIBLZMA_ROOT:-}" && -x "$NDK/toolchains/llvm/prebuilt/$HOST/bin/llvm-readelf" && -x "$NDK/toolchains/llvm/prebuilt/$HOST/bin/llvm-ar" ]]; then
  if bash "$ROOT/tools/verify_android_liblzma.sh" "$ANDROID_LIBLZMA_ROOT" "$NDK/toolchains/llvm/prebuilt/$HOST/bin/llvm-readelf" "$NDK/toolchains/llvm/prebuilt/$HOST/bin/llvm-ar" >/dev/null 2>&1; then
    add liblzma_arch PASS "static archive contains ELF64 AArch64 object"
  else add liblzma_arch FAIL "archive verification failed (expected ELF64 AArch64)"; fi
else add liblzma_arch BLOCKED "requires NDK LLVM tools and liblzma prefix"; fi
if [[ -n "$SDK" ]] && bash "$ROOT/tools/verify_android_sdk_install.sh" >/dev/null 2>&1; then
  add sdk_install_integrity PASS "all pinned SDK/NDK package files and revisions verified"
else
  add sdk_install_integrity FAIL "run tools/install_android_sdk_packages.sh and inspect package verification"
fi
if [[ "$JSON" == 1 ]]; then
  python3 - "$STATUS" "${KEYS[@]}" -- "${VALUES[@]}" -- "${DETAILS[@]}" <<'PY'
import json,sys
args=sys.argv[1:]; status=args.pop(0); i=args.index("--"); keys=args[:i]; args=args[i+1:]; j=args.index("--"); vals=args[:j]; details=args[j+1:]
print(json.dumps({"overall":"READY" if status=="0" else "BLOCKED","checks":[{"name":k,"status":v,"detail":d} for k,v,d in zip(keys,vals,details)]},indent=2))
PY
else
  printf '%-22s %-8s %s\n' CHECK STATUS DETAIL
  for i in "${!KEYS[@]}"; do printf '%-22s %-8s %s\n' "${KEYS[$i]}" "${VALUES[$i]}" "${DETAILS[$i]}"; done
  if [[ "$STATUS" -eq 0 ]]; then echo "RESULT: READY"; else echo "RESULT: BLOCKED — resolve FAIL checks before Android build"; fi
fi
exit "$STATUS"
