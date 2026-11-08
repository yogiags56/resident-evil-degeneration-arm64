#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${RE_HOST_LZMA_BUILD_DIR:-$ROOT/build/host-liblzma-cmake-smoke}"
cmake -S "$ROOT/tests/cmake_liblzma_smoke" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel
"$BUILD_DIR/test_re_dtrz_lzma"
echo "PASS: CMake FindLibLZMA imported target + production decoder"
