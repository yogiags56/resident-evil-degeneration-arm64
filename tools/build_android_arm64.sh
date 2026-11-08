#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to an installed Android NDK directory}"
: "${ANDROID_LIBLZMA_ROOT:?Set ANDROID_LIBLZMA_ROOT to an arm64-v8a liblzma prefix}"
command -v cmake >/dev/null
command -v ninja >/dev/null
command -v python3 >/dev/null
bash "$ROOT/tools/android_toolchain_doctor.sh"
bash "$ROOT/tools/android_arm64_preflight.sh"
case "$(uname -s)-$(uname -m)" in
  Linux-x86_64) NDK_HOST="linux-x86_64" ;;
  Darwin-x86_64) NDK_HOST="darwin-x86_64" ;;
  Darwin-arm64) NDK_HOST="darwin-arm64" ;;
  *) echo "ERROR: unsupported NDK host platform" >&2; exit 2 ;;
esac
NDK_BIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$NDK_HOST/bin"
bash "$ROOT/tools/verify_android_liblzma.sh" "$ANDROID_LIBLZMA_ROOT" \
  "$NDK_BIN/llvm-readelf" "$NDK_BIN/llvm-ar"
cd "$ROOT"
if [[ -x "$ROOT/gradlew" ]]; then
  "$ROOT/gradlew" --no-daemon :app:assembleDebug
elif command -v gradle >/dev/null 2>&1; then
  bash "$ROOT/tools/provision_gradle_wrapper.sh"
  "$ROOT/gradlew" --no-daemon :app:assembleDebug
else
  echo "ERROR: neither gradlew nor system Gradle is available." >&2
  echo "Install Gradle 8.9+ or generate the wrapper before building." >&2
  exit 2
fi
LIB="$ROOT/app/build/intermediates/cxx/RelWithDebInfo"
FOUND="$(find "$ROOT/app/build" -type f -name 'libre_asset_probe.so' -path '*arm64-v8a*' -print -quit)"
if [[ -z "$FOUND" ]]; then
  echo "ERROR: ARM64 libre_asset_probe.so not found after Gradle build" >&2
  exit 1
fi
bash "$ROOT/tools/verify_android_arm64_elf.sh" "$FOUND"
echo "Android ARM64 native library verified: $FOUND"
