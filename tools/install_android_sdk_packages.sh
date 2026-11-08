#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "$SDK" ]]; then
  echo "ERROR: set ANDROID_SDK_ROOT to the Android SDK directory" >&2
  exit 2
fi
mkdir -p "$SDK"

# Android CLI currently omits URLs for some SDK package families (notably
# Build Tools, CMake and NDK). Bootstrap Google's official command-line tools
# and use sdkmanager's documented SDK-style package identifiers instead.
SDKMANAGER="${ANDROID_SDKMANAGER:-$SDK/cmdline-tools/latest/bin/sdkmanager}"
if [[ ! -x "$SDKMANAGER" ]]; then
  if ! command -v curl >/dev/null 2>&1 || ! command -v unzip >/dev/null 2>&1; then
    echo "ERROR: curl and unzip are required to bootstrap Android cmdline-tools." >&2
    exit 2
  fi
  BUILD=15859902
  ARCHIVE="commandlinetools-linux-${BUILD}_latest.zip"
  URL="https://dl.google.com/android/repository/$ARCHIVE"
  SHA256="4e4c464f145a7512b57d088ac6c278c03c9eea610886b35a5e0804e74eedf583"
  TMP="$(mktemp -d)"
  trap 'rm -rf "$TMP"' EXIT
  echo "Downloading pinned official Android command-line tools ($BUILD)."
  curl --fail --location --retry 3 --output "$TMP/$ARCHIVE" "$URL"
  echo "$SHA256  $TMP/$ARCHIVE" | sha256sum --check -
  unzip -q "$TMP/$ARCHIVE" -d "$TMP/unpacked"
  [[ -f "$TMP/unpacked/cmdline-tools/bin/sdkmanager" ]]
  [[ -f "$TMP/unpacked/cmdline-tools/source.properties" ]]
  mkdir -p "$SDK/cmdline-tools"
  rm -rf "$SDK/cmdline-tools/latest"
  mv "$TMP/unpacked/cmdline-tools" "$SDK/cmdline-tools/latest"
  SDKMANAGER="$SDK/cmdline-tools/latest/bin/sdkmanager"
fi

if [[ ! -x "$SDKMANAGER" ]]; then
  echo "ERROR: sdkmanager is missing or not executable: $SDKMANAGER" >&2
  exit 1
fi

echo "Using official Android SDK sdkmanager: $SDKMANAGER"
# sdkmanager is interactive for licenses; temporarily disable pipefail so
# 'yes' receiving SIGPIPE after the final prompt does not fail the step.
set +o pipefail
yes | "$SDKMANAGER" --sdk_root="$SDK" --licenses
set -o pipefail

"$SDKMANAGER" --sdk_root="$SDK" --install \
  "platform-tools" \
  "platforms;android-35" \
  "build-tools;35.0.0" \
  "cmake;3.22.1" \
  "ndk;30.0.16248370"

NDK="$SDK/ndk/30.0.16248370"
if [[ ! -f "$NDK/source.properties" ]]; then
  echo "ERROR: pinned NDK installation missing source.properties: $NDK" >&2
  exit 1
fi
REV="$(sed -n 's/^Pkg.Revision[[:space:]]*=[[:space:]]*//p' "$NDK/source.properties" | head -n1)"
if [[ "$REV" != "30.0.16248370" ]]; then
  echo "ERROR: expected NDK 30.0.16248370; found ${REV:-unknown}" >&2
  exit 1
fi
bash "$ROOT/tools/verify_android_sdk_install.sh"
if [[ -n "${GITHUB_ENV:-}" ]]; then
  printf 'ANDROID_NDK_HOME=%s\nANDROID_NDK_ROOT=%s\n' "$NDK" "$NDK" >> "$GITHUB_ENV"
fi
export ANDROID_NDK_HOME="$NDK"
export ANDROID_NDK_ROOT="$NDK"
echo "Verified ANDROID_NDK_HOME=$NDK"
