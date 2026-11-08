import os
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
REPO = "yogiags56/resident-evil-degeneration-arm64"


class AndroidSdkManagerBootstrapTests(unittest.TestCase):
    def test_installer_uses_official_pinned_command_line_tools_archive(self):
        script = (ROOT / "tools/install_android_sdk_packages.sh").read_text()
        self.assertIn('BUILD=15859902', script)
        self.assertIn('ARCHIVE="commandlinetools-linux-${BUILD}_latest.zip"', script)
        self.assertIn(
            "4e4c464f145a7512b57d088ac6c278c03c9eea610886b35a5e0804e74eedf583",
            script,
        )
        self.assertIn('sha256sum --check', script)
        self.assertIn('cmdline-tools/latest/bin/sdkmanager', script)

    def test_installer_uses_sdkmanager_with_sdk_style_package_ids(self):
        script = (ROOT / "tools/install_android_sdk_packages.sh").read_text()
        self.assertIn('"build-tools;35.0.0"', script)
        self.assertIn('"cmake;3.22.1"', script)
        self.assertIn('"ndk;30.0.16248370"', script)
        self.assertNotIn('"$ANDROID_CLI" --sdk=', script)

    def test_installer_runs_sdkmanager_and_verifies_generated_sdk_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = pathlib.Path(tmp)
            sdk = base / "sdk"
            fake = base / "sdkmanager"
            fake.write_text("""#!/usr/bin/env bash
set -euo pipefail
printf '%s\\n' "$*" >> "$CALL_LOG"
for arg in "$@"; do
  if [[ "$arg" == "--install" ]]; then
    mkdir -p "$SDK/platforms/android-35" "$SDK/build-tools/35.0.0" \\
      "$SDK/cmake/3.22.1/bin" "$SDK/platform-tools" \\
      "$SDK/ndk/30.0.16248370/toolchains/llvm/prebuilt/linux-x86_64/bin" \\
      "$SDK/ndk/30.0.16248370/build/cmake"
    touch "$SDK/platforms/android-35/android.jar"
    for tool in "$SDK/build-tools/35.0.0/aapt2" "$SDK/build-tools/35.0.0/zipalign" \\
      "$SDK/cmake/3.22.1/bin/cmake" "$SDK/platform-tools/adb" \\
      "$SDK/ndk/30.0.16248370/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" \\
      "$SDK/ndk/30.0.16248370/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar" \\
      "$SDK/ndk/30.0.16248370/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf"; do
      touch "$tool"; chmod +x "$tool"
    done
    echo 'Pkg.Revision = 30.0.16248370' > "$SDK/ndk/30.0.16248370/source.properties"
    touch "$SDK/ndk/30.0.16248370/build/cmake/android.toolchain.cmake"
  fi
done
""")
            fake.chmod(0o755)
            log = base / "calls.log"
            github_env = base / "github_env"
            github_env.write_text("")
            env = os.environ.copy()
            env.update({
                "ANDROID_SDK_ROOT": str(sdk),
                "ANDROID_SDKMANAGER": str(fake),
                "CALL_LOG": str(log),
                "SDK": str(sdk),
                "GITHUB_ENV": str(github_env),
            })
            result = subprocess.run(
                ["bash", str(ROOT / "tools/install_android_sdk_packages.sh")],
                cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, timeout=20,
            )
            self.assertEqual(result.returncode, 0, result.stdout)
            calls = log.read_text()
            self.assertIn("--licenses", calls)
            self.assertIn("build-tools;35.0.0", calls)
            self.assertIn("cmake;3.22.1", calls)
            self.assertIn("ndk;30.0.16248370", calls)
            self.assertIn("all pinned Android SDK/NDK components verified", result.stdout)
            expected = str(sdk / "ndk/30.0.16248370")
            self.assertIn(f"ANDROID_NDK_HOME={expected}\n", github_env.read_text())
            self.assertIn(f"ANDROID_NDK_ROOT={expected}\n", github_env.read_text())


if __name__ == "__main__":
    unittest.main()
