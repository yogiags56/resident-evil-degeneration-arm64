#!/usr/bin/env bash
# Create a GitHub repository and push this source tree. Private by default.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_NAME="${1:-resident-evil-degeneration-arm64}"
VISIBILITY_FLAG="--private"

if ! command -v git >/dev/null 2>&1; then
  echo "ERROR: git is required" >&2; exit 2
fi
if ! command -v gh >/dev/null 2>&1; then
  echo "ERROR: GitHub CLI (gh) is required; install it and run 'gh auth login'" >&2; exit 2
fi
gh auth status >/dev/null
OWNER="$(gh api user --jq .login)"
if [[ "$REPO_NAME" == */* ]]; then
  TARGET="$REPO_NAME"
else
  TARGET="$OWNER/$REPO_NAME"
fi

cd "$ROOT"
if [[ ! -d .git ]]; then
  git init -b main
fi
if git remote get-url origin >/dev/null 2>&1; then
  echo "ERROR: origin remote already exists; refusing to overwrite or repoint it" >&2
  exit 3
fi

# Use the authenticated GitHub identity only if this repository has no local identity.
if ! git config user.name >/dev/null 2>&1; then
  git config user.name "$OWNER"
fi
if ! git config user.email >/dev/null 2>&1; then
  git config user.email "$OWNER@users.noreply.github.com"
fi

git add -A
# Hard stop if proprietary binaries/assets or build outputs have entered the index.
if git ls-files --cached | grep -Eiq '(^|/)(main\.dz|biohazard\.s3e|s3e\.icf|local-assets-manifest\.json)$|\.(apk|aab|dex|so|o|a|class|jar)$|(^|/)(\.deps|build|\.gradle)/|(^|/)\.env([^/]*)$|\.(jks|keystore|p12|pfx|key)$'; then
  echo "ERROR: staged proprietary asset or generated binary detected; refusing to publish" >&2
  git status --short >&2
  exit 4
fi
if ! git rev-parse --verify HEAD >/dev/null 2>&1; then
  git commit -m "Initial source import — Stage 115"
elif ! git diff --cached --quiet; then
  git commit -m "Publish Stage 115 source and private CI preparation"
fi

git branch -M main
gh repo create "$TARGET" "$VISIBILITY_FLAG" --source "$ROOT" --remote origin --push \
  --description "Native ARM64 research port of Resident Evil: Degeneration; source, tests, and reproducible Android CI." \
  --disable-wiki
printf 'Private repository published: https://github.com/%s\n' "$TARGET"

# Dispatch only after the private repository and main branch have been created.
# If dispatch fails, publication remains valid and can be retried independently.
if gh workflow run android-arm64.yml --repo "$TARGET" --ref main; then
  echo "Android ARM64 workflow dispatch accepted for $TARGET (main)."
else
  echo "WARNING: repository was published, but workflow dispatch failed." >&2
  echo "Retry with: gh workflow run android-arm64.yml --repo $TARGET --ref main" >&2
  exit 5
fi
