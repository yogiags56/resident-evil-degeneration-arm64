#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOCK="$ROOT/config/android-toolchain.lock.json"
CACHE="${GRADLE_CACHE_DIR:-$ROOT/.deps/cache}"
INSTALL_ROOT="${GRADLE_INSTALL_ROOT:-$ROOT/.deps/tools}"
VERSION="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["gradle"]["distribution"])' "$LOCK")"
SHA256="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["gradle"]["distribution_sha256"])' "$LOCK")"
ARCHIVE="$CACHE/gradle-$VERSION-bin.zip"
DEST="$INSTALL_ROOT/gradle-$VERSION"
mkdir -p "$CACHE" "$INSTALL_ROOT"

if [[ ! -f "$ARCHIVE" ]]; then
  command -v curl >/dev/null 2>&1 || { echo "ERROR: curl is required to download Gradle" >&2; exit 2; }
  TMP="$ARCHIVE.part"
  rm -f "$TMP"
  curl --fail --location --retry 2 --connect-timeout 20 \
    "https://services.gradle.org/distributions/gradle-$VERSION-bin.zip" -o "$TMP"
  mv "$TMP" "$ARCHIVE"
fi

python3 "$ROOT/tools/verify_gradle_distribution.py" \
  "$ARCHIVE" "$DEST" --expected-sha256 "$SHA256"
echo "$DEST"
