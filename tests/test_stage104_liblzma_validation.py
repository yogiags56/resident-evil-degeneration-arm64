from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Stage104LibLzmaValidationTests(unittest.TestCase):
    def read(self, relative):
        return (ROOT / relative).read_text(encoding="utf-8")

    def test_host_smoke_uses_find_package_imported_target(self):
        cmake = self.read("tests/cmake_liblzma_smoke/CMakeLists.txt")
        self.assertIn("find_package(LibLZMA REQUIRED)", cmake)
        self.assertIn("LibLZMA::LibLZMA", cmake)
        self.assertIn("runtime/archive/re_dtrz_lzma.c", cmake)

    def test_host_smoke_preserves_assertions_in_release(self):
        cmake = self.read("tests/cmake_liblzma_smoke/CMakeLists.txt")
        self.assertIn("-UNDEBUG", cmake)

    def test_android_archive_verifier_checks_architecture(self):
        script = self.read("tools/verify_android_liblzma.sh")
        self.assertIn("ELF64", script)
        self.assertIn("AArch64", script)
        self.assertIn("liblzma.a", script)
        self.assertIn("lzma.h", script)
        self.assertIn('for index in "${!MEMBERS[@]}"', script)
        self.assertIn("all $verified liblzma archive members", script)

    def test_host_smoke_script_is_cmake_based(self):
        script = self.read("tools/run_host_liblzma_cmake_smoke.sh")
        self.assertIn("cmake -S", script)
        self.assertIn("test_re_dtrz_lzma", script)


if __name__ == "__main__":
    unittest.main()
