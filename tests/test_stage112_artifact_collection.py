import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools/collect_android_build_artifacts.py"


class Stage112ArtifactCollectionTests(unittest.TestCase):
    def make_tree(self, root, include_proprietary=False):
        elf = bytearray(64)
        elf[:4] = b"\x7fELF"
        elf[4] = 2  # ELFCLASS64
        elf[5] = 1  # little-endian
        elf[18:20] = (183).to_bytes(2, "little")  # EM_AARCH64
        apk = root / "app/build/outputs/apk/debug/app-debug.apk"
        apk.parent.mkdir(parents=True)
        with zipfile.ZipFile(apk, "w") as package:
            package.writestr("AndroidManifest.xml", b"manifest")
            package.writestr("lib/arm64-v8a/libre_asset_probe.so", bytes(elf))
            if include_proprietary:
                package.writestr("assets/main.dz", b"private")
        native = root / "app/build/intermediates/cxx/RelWithDebInfo/obj/arm64-v8a/libre_asset_probe.so"
        native.parent.mkdir(parents=True)
        native.write_bytes(bytes(elf))
        archive = root / ".deps/install/liblzma-android-arm64/lib/liblzma.a"
        archive.parent.mkdir(parents=True)
        archive.write_bytes(b"archive")

    def run_collector(self, root, output, extra_env=None):
        env = os.environ.copy()
        if extra_env:
            env.update(extra_env)
        return subprocess.run(
            [sys.executable, str(SCRIPT), "--root", str(root), "--output", str(output)],
            text=True, capture_output=True, env=env,
        )

    def test_collects_files_and_writes_hash_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "repo"
            root.mkdir()
            self.make_tree(root)
            output = Path(temp) / "dist"
            result = self.run_collector(root, output)
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads((output / "BUILD_ARTIFACTS.json").read_text())
            self.assertFalse(manifest["game_ready"])
            self.assertFalse(manifest["proprietary_game_assets_included"])
            self.assertEqual(len(manifest["files"]), 4)
            self.assertTrue((output / "SHA256SUMS.txt").is_file())

    def test_pull_request_manifest_uses_event_head_sha_with_shallow_checkout(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "repo"
            root.mkdir()
            self.make_tree(root)
            event = Path(temp) / "event.json"
            head_sha = "1dbaa7646866c8b0b21acbaf6d98ec2bd7c87cf2"
            event.write_text(json.dumps({
                "pull_request": {"head": {"sha": head_sha}}
            }))
            output = Path(temp) / "dist"
            result = self.run_collector(root, output, {
                "GITHUB_EVENT_NAME": "pull_request",
                "GITHUB_EVENT_PATH": str(event),
                "GITHUB_SHA": "925385c26f22149ce910b55d7685a63e6175046f",
            })
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads((output / "BUILD_ARTIFACTS.json").read_text())
            self.assertEqual(manifest["source_commit"], head_sha)

    def test_rejects_proprietary_game_assets_in_ci_apk(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "repo"
            root.mkdir()
            self.make_tree(root, include_proprietary=True)
            result = self.run_collector(root, Path(temp) / "dist")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("proprietary game assets", result.stderr)

    def test_accepts_stripped_standalone_native_library_that_differs_from_apk_copy(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "repo"
            root.mkdir()
            self.make_tree(root)
            native = root / "app/build/intermediates/cxx/RelWithDebInfo/obj/arm64-v8a/libre_asset_probe.so"
            native.write_bytes(b"different native payload")
            result = self.run_collector(root, Path(temp) / "dist")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((Path(temp) / "dist/libre_asset_probe.so").is_file())
            self.assertTrue((Path(temp) / "dist/libre_asset_probe-unstripped.so").is_file())

    def test_rejects_missing_arm64_library_inside_apk(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "repo"
            root.mkdir()
            self.make_tree(root)
            apk = root / "app/build/outputs/apk/debug/app-debug.apk"
            with zipfile.ZipFile(apk, "w") as package:
                package.writestr("AndroidManifest.xml", b"manifest")
            result = self.run_collector(root, Path(temp) / "dist")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("does not package", result.stderr)


if __name__ == "__main__":
    unittest.main()
