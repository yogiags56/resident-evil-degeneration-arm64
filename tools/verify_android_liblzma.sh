#!/usr/bin/env bash
set -euo pipefail
if [[ $# -lt 1 || $# -gt 3 ]]; then
  echo "Usage: $0 <android-liblzma-prefix> [llvm-readelf] [llvm-ar]" >&2
  exit 64
fi
PREFIX="$1"
READELF="${2:-}"
AR="${3:-}"
LIB="$(find "$PREFIX" -type f -name liblzma.a -print -quit)"
HEADER="$(find "$PREFIX" -type f -name lzma.h -print -quit)"
[[ -n "$LIB" && -n "$HEADER" ]] || {
  echo "ERROR: expected liblzma.a and lzma.h under $PREFIX" >&2; exit 2;
}
if [[ -z "$AR" ]]; then AR="$(command -v llvm-ar || command -v ar || true)"; fi
if [[ -z "$READELF" ]]; then READELF="$(command -v llvm-readelf || command -v readelf || true)"; fi
[[ -n "$AR" && -x "$AR" ]] || { echo "ERROR: llvm-ar/ar unavailable" >&2; exit 2; }
[[ -n "$READELF" && -x "$READELF" ]] || { echo "ERROR: llvm-readelf/readelf unavailable" >&2; exit 2; }

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mapfile -t MEMBERS < <("$AR" t "$LIB")
if ((${#MEMBERS[@]} == 0)); then
  echo "ERROR: liblzma archive is empty" >&2
  exit 1
fi

verified=0
for index in "${!MEMBERS[@]}"; do
  member="${MEMBERS[$index]}"
  [[ -n "$member" ]] || continue
  object="$TMP/member-$index.o"
  if ! "$AR" p "$LIB" "$member" > "$object"; then
    echo "ERROR: could not extract archive member: $member" >&2
    exit 1
  fi
  if [[ ! -s "$object" ]]; then
    echo "ERROR: archive member is empty: $member" >&2
    exit 1
  fi
  HEADER_TEXT="$("$READELF" -h "$object")"
  if ! grep -Eq 'Class:[[:space:]]+ELF64' <<<"$HEADER_TEXT"; then
    echo "ERROR: archive member is not ELF64: $member" >&2
    exit 1
  fi
  if ! grep -Eq 'Machine:[[:space:]]+AArch64' <<<"$HEADER_TEXT"; then
    echo "ERROR: archive member is not AArch64: $member" >&2
    exit 1
  fi
  verified=$((verified + 1))
done
if (( verified == 0 )); then
  echo "ERROR: no object members were verified in liblzma archive" >&2
  exit 1
fi
echo "PASS: all $verified liblzma archive members are ELF64 AArch64 objects"
echo "Archive: $LIB"
