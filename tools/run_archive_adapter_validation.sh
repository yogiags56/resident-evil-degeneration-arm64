#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo "usage: $0 /path/to/main.dz" >&2
  exit 2
fi
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
if ! pkg-config --exists liblzma; then
  echo "liblzma development package and pkg-config are required" >&2
  exit 2
fi
mkdir -p build/host-tests
CC="${CC:-clang}"
"$CC" -std=c11 -Wall -Wextra -Werror \
  -Iruntime/archive -Iapp/src/main/cpp -Iruntime/guest \
  tools/dtrz_adapter_verify.c runtime/archive/re_dtrz_vfs.c \
  runtime/archive/re_dtrz_lzma.c runtime/archive/re_dtrz_file_backend.c \
  app/src/main/cpp/re_file_adapter.c \
  $(pkg-config --cflags --libs liblzma) \
  -o build/host-tests/dtrz_adapter_verify
build/host-tests/dtrz_adapter_verify "$1"
