import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
class AndroidArm64WorkflowTests(unittest.TestCase):
    def test_workflow_has_full_provision_build_order(self):
        workflow=(ROOT/".github/workflows/android-arm64.yml").read_text()
        required=[
            "actions/checkout@v4",
            "actions/setup-java@v4",
            "Install official Android CLI",
            "tools/install_android_sdk_packages.sh",
            "tools/install_gradle_distribution.sh",
            "tools/provision_gradle_wrapper.sh",
            "tools/fetch_xz_source.sh",
            "tools/build_liblzma_android_arm64.sh",
            "tools/build_android_arm64.sh",
            "tools/collect_android_build_artifacts.py",
            "actions/upload-artifact@v4",
        ]
        positions=[workflow.index(x) for x in required]
        self.assertEqual(positions,sorted(positions))
        self.assertIn("ubuntu-24.04",workflow)
        self.assertIn("timeout-minutes: 60",workflow)
        self.assertIn("retention-days: 14", workflow)
        self.assertIn("compression-level: 0", workflow)
        self.assertIn("dist/android-arm64/", workflow)

    def test_workflow_does_not_embed_or_upload_original_apk_assets(self):
        workflow=(ROOT/".github/workflows/android-arm64.yml").read_text()
        self.assertNotIn("*.apk",workflow.split("Upload build artifacts")[0])
        self.assertNotIn("main.dz",workflow)
        self.assertNotIn("biohazard.s3e",workflow)

if __name__=="__main__":
    unittest.main()
