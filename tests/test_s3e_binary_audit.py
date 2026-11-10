import importlib.util
import struct
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "tools" / "s3e_binary_audit.py"
SPEC = importlib.util.spec_from_file_location("s3e_binary_audit", MODULE_PATH)
s3e = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(s3e)


def synthetic_image():
    symbols = b"s3eFileOpen\0s3eFileRead\0"
    sym_section = struct.pack("<IIH", 0, 8 + 2 + len(symbols), 2) + symbols
    # Two in-range external relocation cells: code offsets 0x4 and 0x8.
    ext = struct.pack("<II", 4, 8 + 4 + 12) + struct.pack(
        "<IHHHHHH", 2, 0, 0x0004, 0, 0, 0x0008, 1
    )
    fixups = sym_section + ext
    code_offset = 0x48 + len(fixups)
    header = bytearray(0x48)
    struct.pack_into(
        "<4sIHH" + "I" * 14, header, 0,
        b"XE3U", 0x00041901, 2, 5,
        0x48, len(fixups), code_offset, 16, 16,
        0, 0, 0, 0x48, 0, 0x4A000000, code_offset + 16, 0, 8
    )
    struct.pack_into("<I", header, 0x44, 0)
    return bytes(header) + fixups + bytes(range(16))


class S3EBinaryAuditTests(unittest.TestCase):
    def test_header_and_fixup_symbol_mapping(self):
        image = synthetic_image()
        result = s3e.audit_image(image)
        self.assertEqual(result["header"]["magic"], "XE3U")
        self.assertEqual(result["header"]["arch"], 5)
        self.assertEqual(len(result["code_blob_sha256"]), 64)
        self.assertEqual(result["fixups"]["symbol_count"], 2)
        self.assertEqual(
            result["target_import_relocation_offsets"]["s3eFileOpen"],
            ["0x4"],
        )
        self.assertEqual(
            result["target_import_relocation_offsets"]["s3eFileRead"],
            ["0x8"],
        )
        self.assertEqual(
            result["target_import_symbol_indices"]["s3eFileOpen"], [0]
        )
        self.assertEqual(
            result["target_import_symbol_indices"]["s3eFileRead"], [1]
        )
        self.assertEqual(
            result["target_import_relocation_counts_by_type"]["s3eFileRead"],
            {"4": 1},
        )
        self.assertEqual(result["external_relocation_cell_summary"], {"other": 2})
        self.assertEqual(len(result["import_manifest"]), 2)
        self.assertEqual(result["import_manifest"][0]["symbol_index"], 0)
        self.assertEqual(result["import_manifest"][0]["name"], "s3eFileOpen")
        self.assertEqual(result["import_manifest"][0]["cells"][0]["offset"], "0x4")
        self.assertEqual(result["import_manifest"][1]["cells"][0]["cell_kind"], "other")

    def test_rejects_external_cell_out_of_code_bounds(self):
        image = bytearray(synthetic_image())
        fixup_size = struct.unpack_from("<I", image, 0x10)[0]
        pos = 0x48
        end = pos + fixup_size
        while pos < end:
            typ, size = struct.unpack_from("<II", image, pos)
            if typ == 4:
                struct.pack_into("<HH", image, pos + 12, 0, 0x1000)
                break
            pos += size
        with self.assertRaises(s3e.S3EAuditError):
            s3e.audit_image(bytes(image))

    def test_lzma_alone_decompression(self):
        import lzma
        image = synthetic_image()
        compressed = lzma.compress(image, format=lzma.FORMAT_ALONE)
        self.assertEqual(s3e.decompress_s3e_stream(compressed), image)

    def test_rejects_wrong_magic(self):
        with self.assertRaises(s3e.S3EAuditError):
            s3e.parse_header(b"NOPE" + bytes(100))

    def test_rejects_fixup_out_of_bounds(self):
        image = bytearray(synthetic_image())
        struct.pack_into("<I", image, 0x10, len(image) + 100)
        with self.assertRaises(s3e.S3EAuditError):
            s3e.parse_header(bytes(image))

    def test_rejects_external_symbol_index_out_of_range(self):
        image = bytearray(synthetic_image())
        # Find the external relocation symbol index and replace it with 0xFFFF.
        fixup_size = struct.unpack_from("<I", image, 0x10)[0]
        pos = 0x48
        end = pos + fixup_size
        while pos < end:
            typ, size = struct.unpack_from("<II", image, pos)
            if typ == 4:
                # second relocation's symbol index
                struct.pack_into("<H", image, pos + 8 + 4 + 6 + 4, 0xFFFF)
                break
            pos += size
        with self.assertRaises(s3e.S3EAuditError):
            s3e.audit_image(bytes(image))


if __name__ == "__main__":
    unittest.main()
