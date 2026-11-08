import json
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from stage_android_assets import REQUIRED_ASSETS, stage


class StageAndroidAssetsTests(unittest.TestCase):
    def make_apk(self, path, entries=None):
        entries = entries or {
            "assets/main.dz": b"DTRZsynthetic archive",
            "assets/biohazard.s3e": b"XE3U-like synthetic image",
            "assets/s3e.icf": b"# synthetic config",
        }
        with zipfile.ZipFile(path, "w") as archive:
            for name, data in entries.items():
                archive.writestr(name, data)

    def test_stages_allowlisted_assets_and_hash_manifest(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            apk = root / "input.apk"
            output = root / "assets"
            self.make_apk(apk)
            manifest = stage(apk, output)
            self.assertEqual(len(manifest["assets"]), 3)
            self.assertEqual((output / "main.dz").read_bytes(), b"DTRZsynthetic archive")
            saved = json.loads((output / "local-assets-manifest.json").read_text())
            self.assertEqual(saved["schema"], 1)
            self.assertIn("Do not commit", saved["notice"])

    def test_rejects_missing_asset(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            apk = root / "input.apk"
            self.make_apk(apk, {"assets/main.dz": b"DTRZ"})
            with self.assertRaisesRegex(ValueError, "biohazard.s3e"):
                stage(apk, root / "assets")

    def test_rejects_bad_dtrz_magic(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            apk = root / "input.apk"
            self.make_apk(apk, {
                "assets/main.dz": b"NOPE",
                "assets/biohazard.s3e": b"image",
                "assets/s3e.icf": b"config",
            })
            with self.assertRaisesRegex(ValueError, "DTRZ magic"):
                stage(apk, root / "assets")

    def test_refuses_overwrite_without_force(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            apk = root / "input.apk"
            output = root / "assets"
            self.make_apk(apk)
            stage(apk, output)
            with self.assertRaises(FileExistsError):
                stage(apk, output)

    def test_rejects_duplicate_asset_entries(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            apk = root / "input.apk"
            with zipfile.ZipFile(apk, "w") as archive:
                archive.writestr("assets/main.dz", b"DTRZ")
                archive.writestr("assets/main.dz", b"DTRZ duplicate")
                archive.writestr("assets/biohazard.s3e", b"image")
                archive.writestr("assets/s3e.icf", b"config")
            with self.assertRaisesRegex(ValueError, "exactly one"):
                stage(apk, root / "assets")


if __name__ == "__main__":
    unittest.main()
