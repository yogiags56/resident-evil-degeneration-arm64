import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("dtrz_inspect", ROOT / "tools/dtrz_inspect.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class DTRZInspectorTests(unittest.TestCase):
    def fixture(self):
        # 9-byte observed header, two root names, two paths (last is sentinel),
        # followed by opaque metadata bytes.
        return b"DTRZ/\x00\x23\x00\x00" + b"a.bin\x00b.bin\x00" + b"folder\x00\x00" + b"\x00\x00\xff\xffDATA"

    def test_tables_and_boundary(self):
        result = MODULE.inspect_bytes(self.fixture(), root_count=2, path_count=2)
        self.assertEqual([x["value"] for x in result["root_names"]], ["a.bin", "b.bin"])
        self.assertEqual([x["value"] for x in result["paths"]], ["folder", ""])
        self.assertTrue(result["path_table_has_empty_final_sentinel"])
        self.assertEqual(result["metadata"]["offset"], 9 + 12 + 8)
        self.assertFalse(result["metadata"]["payload_offsets_decoded"])

    def test_bad_magic_rejected(self):
        with self.assertRaises(MODULE.DTRZError):
            MODULE.inspect_bytes(b"NOPE/\x00\x00\x00\x00")

    def test_unterminated_table_rejected(self):
        with self.assertRaises(MODULE.DTRZError):
            MODULE.inspect_bytes(b"DTRZ/\x00\x23\x00\x00no-terminator", 1, 0)

    def test_empty_root_rejected(self):
        with self.assertRaises(MODULE.DTRZError):
            MODULE.inspect_bytes(b"DTRZ/\x00\x23\x00\x00\x00x\x00\x00", 1, 1)

    def test_negative_count_rejected(self):
        with self.assertRaises(MODULE.DTRZError):
            MODULE.inspect_bytes(self.fixture(), -1, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
