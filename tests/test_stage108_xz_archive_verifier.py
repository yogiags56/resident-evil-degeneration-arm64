import hashlib
import io
import pathlib
import subprocess
import sys
import tarfile
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
VERIFY = ROOT / "tools/verify_xz_source_archive.py"

def make_tar(path, members):
    with tarfile.open(path, "w:gz") as tf:
        for name, data, kind in members:
            info = tarfile.TarInfo(name)
            if kind == "symlink":
                info.type = tarfile.SYMTYPE
                info.linkname = data
                tf.addfile(info)
            else:
                payload = data.encode()
                info.size = len(payload)
                tf.addfile(info, io.BytesIO(payload))

class XzArchiveVerifierTests(unittest.TestCase):
    def run_verify(self, archive, expected):
        return subprocess.run(
            [sys.executable, str(VERIFY), str(archive), "--expected-sha256", expected,
             "--expected-version", "5.8.4"],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )

    def test_accepts_correct_checksum_safe_paths_and_expected_version(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = pathlib.Path(tmp) / "xz.tar.gz"
            header = ("#define LZMA_VERSION_MAJOR 5\n"
                      "#define LZMA_VERSION_MINOR 8\n"
                      "#define LZMA_VERSION_PATCH 4\n")
            make_tar(archive, [("xz/src/liblzma/api/lzma/version.h", header, "file")])
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            result = self.run_verify(archive, digest)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("PASS:", result.stdout)

    def test_rejects_checksum_mismatch(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = pathlib.Path(tmp) / "xz.tar.gz"
            make_tar(archive, [("xz/src/liblzma/api/lzma/version.h", "", "file")])
            result = self.run_verify(archive, "0" * 64)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("SHA-256 mismatch", result.stdout)

    def test_rejects_parent_traversal(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = pathlib.Path(tmp) / "xz.tar.gz"
            header = ("#define LZMA_VERSION_MAJOR 5\n"
                      "#define LZMA_VERSION_MINOR 8\n"
                      "#define LZMA_VERSION_PATCH 4\n")
            make_tar(archive, [
                ("xz/src/liblzma/api/lzma/version.h", header, "file"),
                ("xz/../../escape.txt", "bad", "file"),
            ])
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            result = self.run_verify(archive, digest)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("unsafe path", result.stdout)

    def test_rejects_escaping_symlink(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = pathlib.Path(tmp) / "xz.tar.gz"
            header = ("#define LZMA_VERSION_MAJOR 5\n"
                      "#define LZMA_VERSION_MINOR 8\n"
                      "#define LZMA_VERSION_PATCH 4\n")
            make_tar(archive, [
                ("xz/src/liblzma/api/lzma/version.h", header, "file"),
                ("xz/escape", "../../outside", "symlink"),
            ])
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            result = self.run_verify(archive, digest)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("unsafe link", result.stdout)

    def test_rejects_wrong_version_header(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = pathlib.Path(tmp) / "xz.tar.gz"
            header = ("#define LZMA_VERSION_MAJOR 5\n"
                      "#define LZMA_VERSION_MINOR 8\n"
                      "#define LZMA_VERSION_PATCH 3\n")
            make_tar(archive, [("xz/src/liblzma/api/lzma/version.h", header, "file")])
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            result = self.run_verify(archive, digest)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("expected XZ Utils 5.8.4", result.stdout)

if __name__ == "__main__":
    unittest.main()
