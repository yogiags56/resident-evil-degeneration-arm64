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
  PRIMARY_URL="https://services.gradle.org/distributions/gradle-$VERSION-bin.zip"
  FALLBACK_URL="https://downloads.gradle.org/distributions/gradle-$VERSION-bin.zip"
  echo "Downloading pinned Gradle $VERSION from the primary official distribution endpoint."
  if ! curl --fail --location --retry 2 --retry-all-errors --connect-timeout 20 \
      "$PRIMARY_URL" -o "$TMP"; then
    echo "WARNING: primary Gradle endpoint failed; trying official downloads.gradle.org mirror." >&2
    rm -f "$TMP"
    curl --fail --location --retry 3 --retry-all-errors --connect-timeout 20 \
      "$FALLBACK_URL" -o "$TMP"
  fi
  mv "$TMP" "$ARCHIVE"
fi

python3 "$ROOT/tools/verify_gradle_distribution.py" \
  "$ARCHIVE" "$DEST" --expected-sha256 "$SHA256"
echo "$DEST"
