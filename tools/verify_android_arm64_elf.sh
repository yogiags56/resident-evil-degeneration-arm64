#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 <libre_asset_probe.so> [llvm-readelf]" >&2
  exit 64
fi
LIB="$1"
READELF="${2:-}"
if [[ ! -f "$LIB" ]]; then
  echo "ERROR: shared library not found: $LIB" >&2
  exit 2
fi
if [[ -z "$READELF" ]]; then
  if [[ -n "${ANDROID_NDK_HOME:-}" ]]; then
    HOST_TAG=""
    case "$(uname -s)-$(uname -m)" in
      Linux-x86_64) HOST_TAG="linux-x86_64" ;;
      Darwin-x86_64) HOST_TAG="darwin-x86_64" ;;
      Darwin-arm64) HOST_TAG="darwin-arm64" ;;
      *) echo "ERROR: unsupported NDK host platform; pass llvm-readelf explicitly" >&2; exit 2 ;;
    esac
    READELF="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG/bin/llvm-readelf"
  else
    READELF="$(command -v llvm-readelf || command -v readelf || true)"
  fi
fi
if [[ -z "$READELF" || ! -x "$READELF" ]]; then
  echo "ERROR: llvm-readelf/readelf not found; install NDK or pass its llvm-readelf path" >&2
  exit 2
fi

HEADER="$("$READELF" -h "$LIB")"
grep -Eq 'Class:[[:space:]]+ELF64' <<<"$HEADER" || {
  echo "ERROR: expected ELF64 library" >&2; exit 1;
}
grep -Eq 'Machine:[[:space:]]+AArch64' <<<"$HEADER" || {
  echo "ERROR: expected AArch64 machine type" >&2; exit 1;
}
SYMBOLS="$("$READELF" --dyn-syms --wide "$LIB")"
grep -q 'Java_org_example_redstage_MainActivity_nativeProbe' <<<"$SYMBOLS" || {
  echo "ERROR: JNI nativeProbe export not found in dynamic symbol table" >&2; exit 1;
}
echo "PASS: ELF64"
echo "PASS: AArch64"
echo "PASS: JNI nativeProbe export"
