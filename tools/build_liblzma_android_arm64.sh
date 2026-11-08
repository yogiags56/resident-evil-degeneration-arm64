#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to an installed Android NDK directory}"
: "${ANDROID_LIBLZMA_ROOT:?Set ANDROID_LIBLZMA_ROOT to the install prefix for ARM64 liblzma}"
XZ_SOURCE_DIR="${XZ_SOURCE_DIR:-$ROOT/.deps/src/xz-5.8.4}"
if [[ ! -f "$XZ_SOURCE_DIR/CMakeLists.txt" ]]; then
  echo "XZ Utils source not found; fetching and verifying the pinned 5.8.4 release."
  XZ_SOURCE_DIR="$(bash "$ROOT/tools/fetch_xz_source.sh" | tail -n1)"
fi

TOOLCHAIN="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
if [[ ! -f "$TOOLCHAIN" ]]; then
  echo "ERROR: Android NDK CMake toolchain not found: $TOOLCHAIN" >&2
  exit 2
fi
NDK_VERSION="$(sed -n 's/^Pkg.Revision[[:space:]]*=[[:space:]]*//p' "$ANDROID_NDK_HOME/source.properties" | head -n1)"
if [[ "$NDK_VERSION" != "30.0.16248370" ]]; then
  echo "ERROR: expected Android NDK 30.0.16248370; found ${NDK_VERSION:-unknown}" >&2
  exit 2
fi
if [[ ! -f "$XZ_SOURCE_DIR/CMakeLists.txt" ]]; then
  echo "ERROR: XZ_SOURCE_DIR must contain CMakeLists.txt: $XZ_SOURCE_DIR" >&2
  exit 2
fi
EXPECTED_XZ_VERSION="5.8.4"
VERSION_HEADER="$XZ_SOURCE_DIR/src/liblzma/api/lzma/version.h"
if [[ ! -f "$VERSION_HEADER" ]]; then
  echo "ERROR: XZ Utils version header not found: $VERSION_HEADER" >&2
  exit 2
fi
XZ_VERSION="$(awk '
  /^#define LZMA_VERSION_MAJOR / { major=$3 }
  /^#define LZMA_VERSION_MINOR / { minor=$3 }
  /^#define LZMA_VERSION_PATCH / { patch=$3 }
  END { if (major != "" && minor != "" && patch != "") print major "." minor "." patch }
' "$VERSION_HEADER")"
if [[ "$XZ_VERSION" != "$EXPECTED_XZ_VERSION" ]]; then
  echo "ERROR: expected XZ Utils ${EXPECTED_XZ_VERSION}; found ${XZ_VERSION:-unknown}" >&2
  exit 2
fi
command -v cmake >/dev/null
command -v ninja >/dev/null

BUILD_DIR="${XZ_BUILD_DIR:-$ROOT/build/xz-android-arm64}"
mkdir -p "$BUILD_DIR" "$ANDROID_LIBLZMA_ROOT"
cmake -S "$XZ_SOURCE_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-23 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$ANDROID_LIBLZMA_ROOT" \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF \
  -DXZ_TOOL_XZ=OFF \
  -DXZ_TOOL_XZDEC=OFF \
  -DXZ_TOOL_LZMADEC=OFF \
  -DXZ_TOOL_LZMAINFO=OFF \
  -DXZ_TOOL_SCRIPTS=OFF \
  -DXZ_NLS=OFF \
  -DXZ_DOXYGEN=OFF \
  -DXZ_DOC=OFF
cmake --build "$BUILD_DIR" --parallel
cmake --install "$BUILD_DIR"

HEADER="$(find "$ANDROID_LIBLZMA_ROOT" -type f -name lzma.h -print -quit)"
LIBRARY="$(find "$ANDROID_LIBLZMA_ROOT" -type f -name liblzma.a -print -quit)"
if [[ -z "$HEADER" || -z "$LIBRARY" ]]; then
  echo "ERROR: expected lzma.h and liblzma.a were not installed" >&2
  exit 1
fi
case "$(uname -s)-$(uname -m)" in
  Linux-x86_64) NDK_HOST="linux-x86_64" ;;
  Darwin-x86_64) NDK_HOST="darwin-x86_64" ;;
  Darwin-arm64) NDK_HOST="darwin-arm64" ;;
  *) echo "ERROR: unsupported NDK host platform for archive verification" >&2; exit 2 ;;
esac
NDK_BIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$NDK_HOST/bin"
bash "$ROOT/tools/verify_android_liblzma.sh" "$ANDROID_LIBLZMA_ROOT" \
  "$NDK_BIN/llvm-readelf" "$NDK_BIN/llvm-ar"
echo "PASS: Android ARM64 liblzma installed and architecture-verified"
echo "Header: $HEADER"
echo "Static library: $LIBRARY"
