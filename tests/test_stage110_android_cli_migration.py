import pathlib
import os
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class AndroidCliMigrationTests(unittest.TestCase):
    def test_sdk_installer_prefers_android_cli_and_pins_package_versions(self):
        script=(ROOT/"tools/install_android_sdk_packages.sh").read_text()
        self.assertLess(script.index('"$ANDROID_CLI" --sdk="$SDK" sdk install'),
                        script.index('deprecated sdkmanager compatibility path'))
        self.assertIn("platforms/android-35",script)
        self.assertIn("build-tools/35.0.0",script)
        self.assertIn("cmake/3.22.1",script)
        self.assertIn("ndk/30.0.16248370",script)

    def test_doctor_accepts_android_cli_or_legacy_fallback(self):
        script=(ROOT/"tools/android_toolchain_doctor.sh").read_text()
        self.assertIn('command -v android',script)
        self.assertIn("legacy sdkmanager fallback",script)
        self.assertIn("android_cli",script)

    def test_official_android_cli_install_reference_is_documented(self):
        readme=(ROOT/"README.md").read_text()
        self.assertIn("developer.android.com/tools/agents/android-cli/download",readme)

    def test_installer_executes_android_cli_and_verifies_generated_sdk_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=pathlib.Path(tmp)
            sdk=base/"sdk"
            fake=base/"android"
            fake.write_text("""#!/usr/bin/env bash
set -euo pipefail
SDK="${1#--sdk=}"
shift 2
[[ "$1" == install ]]
shift
mkdir -p "$SDK"
printf '%s\\n' "$@" > "$SDK/packages.requested"
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
  touch "$tool"
  chmod +x "$tool"
done
echo 'Pkg.Revision = 30.0.16248370' > "$SDK/ndk/30.0.16248370/source.properties"
touch "$SDK/ndk/30.0.16248370/build/cmake/android.toolchain.cmake"
""")
            fake.chmod(0o755)
            env=os.environ.copy()
            github_env=base/"github_env"
            github_env.write_text("")
            env.update({"ANDROID_SDK_ROOT":str(sdk),"ANDROID_CLI":str(fake),"GITHUB_ENV":str(github_env)})
            env.pop("ANDROID_SDKMANAGER",None)
            result=subprocess.run(
                ["bash",str(ROOT/"tools/install_android_sdk_packages.sh")],
                cwd=ROOT,env=env,text=True,stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,timeout=20)
            self.assertEqual(result.returncode,0,result.stdout)
            requested=(sdk/"packages.requested").read_text().splitlines()
            self.assertEqual(requested,[
                "platform-tools","platforms/android-35","build-tools@35.0.0",
                "cmake@3.22.1","ndk@30.0.16248370"])
            self.assertIn("all pinned Android SDK/NDK components verified",result.stdout)
            persisted=github_env.read_text()
            expected_ndk=str(sdk/"ndk/30.0.16248370")
            self.assertIn(f"ANDROID_NDK_HOME={expected_ndk}\n",persisted)
            self.assertIn(f"ANDROID_NDK_ROOT={expected_ndk}\n",persisted)

if __name__=="__main__":
    unittest.main()
