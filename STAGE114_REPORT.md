# Stage 114 report — GitHub repository bootstrap

## Changes
- Added `tools/publish_github_repository.sh`.
- Creates a private repository by default via GitHub CLI; public visibility requires explicit `REPO_VISIBILITY=public`.
- Derives repository owner from the authenticated GitHub CLI account.
- Initializes Git when absent, commits source if needed, and pushes using `gh repo create --source --remote --push`.
- Refuses to repoint an existing `origin` remote.
- Scans staged paths and blocks original game assets, APK/AAB, native binaries, object files, archives, and build outputs.
- Updated Android versionName and workflow artifact label to Stage 114.
- Added tests for private-by-default behavior, auth requirement, origin protection, and proprietary asset protection.

## Validation
- Python test suite: **79 passed**.
- `bash tools/run_tests.sh`: passed, including native C syntax checks and historical C regression suites through Stage 99.
- Host CMake/liblzma smoke: six decoder scenarios passed.
- All shell scripts: syntax checks passed.
- GitHub Actions YAML parsed successfully.
- GitHub publisher was not executed; this environment has no authenticated `gh` session.

## Limitations
The GitHub connector available in this session does not expose repository creation. The publisher script has not been run because that requires the user's authenticated GitHub CLI session and repository-creation permission. No remote workflow was dispatched. No Android ARM64 APK or playable game is claimed.
