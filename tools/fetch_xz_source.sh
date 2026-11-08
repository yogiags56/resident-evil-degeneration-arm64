#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CACHE="${XZ_CACHE_DIR:-$ROOT/.deps/cache}"
SOURCE_ROOT="${XZ_SOURCE_ROOT:-$ROOT/.deps/src}"
ARCHIVE="xz-5.8.4.tar.gz"
DEST="$SOURCE_ROOT/xz-5.8.4"
mkdir -p "$CACHE" "$SOURCE_ROOT"
if [[ -f "$DEST/src/liblzma/api/lzma/version.h" ]]; then
  VERSION="$(awk '
    /^#define LZMA_VERSION_MAJOR / { a=$3 }
    /^#define LZMA_VERSION_MINOR / { b=$3 }
    /^#define LZMA_VERSION_PATCH / { c=$3 }
    END { if (a != "" && b != "" && c != "") print a "." b "." c }
  ' "$DEST/src/liblzma/api/lzma/version.h")"
  if [[ "$VERSION" == "5.8.4" ]]; then
    echo "PASS: XZ Utils 5.8.4 source already present: $DEST"
    echo "$DEST"
    exit 0
  fi
  echo "ERROR: existing XZ source directory has version ${VERSION:-unknown}" >&2
  exit 2
fi
ARCHIVE_PATH="$CACHE/$ARCHIVE"
if [[ ! -f "$ARCHIVE_PATH" ]]; then
  if ! command -v curl >/dev/null 2>&1; then
    echo "ERROR: curl is required to fetch XZ Utils 5.8.4" >&2
    exit 2
  fi
  URLS=(
    "https://github.com/tukaani-project/xz/releases/download/v5.8.4/$ARCHIVE"
    "https://downloads.sourceforge.net/project/lzmautils/$ARCHIVE"
  )
  TMP="$ARCHIVE_PATH.part"
  rm -f "$TMP"
  fetched=0
  for url in "${URLS[@]}"; do
    echo "Downloading $url"
    if curl --fail --location --retry 2 --connect-timeout 20 --output "$TMP" "$url"; then
      mv "$TMP" "$ARCHIVE_PATH"
      fetched=1
      break
    fi
    rm -f "$TMP"
  done
  if [[ "$fetched" != 1 ]]; then
    echo "ERROR: could not download XZ Utils 5.8.4 from official release mirrors" >&2
    exit 1
  fi
fi
python3 "$ROOT/tools/verify_xz_source_archive.py" "$ARCHIVE_PATH"
TMPDIR="$SOURCE_ROOT/.xz-5.8.4.extract.$$"
rm -rf "$TMPDIR"
mkdir -p "$TMPDIR"
trap 'rm -rf "$TMPDIR"' EXIT
tar -xzf "$ARCHIVE_PATH" -C "$TMPDIR" --strip-components=1
VERSION_HEADER="$TMPDIR/src/liblzma/api/lzma/version.h"
if [[ ! -f "$VERSION_HEADER" ]]; then
  echo "ERROR: extracted source lacks liblzma version header" >&2
  exit 1
fi
VERSION="$(awk '
  /^#define LZMA_VERSION_MAJOR / { a=$3 }
  /^#define LZMA_VERSION_MINOR / { b=$3 }
  /^#define LZMA_VERSION_PATCH / { c=$3 }
  END { if (a != "" && b != "" && c != "") print a "." b "." c }
' "$VERSION_HEADER")"
if [[ "$VERSION" != "5.8.4" ]]; then
  echo "ERROR: expected XZ Utils 5.8.4; extracted ${VERSION:-unknown}" >&2
  exit 1
fi
mv "$TMPDIR" "$DEST"
trap - EXIT
echo "PASS: XZ Utils $VERSION extracted and SHA-256 verified: $DEST"
echo "$DEST"
