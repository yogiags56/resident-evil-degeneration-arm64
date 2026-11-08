import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class AndroidSdkInstallVerifierTests(unittest.TestCase):
    def test_verifier_checks_all_locked_sdk_packages(self):
        script = (ROOT / "tools/verify_android_sdk_install.sh").read_text()
        for token in (
            "platforms/android-35/android.jar",
            "build-tools/35.0.0/aapt2",
            "build-tools/35.0.0/zipalign",
            "cmake/3.22.1/bin/cmake",
            "platform-tools/adb",
            "30.0.16248370",
            "llvm-readelf",
            "android.toolchain.cmake",
        ):
            self.assertIn(token, script)

    def test_installer_runs_post_install_verification(self):
        script = (ROOT / "tools/install_android_sdk_packages.sh").read_text()
        self.assertIn("verify_android_sdk_install.sh", script)
        self.assertIn("ndk;30.0.16248370", script)
        self.assertIn("sdk install", script)
        self.assertIn("deprecated sdkmanager compatibility path", script)

    def test_doctor_reports_consolidated_sdk_integrity(self):
        script = (ROOT / "tools/android_toolchain_doctor.sh").read_text()
        self.assertIn("sdk_install_integrity", script)
        self.assertIn("verify_android_sdk_install.sh", script)

if __name__ == "__main__":
    unittest.main()
