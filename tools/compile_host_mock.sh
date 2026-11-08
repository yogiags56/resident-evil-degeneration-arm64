#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/app/src/main/cpp"
OUT="$ROOT/build/host-mock"
mkdir -p "$OUT"
CC="${CC:-clang}"
FLAGS=(-std=c11 -Wall -Wextra -Werror -Wconversion -Wshadow -fsyntax-only
       -I"$ROOT/tests/mock_android" -I"$SRC" -I"$ROOT/runtime/guest" -I"$ROOT")
for f in re_file_adapter.c re_android_asset_backend.c re_android_jni_assets.c \
         native_probe.c re_s3e_file_read_contract.c re_s3e_file_api_audit.c \
         re_s3e_file_bridge.c; do
  echo "CHECK $f"
  "$CC" "${FLAGS[@]}" "$SRC/$f"
done
echo 'PASS: native C translation units pass host mock-header syntax checks.'
