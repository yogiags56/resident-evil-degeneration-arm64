import importlib.util
import lzma
import pathlib
import struct
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("dtrz_extract", ROOT / "tools/dtrz_extract.py")
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


def make_archive(entries, folders):
    """Build a tiny observed-profile archive for parser tests.

    entries: (filename, folder_index, decoded_bytes)
    folders: folder strings excluding root.
    """
    file_count = len(entries)
    header = b"DTRZ" + struct.pack("<HHB", file_count, len(folders) + 1, 0)
    names = b"".join(name.encode() + b"\0" for name, _, _ in entries)
    folder_table = b"".join(folder.encode() + b"\0" for folder in folders)
    attrs = b"".join(
        struct.pack("<HHH", folder_index, index, 0xFFFF)
        for index, (_, folder_index, _) in enumerate(entries)
    )
    loc_header_offset = len(header) + len(names) + len(folder_table) + len(attrs)
    loc_header = struct.pack("<HH", 1, file_count)
    loc_start = loc_header_offset + 4
    payload_start = loc_start + file_count * 16

    compressed = [lzma.compress(raw, format=lzma.FORMAT_ALONE) for _, _, raw in entries]
    offsets = []
    cursor = payload_start
    for blob in compressed:
        offsets.append(cursor)
        cursor += len(blob)
    locations = b"".join(
        struct.pack("<IIII", offsets[i], len(entries[i][2]), len(entries[i][2]), MODULE.METHOD_LZMA_ALONE)
        for i in range(file_count)
    )
    return header + names + folder_table + attrs + loc_header + locations + b"".join(compressed)


class DTRZExtractTests(unittest.TestCase):
    def test_parse_and_decode_root_and_folder_entries(self):
        data = make_archive(
            [
                ("root.bin", 0, b"root-data"),
                ("sub.bin", 1, b"folder-data"),
            ],
            ["assets\\sub"],
        )
        archive = MODULE.parse_archive(data)
        self.assertEqual(archive.file_count, 2)
        self.assertEqual(archive.folder_count, 2)
        self.assertEqual([entry.path for entry in archive.entries], ["root.bin", "assets/sub/sub.bin"])
        self.assertEqual(MODULE.decode_entry(data, archive.entries[0]), b"root-data")
        self.assertEqual(MODULE.decode_entry(data, archive.entries[1]), b"folder-data")

    def test_verify_reports_all_entries(self):
        data = make_archive([("a.bin", 0, b"A" * 100), ("b.bin", 0, b"B" * 50)], [])
        archive = MODULE.parse_archive(data)
        result = MODULE.verify_archive(data, archive)
        self.assertTrue(result["verified"])
        self.assertEqual(result["decoded_total_bytes"], 150)
        self.assertEqual(len(result["entries"]), 2)

    def test_extract_writes_expected_paths(self):
        data = make_archive([("level.bin", 1, b"level")], ["levels\\one"])
        archive = MODULE.parse_archive(data)
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(MODULE.extract_archive(data, archive, pathlib.Path(tmp)), 1)
            self.assertEqual((pathlib.Path(tmp) / "levels/one/level.bin").read_bytes(), b"level")

    def test_bad_magic_rejected(self):
        with self.assertRaises(MODULE.DTRZError):
            MODULE.parse_archive(b"NOPE\x00\x00\x00\x00\x00")

    def test_truncated_attribute_table_rejected(self):
        data = make_archive([("a.bin", 0, b"x")], [])
        with self.assertRaises(MODULE.DTRZError):
            MODULE.parse_archive(data[:20])

    def test_folder_index_out_of_range_rejected(self):
        data = bytearray(make_archive([("a.bin", 0, b"x")], []))
        # Header 9 + filename string 6; first attribute starts there.
        struct.pack_into("<H", data, 15, 1)
        with self.assertRaises(MODULE.DTRZError):
            MODULE.parse_archive(bytes(data))

    def test_file_index_mismatch_rejected(self):
        data = bytearray(make_archive([("a.bin", 0, b"x")], []))
        struct.pack_into("<H", data, 17, 7)
        with self.assertRaises(MODULE.DTRZError):
            MODULE.parse_archive(bytes(data))

    def test_location_count_mismatch_rejected(self):
        data = bytearray(make_archive([("a.bin", 0, b"x")], []))
        archive = MODULE.parse_archive(bytes(data))
        struct.pack_into("<H", data, archive.location_header_offset + 2, 2)
        with self.assertRaises(MODULE.DTRZError):
            MODULE.parse_archive(bytes(data))

    def test_unsafe_filename_rejected(self):
        data = make_archive([("../escape.bin", 0, b"x")], [])
        with self.assertRaises(MODULE.DTRZError):
            MODULE.parse_archive(data)

    def test_non_monotonic_payload_offsets_rejected(self):
        data = bytearray(make_archive([("a.bin", 0, b"a"), ("b.bin", 0, b"b")], []))
        archive = MODULE.parse_archive(bytes(data))
        first = archive.entries[0].offset
        second_record = archive.location_table_offset + 16
        struct.pack_into("<I", data, second_record, first)
        with self.assertRaises(MODULE.DTRZError):
            MODULE.parse_archive(bytes(data))

    def test_unknown_method_rejected(self):
        data = bytearray(make_archive([("a.bin", 0, b"x")], []))
        archive = MODULE.parse_archive(bytes(data))
        struct.pack_into("<I", data, archive.location_table_offset + 12, 0xDEADBEEF)
        parsed = MODULE.parse_archive(bytes(data))
        with self.assertRaises(MODULE.DTRZError):
            MODULE.decode_entry(bytes(data), parsed.entries[0])

    def test_decoded_size_mismatch_rejected(self):
        data = bytearray(make_archive([("a.bin", 0, b"x")], []))
        archive = MODULE.parse_archive(bytes(data))
        struct.pack_into("<I", data, archive.location_table_offset + 4, 2)
        struct.pack_into("<I", data, archive.location_table_offset + 8, 2)
        parsed = MODULE.parse_archive(bytes(data))
        with self.assertRaises(MODULE.DTRZError):
            MODULE.decode_entry(bytes(data), parsed.entries[0])


if __name__ == "__main__":
    unittest.main(verbosity=2)
