#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
errors=()

if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
  errors+=("ANDROID_NDK_HOME is not set")
elif [[ ! -f "$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" ]]; then
  errors+=("Android NDK CMake toolchain not found under: $ANDROID_NDK_HOME")
else
  case "$(uname -s)-$(uname -m)" in
    Linux-x86_64) ndk_host="linux-x86_64" ;;
    Darwin-x86_64) ndk_host="darwin-x86_64" ;;
    Darwin-arm64) ndk_host="darwin-arm64" ;;
    *) ndk_host="" ;;
  esac
  if [[ -n "$ndk_host" && ! -x "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$ndk_host/bin/clang" ]]; then
    errors+=("NDK LLVM clang missing for host tag $ndk_host")
  fi
fi

if [[ -z "${ANDROID_LIBLZMA_ROOT:-}" ]]; then
  errors+=("ANDROID_LIBLZMA_ROOT is not set (must point to an Android arm64-v8a liblzma prefix)")
elif [[ ! -d "$ANDROID_LIBLZMA_ROOT" ]]; then
  errors+=("ANDROID_LIBLZMA_ROOT does not exist: $ANDROID_LIBLZMA_ROOT")
fi

for tool in cmake ninja python3; do
  command -v "$tool" >/dev/null 2>&1 || errors+=("Required tool not found on PATH: $tool")
done

if ((${#errors[@]})); then
  printf 'ANDROID ARM64 PREFLIGHT BLOCKED:\\n' >&2
  printf ' - %s\\n' "${errors[@]}" >&2
  exit 2
fi

cmake_version="$(cmake --version | awk 'NR==1 {print $3}')"
if [[ "$(printf '%s\\n' 3.22.1 "$cmake_version" | sort -V | head -n1)" != "3.22.1" ]]; then
  echo "ERROR: CMake 3.22.1 or newer is required; found $cmake_version" >&2
  exit 2
fi

if ! find "$ANDROID_LIBLZMA_ROOT" -type f -name 'lzma.h' -print -quit | grep -q .; then
  echo "ERROR: lzma.h not found under ANDROID_LIBLZMA_ROOT" >&2
  exit 2
fi
if ! find "$ANDROID_LIBLZMA_ROOT" -type f \( -name 'liblzma.a' -o -name 'liblzma.so' \) -print -quit | grep -q .; then
  echo "ERROR: no liblzma.a or liblzma.so found under ANDROID_LIBLZMA_ROOT" >&2
  exit 2
fi

cd "$ROOT"
python3 -m unittest discover -s tests -v
cmake --preset android-arm64
cmake --build --preset android-arm64
