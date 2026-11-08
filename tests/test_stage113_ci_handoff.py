from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Stage113CIHandoffTests(unittest.TestCase):
    def test_sdk_installer_persists_verified_ndk_for_later_github_steps(self):
        script = (ROOT / "tools/install_android_sdk_packages.sh").read_text()
        self.assertIn('if [[ -n "${GITHUB_ENV:-}" ]]', script)
        self.assertIn("ANDROID_NDK_HOME=%s", script)
        self.assertIn("ANDROID_NDK_ROOT=%s", script)
        self.assertIn('export ANDROID_NDK_HOME="$NDK"', script)

    def test_workflow_checks_persisted_ndk_before_gradle_and_native_build(self):
        workflow = (ROOT / ".github/workflows/android-arm64.yml").read_text()
        install = workflow.index("tools/install_android_sdk_packages.sh")
        handoff = workflow.index("Verify NDK environment handoff")
        gradle = workflow.index("tools/install_gradle_distribution.sh")
        native = workflow.index("tools/build_liblzma_android_arm64.sh")
        self.assertLess(install, handoff)
        self.assertLess(handoff, gradle)
        self.assertLess(gradle, native)
        self.assertIn('test -n "${ANDROID_NDK_HOME:-}"', workflow)
        self.assertIn("Pkg.Revision = 30.0.16248370", workflow)

    def test_artifact_collector_exports_packaged_and_unstripped_native_files(self):
        collector = (ROOT / "tools/collect_android_build_artifacts.py").read_text()
        self.assertIn("packaged_native_bytes", collector)
        self.assertIn("is_elf64_aarch64", collector)
        self.assertIn("libre_asset_probe-unstripped.so", collector)
        self.assertIn("libre_asset_probe.so", collector)


if __name__ == "__main__":
    unittest.main()
