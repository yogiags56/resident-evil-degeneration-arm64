from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_publish_script_enforces_private_visibility_and_dispatches_stage115():
    script = (ROOT / "tools/publish_github_repository.sh").read_text()
    assert 'VISIBILITY_FLAG="--private"' in script
    assert "REPO_VISIBILITY" not in script
    assert 'gh repo create "$TARGET" "$VISIBILITY_FLAG"' in script
    assert '--source "$ROOT" --remote origin --push' in script
    assert 'gh workflow run android-arm64.yml --repo "$TARGET" --ref main' in script


def test_publish_script_refuses_existing_origin_and_proprietary_assets():
    script = (ROOT / "tools/publish_github_repository.sh").read_text()
    assert 'git remote get-url origin' in script
    assert 'refusing to overwrite or repoint it' in script
    assert 'main\\.dz' in script
    assert 'biohazard\\.s3e' in script
    assert 's3e\\.icf' in script
    assert '\\.(apk|aab|dex|so|o|a|class|jar)' in script
    assert '\\.(jks|keystore|p12|pfx|key)' in script
    assert r'\.env([^/]*)$' in script


def test_publish_script_requires_authenticated_github_cli():
    script = (ROOT / "tools/publish_github_repository.sh").read_text()
    assert "command -v gh" in script
    assert "gh auth status" in script
    assert "gh api user --jq .login" in script


def test_publish_script_keeps_publication_success_if_dispatch_needs_retry():
    script = (ROOT / "tools/publish_github_repository.sh").read_text()
    assert "Private repository published" in script
    assert "repository was published, but workflow dispatch failed" in script
    assert "gh workflow run android-arm64.yml" in script
