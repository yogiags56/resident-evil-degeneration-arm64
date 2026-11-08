import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
SCRIPT=ROOT/"tools/install_gradle_distribution.sh"
VERIFY=ROOT/"tools/verify_gradle_distribution.py"

class GradleDistributionTests(unittest.TestCase):
    def test_installer_uses_lock_and_verified_official_distribution(self):
        script=SCRIPT.read_text()
        self.assertIn("distribution_sha256",script)
        self.assertIn("services.gradle.org/distributions",script)
        self.assertIn("verify_gradle_distribution.py",script)
        verifier=VERIFY.read_text()
        self.assertIn("hashlib.sha256",verifier)
        self.assertIn('".." in path.parts',verifier)
        self.assertIn("symbolic links are not accepted",verifier)

    def test_lock_contains_expected_gradle_version_and_hash(self):
        lock=json.loads((ROOT/"config/android-toolchain.lock.json").read_text())
        self.assertEqual(lock["gradle"]["distribution"],"8.9")
        self.assertEqual(len(lock["gradle"]["distribution_sha256"]),64)

    def run_verifier(self, make_archive):
        with tempfile.TemporaryDirectory() as tmp:
            base=pathlib.Path(tmp)
            archive=base/"gradle.zip"
            make_archive(archive)
            digest=hashlib.sha256(archive.read_bytes()).hexdigest()
            destination=base/"out"/"gradle-8.9"
            result=subprocess.run(
                [sys.executable,str(VERIFY),str(archive),str(destination),
                 "--expected-sha256",digest],
                text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=10)
            executable=destination/"bin/gradle"
            is_executable=executable.is_file() and bool(executable.stat().st_mode & 0o111)
            return result,is_executable

    def test_valid_distribution_extracts_and_marks_gradle_executable(self):
        def create(path):
            with zipfile.ZipFile(path,"w") as z:
                z.writestr("gradle-8.9/bin/gradle","#!/bin/sh\\necho Gradle 8.9\\n")
        result,is_executable=self.run_verifier(create)
        self.assertEqual(result.returncode,0,result.stdout)
        self.assertTrue(is_executable)

    def test_rejects_path_traversal(self):
        def create(path):
            with zipfile.ZipFile(path,"w") as z:
                z.writestr("gradle-8.9/bin/gradle","#!/bin/sh")
                z.writestr("../escape","bad")
        result,_=self.run_verifier(create)
        self.assertNotEqual(result.returncode,0)
        self.assertIn("unsafe Gradle ZIP path",result.stdout)

    def test_rejects_checksum_mismatch(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=pathlib.Path(tmp)
            archive=base/"gradle.zip"
            with zipfile.ZipFile(archive,"w") as z:
                z.writestr("gradle-8.9/bin/gradle","#!/bin/sh")
            result=subprocess.run(
                [sys.executable,str(VERIFY),str(archive),str(base/"out"/"gradle-8.9"),
                 "--expected-sha256","0"*64],
                text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=10)
            self.assertNotEqual(result.returncode,0)
            self.assertIn("SHA-256 mismatch",result.stdout)

if __name__=="__main__":
    unittest.main()
