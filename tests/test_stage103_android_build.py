from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Stage103AndroidBuildTests(unittest.TestCase):
    def read(self, relative):
        return (ROOT / relative).read_text(encoding="utf-8")

    def test_liblzma_cross_build_targets_android_arm64(self):
        script = self.read("tools/build_liblzma_android_arm64.sh")
        for token in (
            "android.toolchain.cmake",
            "ANDROID_ABI=arm64-v8a",
            "ANDROID_PLATFORM=android-23",
            "BUILD_SHARED_LIBS=OFF",
            "CMAKE_POSITION_INDEPENDENT_CODE=ON",
            "liblzma.a",
        ):
            self.assertIn(token, script)

    def test_android_gradle_build_requires_liblzma(self):
        gradle = self.read("app/build.gradle")
        self.assertIn("-DRE_ENABLE_LIBLZMA=ON", gradle)
        self.assertIn("ANDROID_LIBLZMA_ROOT", gradle)
        self.assertIn("abiFilters 'arm64-v8a'", gradle)

    def test_build_script_supports_wrapper_or_system_gradle(self):
        script = self.read("tools/build_android_arm64.sh")
        self.assertIn('"$ROOT/gradlew"', script)
        self.assertIn("command -v gradle", script)
        self.assertIn("provision_gradle_wrapper.sh", script)

    def test_preflight_checks_android_liblzma_files(self):
        script = self.read("tools/android_arm64_preflight.sh")
        self.assertIn("ANDROID_NDK_HOME", script)
        self.assertIn("ANDROID_LIBLZMA_ROOT", script)
        self.assertIn("lzma.h", script)
        self.assertIn("liblzma.a", script)


if __name__ == "__main__":
    unittest.main()
