#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXPECTED="8.9"
CHECKSUM="d725d707bfabd4dfdc958c624003b3c80accc03f7037b5122c4b1d0ef15cecab"
if ! command -v gradle >/dev/null 2>&1; then
  echo "ERROR: system Gradle is required to generate the wrapper once." >&2
  exit 2
fi
VERSION="$(gradle --version | sed -n 's/^Gradle //p' | head -n1)"
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+ ]] || ! awk -v v="$VERSION" 'BEGIN {split(v,a,"."); exit !((a[1]>8)||(a[1]==8 && a[2]>=9))}'; then
  echo "ERROR: Gradle 8.9+ required to provision the project wrapper; found ${VERSION:-unknown}" >&2
  exit 2
fi
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
printf "rootProject.name = 'wrapper-bootstrap'\\n" > "$TMP/settings.gradle"
: > "$TMP/build.gradle"
gradle -p "$TMP" --no-daemon wrapper --gradle-version "$EXPECTED" --distribution-type bin
mkdir -p "$ROOT/gradle/wrapper"
cp "$TMP/gradlew" "$ROOT/gradlew"
cp "$TMP/gradlew.bat" "$ROOT/gradlew.bat"
cp "$TMP/gradle/wrapper/gradle-wrapper.jar" "$ROOT/gradle/wrapper/gradle-wrapper.jar"
cat > "$ROOT/gradle/wrapper/gradle-wrapper.properties" <<EOF
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-${EXPECTED}-bin.zip
distributionSha256Sum=${CHECKSUM}
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
EOF
chmod +x "$ROOT/gradlew"
echo "PASS: Gradle ${EXPECTED} wrapper generated from an isolated bootstrap project and SHA-256 pinned"
