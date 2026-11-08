import json
import os
import pathlib
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
DOCTOR = ROOT / "tools" / "android_toolchain_doctor.sh"


class AndroidToolchainDoctorTests(unittest.TestCase):
    def test_doctor_script_has_required_gates(self):
        source = DOCTOR.read_text()
        for token in (
            "ANDROID_NDK_HOME", "ANDROID_NDK_ROOT", "source.properties",
            "android.toolchain.cmake", "llvm-readelf", "llvm-ar",
            "ANDROID_LIBLZMA_ROOT", "verify_android_liblzma.sh",
            "gradlew", "cmake", "ninja", "java", "android_cli", "sdkmanager",
        ):
            self.assertIn(token, source)

    def test_json_report_is_machine_readable_and_reports_blocked(self):
        env = os.environ.copy()
        for key in ("ANDROID_NDK_HOME", "ANDROID_NDK_ROOT", "ANDROID_LIBLZMA_ROOT"):
            env.pop(key, None)
        result = subprocess.run(
            ["bash", str(DOCTOR), "--json"], cwd=ROOT, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )
        self.assertNotEqual(result.returncode, 0)
        report = json.loads(result.stdout)
        self.assertEqual(report["overall"], "BLOCKED")
        names = {item["name"] for item in report["checks"]}
        self.assertTrue({"ndk_root", "ndk_cmake_toolchain", "liblzma_files", "gradle"} <= names)

    def test_text_report_identifies_missing_ndk(self):
        env = os.environ.copy()
        env.pop("ANDROID_NDK_HOME", None)
        env.pop("ANDROID_NDK_ROOT", None)
        result = subprocess.run(
            ["bash", str(DOCTOR)], cwd=ROOT, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("RESULT: BLOCKED", result.stdout)
        self.assertIn("ANDROID_NDK_HOME/ROOT unset", result.stdout)


if __name__ == "__main__":
    unittest.main()
