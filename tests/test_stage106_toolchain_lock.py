import json
import pathlib
import tempfile
import subprocess
import os
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class AndroidToolchainLockTests(unittest.TestCase):
    def setUp(self):
        self.lock = json.loads((ROOT / "config/android-toolchain.lock.json").read_text())

    def test_android_versions_are_pinned_consistently(self):
        android = self.lock["android"]
        app = (ROOT / "app/build.gradle").read_text()
        self.assertEqual(android["ndk_version"], "30.0.16248370")
        self.assertIn(f"ndkVersion '{android['ndk_version']}'", app)
        self.assertIn(f"buildToolsVersion '{android['build_tools']}'", app)
        self.assertEqual(android["abi"], "arm64-v8a")
        self.assertEqual(android["min_api"], 23)

    def test_gradle_agp_compatibility_and_checksum_are_pinned(self):
        gradle = self.lock["gradle"]
        self.assertEqual(gradle["agp"], "8.7.3")
        self.assertEqual(gradle["distribution"], "8.9")
        self.assertEqual(
            gradle["distribution_sha256"],
            "d725d707bfabd4dfdc958c624003b3c80accc03f7037b5122c4b1d0ef15cecab",
        )
        props = (ROOT / "gradle/wrapper/gradle-wrapper.properties").read_text()
        self.assertIn("gradle-8.9-bin.zip", props)
        self.assertIn(gradle["distribution_sha256"], props)


    def test_cmake_presets_schema_is_supported_by_pinned_cmake(self):
        presets = json.loads((ROOT / "CMakePresets.json").read_text())
        self.assertLessEqual(presets["version"], 3)
        self.assertEqual(self.lock["android"]["cmake"], "3.22.1")
        self.assertEqual(presets["configurePresets"][0]["name"], "android-arm64")

    def test_xz_build_is_pinned_to_security_fixed_release(self):
        xz = self.lock["xz"]
        self.assertEqual(xz["version"], "5.8.4")
        script = (ROOT / "tools/build_liblzma_android_arm64.sh").read_text()
        self.assertIn('EXPECTED_XZ_VERSION="5.8.4"', script)
        self.assertIn("src/liblzma/api/lzma/version.h", script)
        self.assertIn("-DXZ_TOOL_XZDEC=OFF", script)
        self.assertIn("-DXZ_NLS=OFF", script)
        self.assertIn('NDK_VERSION" != "30.0.16248370"', script)

    def test_doctor_checks_sdk_components_and_pinned_ndk(self):
        doctor = (ROOT / "tools/android_toolchain_doctor.sh").read_text()
        for item in (
            "30.0.16248370", "platforms/android-35/android.jar",
            "build-tools/35.0.0/aapt2", "cmake/3.22.1/bin/cmake",
            "gradle-8.9-bin.zip", "JDK 17+",
        ):
            self.assertIn(item, doctor)


    def test_liblzma_builder_rejects_unpinned_xz_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = pathlib.Path(tmp)
            ndk = base / "ndk"
            (ndk / "build/cmake").mkdir(parents=True)
            (ndk / "build/cmake/android.toolchain.cmake").write_text("# mock")
            (ndk / "source.properties").write_text("Pkg.Revision = 30.0.16248370\n")
            xz = base / "xz"
            xz.mkdir()
            (xz / "CMakeLists.txt").write_text("# mock XZ CMake project\n")
            version_dir = xz / "src/liblzma/api/lzma"
            version_dir.mkdir(parents=True)
            (version_dir / "version.h").write_text(
                "#define LZMA_VERSION_MAJOR 5\n"
                "#define LZMA_VERSION_MINOR 8\n"
                "#define LZMA_VERSION_PATCH 3\n"
            )
            env = os.environ.copy()
            env.update({
                "ANDROID_NDK_HOME": str(ndk),
                "ANDROID_LIBLZMA_ROOT": str(base / "prefix"),
                "XZ_SOURCE_DIR": str(xz),
            })
            result = subprocess.run(
                ["bash", str(ROOT / "tools/build_liblzma_android_arm64.sh")],
                cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("expected XZ Utils 5.8.4; found 5.8.3", result.stdout)


    def test_xz_archive_checksum_is_pinned_to_official_release(self):
        xz = self.lock["xz"]
        self.assertEqual(xz["archive"], "xz-5.8.4.tar.gz")
        self.assertEqual(
            xz["sha256"],
            "0014c7886930454fe8bd4228665b51af55eeae560ea135c9c4cd33f55b2591d9",
        )
        fetch = (ROOT / "tools/fetch_xz_source.sh").read_text()
        self.assertIn("verify_xz_source_archive.py", fetch)
        verifier = (ROOT / "tools/verify_xz_source_archive.py").read_text()
        self.assertIn(xz["sha256"], (ROOT / "config/android-toolchain.lock.json").read_text())
        self.assertIn("unsafe path", verifier)
        self.assertIn("unsafe link", verifier)
        self.assertIn("hashlib.sha256", verifier)

    def test_sdk_installer_uses_all_pinned_sdk_packages(self):
        script = (ROOT / "tools/install_android_sdk_packages.sh").read_text()
        for package in (
            "platform-tools", "platforms/android-35", "build-tools@35.0.0",
            "cmake@3.22.1", "ndk@30.0.16248370",
        ):
            self.assertIn(package, script)
        self.assertIn("ANDROID_CLI", script)
        self.assertIn("deprecated sdkmanager compatibility path", script)

    def test_sdk_installer_fails_clearly_without_sdkmanager(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = os.environ.copy()
            env["ANDROID_SDK_ROOT"] = tmp
            env.pop("ANDROID_SDKMANAGER", None)
            result = subprocess.run(
                ["bash", str(ROOT / "tools/install_android_sdk_packages.sh")],
                cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn("Android CLI not found", result.stdout)


    def test_wrapper_provisioner_pins_checksum_and_version(self):
        script = (ROOT / "tools/provision_gradle_wrapper.sh").read_text()
        build = (ROOT / "tools/build_android_arm64.sh").read_text()
        self.assertIn("android_toolchain_doctor.sh", build)
        self.assertIn('EXPECTED="8.9"', script)
        self.assertIn(self.lock["gradle"]["distribution_sha256"], script)
        self.assertIn("gradle-wrapper.jar", script)


if __name__ == "__main__":
    unittest.main()

