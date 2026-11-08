# Stage 115 report — Private publication and CI dispatch preparation

## User decision
Repository visibility is explicitly PRIVATE. The publisher now enforces `--private`; no environment variable can switch it to public.

## Changes
- Hardened `tools/publish_github_repository.sh` to enforce private visibility.
- Pushes source to `main`, then dispatches `android-arm64.yml` through GitHub CLI.
- Handles dispatch failure separately: publication remains intact and a retry command is printed.
- Disabled GitHub Wiki for a minimal source repository.
- Updated Android versionName and workflow artifact label to Stage 115.
- Added tests for private-only policy, workflow dispatch, authentication, origin protection, and proprietary asset rejection.

## Validation
- Python test suite: **80 passed**.
- `bash tools/run_tests.sh`: passed; native C syntax checks and historical C regression suites through Stage 99 passed.
- Host CMake/liblzma smoke: six decoder scenarios passed.
- All shell scripts passed `bash -n`.
- GitHub Actions YAML parsed; workflow_dispatch and artifact upload steps verified.
- Source ZIP integrity passed; no APK, shared/static libraries, object files, JARs, or proprietary game assets included.

## Execution limitations
The connected GitHub integration has no repository-creation operation, and this runtime has no `gh` executable or authenticated CLI session. Therefore no remote repository was created, no source was pushed, and no workflow was dispatched. The one-command publisher is prepared for execution from an authenticated GitHub CLI environment.

No Android ARM64 APK, device runtime, or playable game is claimed.
