#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "$SDK" ]]; then
  echo "ERROR: set ANDROID_SDK_ROOT to the Android SDK directory" >&2
  exit 2
fi

ANDROID_CLI="${ANDROID_CLI:-$(command -v android || true)}"
SDKMANAGER="${ANDROID_SDKMANAGER:-$SDK/cmdline-tools/latest/bin/sdkmanager}"
if [[ -n "$ANDROID_CLI" && -x "$ANDROID_CLI" ]]; then
  echo "Using Android CLI for SDK package management."
  echo "Installing pinned packages using catalog package paths."
  "$ANDROID_CLI" --sdk="$SDK" sdk install \
    platform-tools \
    platforms/android-35 \
    build-tools/35.0.0 \
    cmake/3.22.1 \
    ndk/30.0.16248370
elif [[ -x "$SDKMANAGER" ]] || command -v sdkmanager >/dev/null 2>&1; then
  if [[ -x "$SDKMANAGER" ]]; then
    MANAGER="$SDKMANAGER"
  else
    MANAGER="$(command -v sdkmanager)"
  fi
  echo "WARNING: Android CLI not found; using deprecated sdkmanager compatibility path." >&2
  echo "The Android SDK licenses will be presented now."
  "$MANAGER" --sdk_root="$SDK" --licenses
  "$MANAGER" --sdk_root="$SDK" --install \
    "platform-tools" \
    "platforms;android-35" \
    "build-tools;35.0.0" \
    "cmake;3.22.1" \
    "ndk;30.0.16248370"
else
  echo "ERROR: Android CLI not found. Install it using the official instructions:" >&2
  echo "https://developer.android.com/tools/agents/android-cli/download" >&2
  echo "Legacy sdkmanager is also accepted as a temporary fallback." >&2
  exit 2
fi

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
# GitHub Actions steps run in separate shells. Persist the verified NDK path so
# subsequent build steps receive the exact pinned installation.
if [[ -n "${GITHUB_ENV:-}" ]]; then
  printf 'ANDROID_NDK_HOME=%s\nANDROID_NDK_ROOT=%s\n' "$NDK" "$NDK" >> "$GITHUB_ENV"
fi
export ANDROID_NDK_HOME="$NDK"
export ANDROID_NDK_ROOT="$NDK"
echo "Verified ANDROID_NDK_HOME=$NDK"
