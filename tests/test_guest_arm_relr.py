import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import guest_arm_relr as module
from guest_arm_elf_audit import ELFAuditError


def words(values, endian="<"):
    return b"".join(struct.pack(endian + "I", value) for value in values)



def make_relr_elf32_fixture():
    """Create a minimal ELF32 ARM DYN file with a SHT_RELR section."""
    blob = bytearray(0x1f8)
    ident = b"\x7fELF" + bytes([1, 1, 1]) + bytes(9)
    struct.pack_into(
        "<16sHHIIIIIHHHHHH", blob, 0, ident,
        3, 40, 1, 0, 52, 0x180, 0, 52, 32, 1, 40, 3, 2,
    )
    struct.pack_into("<IIIIIIII", blob, 52, 1, 0, 0x1000, 0x1000, 0x140, 0x140, 5, 0x1000)
    struct.pack_into("<III", blob, 0x100, 0x2000, 0x0b, 0x2080)
    names = b"\0.relr.dyn\0.shstrtab\0"
    blob[0x120:0x120 + len(names)] = names
    struct.pack_into("<IIIIIIIIII", blob, 0x180 + 40, 1, 19, 2, 0x1100, 0x100, 12, 0, 0, 4, 4)
    struct.pack_into("<IIIIIIIIII", blob, 0x180 + 80, 11, 3, 0, 0, 0x120, len(names), 0, 0, 1, 0)
    return bytes(blob)


class RELRDecodeTests(unittest.TestCase):
    def test_direct_and_bitmap_entries_expand_in_order(self):
        blob = words([0x1000, 0x0b, 0x1084])
        self.assertEqual(module.decode_relr(blob), [0x1000, 0x1004, 0x100c, 0x1084])

    def test_decode_records_preserves_direct_and_bitmap_provenance(self):
        records = module.decode_relr_records(words([0x1000, 0x0b]))
        self.assertEqual(records, [
            {"index": 0, "table_entry_index": 0, "encoding": "direct",
             "target_address": "0x00001000", "target_offset": 0x1000},
            {"index": 1, "table_entry_index": 1, "encoding": "bitmap",
             "bitmap_bit": 0, "target_address": "0x00001004", "target_offset": 0x1004},
            {"index": 2, "table_entry_index": 1, "encoding": "bitmap",
             "bitmap_bit": 2, "target_address": "0x0000100c", "target_offset": 0x100c},
        ])

    def test_bitmap_advances_cursor_by_31_words_even_when_sparse(self):
        blob = words([0x2000, 1 | (1 << 31), 0x2080])
        self.assertEqual(module.decode_relr(blob), [0x2000, 0x207c, 0x2080])

    def test_supports_big_endian_elf32(self):
        self.assertEqual(module.decode_relr(words([0x3000, 0x03], ">"), ">"),
                         [0x3000, 0x3004])

    def test_empty_table_is_valid(self):
        self.assertEqual(module.decode_relr(b""), [])

    def test_rejects_bitmap_before_initial_direct_address(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(words([0x03]))

    def test_rejects_unaligned_direct_address(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(words([0x1002]))

    def test_rejects_direct_address_that_overlaps_previous_bitmap_window(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(words([0x1000, 0x03, 0x1004]))

    def test_rejects_non_monotonic_direct_addresses(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(words([0x2000, 0x1000]))

    def test_rejects_partial_word(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(b"\x00\x10\x00")

    def test_rejects_bitmap_address_overflow(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(words([0xfffffffc, 0xffffffff]))

    def test_rejects_bitmap_cursor_overflow(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(words([0xfffffffc, 0x01]))


    def test_expanded_relocation_count_has_independent_safety_limit(self):
        previous = module.MAX_RELR_RELOCATIONS
        try:
            module.MAX_RELR_RELOCATIONS = 2
            with self.assertRaises(ELFAuditError):
                module.decode_relr(words([0x1000, 0xffffffff]))
        finally:
            module.MAX_RELR_RELOCATIONS = previous

    def test_rejects_invalid_word_size(self):
        with self.assertRaises(ELFAuditError):
            module.decode_relr(words([0x1000]), word_size=8)



class RELRDifferentialTests(unittest.TestCase):
    def test_matches_gnu_readelf_expanded_relocation_addresses(self):
        import re
        import shutil
        import subprocess
        import tempfile

        readelf = shutil.which("readelf")
        if readelf is None:
            self.skipTest("GNU readelf is unavailable")
        fixture = make_relr_elf32_fixture()
        raw = fixture[0x100:0x10c]
        expected = [0x2000, 0x2004, 0x200c, 0x2080]
        self.assertEqual(module.decode_relr(raw), expected)
        with tempfile.NamedTemporaryFile(suffix=".elf") as file:
            file.write(fixture)
            file.flush()
            result = subprocess.run(
                [readelf, "-rW", file.name],
                check=True, capture_output=True, text=True,
            )
        reported = {
            int(value, 16)
            for value in re.findall(r"(?m)^\s*([0-9a-fA-F]{8})\s*$", result.stdout)
        }
        self.assertEqual(reported, set(expected), result.stdout)

class RELRTagTests(unittest.TestCase):
    @staticmethod
    def mapper(data, address, size, loads, label):
        offset = address - 0x1000
        if offset < 0 or offset + size > len(data):
            raise ELFAuditError(f"{label}: outside fixture")
        return offset, data[offset:offset + size]

    def test_standard_relr_triplet_preserves_target_provenance(self):
        data = bytearray(0x400)
        data[0x200:0x208] = words([0x1300, 0x0b])
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 8, module.DT_RELRENT: 4}
        tables = module.collect_relr_tables(bytes(data), tags, [], "<", self.mapper)
        self.assertEqual(tables[0]["targets"],
                         ["0x00001300", "0x00001304", "0x0000130c"])
        self.assertEqual([r["encoding"] for r in tables[0]["target_records"]],
                         ["direct", "bitmap", "bitmap"])
        self.assertEqual([r.get("bitmap_bit") for r in tables[0]["target_records"]],
                         [None, 0, 2])

    def test_standard_relr_triplet_maps_and_decodes(self):
        data = bytearray(0x400)
        data[0x200:0x208] = words([0x1300, 0x0b])
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 8, module.DT_RELRENT: 4}
        tables = module.collect_relr_tables(bytes(data), tags, [], "<", self.mapper)
        self.assertEqual(tables[0]["targets"], ["0x00001300", "0x00001304", "0x0000130c"])
        self.assertEqual(tables[0]["source_tags"], ["DT_RELR", "DT_RELRSZ", "DT_RELRENT"])

    def test_android_relr_triplet_maps_and_decodes(self):
        data = bytearray(0x400)
        data[0x200:0x204] = words([0x1300])
        tags = {module.DT_ANDROID_RELR: 0x1200,
                module.DT_ANDROID_RELRSZ: 4, module.DT_ANDROID_RELRENT: 4}
        tables = module.collect_relr_tables(bytes(data), tags, [], "<", self.mapper)
        self.assertEqual(tables[0]["source_tags"], ["DT_ANDROID_RELR", "DT_ANDROID_RELRSZ", "DT_ANDROID_RELRENT"])

    def test_identical_standard_and_android_triplets_are_reported_once(self):
        data = bytearray(0x400)
        data[0x200:0x204] = words([0x1300])
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 4, module.DT_RELRENT: 4,
                module.DT_ANDROID_RELR: 0x1200,
                module.DT_ANDROID_RELRSZ: 4, module.DT_ANDROID_RELRENT: 4}
        tables = module.collect_relr_tables(bytes(data), tags, [], "<", self.mapper)
        self.assertEqual(len(tables), 1)
        self.assertEqual(tables[0]["source_tags"], ["DT_RELR", "DT_RELRSZ", "DT_RELRENT", "DT_ANDROID_RELR", "DT_ANDROID_RELRSZ", "DT_ANDROID_RELRENT"])

    def test_rejects_conflicting_standard_and_android_triplets(self):
        data = bytearray(0x400)
        data[0x200:0x208] = words([0x1300, 0x1400])
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 4, module.DT_RELRENT: 4,
                module.DT_ANDROID_RELR: 0x1204,
                module.DT_ANDROID_RELRSZ: 4, module.DT_ANDROID_RELRENT: 4}
        with self.assertRaises(ELFAuditError):
            module.collect_relr_tables(bytes(data), tags, [], "<", self.mapper)


    def test_accepts_mixed_standard_and_android_tag_aliases(self):
        data = bytearray(0x400)
        data[0x200:0x204] = words([0x1300])
        tags = {module.DT_RELR: 0x1200, module.DT_ANDROID_RELRSZ: 4,
                module.DT_RELRENT: 4}
        tables = module.collect_relr_tables(bytes(data), tags, [], "<", self.mapper)
        self.assertEqual(tables[0]["targets"], ["0x00001300"])
        self.assertEqual(tables[0]["source_tags"],
                         ["DT_RELR", "DT_RELRENT", "DT_ANDROID_RELRSZ"])

    def test_preserves_android_relr_count_hint_without_using_it_as_table_size(self):
        data = bytearray(0x400)
        data[0x200:0x204] = words([0x1300])
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 4, module.DT_RELRENT: 4,
                module.DT_ANDROID_RELRCOUNT: 99}
        tables = module.collect_relr_tables(bytes(data), tags, [], "<", self.mapper)
        self.assertEqual(tables[0]["relocation_count"], 1)
        self.assertEqual(tables[0]["android_relr_count_hint"], 99)

    def test_rejects_incomplete_standard_tag_triplet(self):
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 4}
        with self.assertRaises(ELFAuditError):
            module.collect_relr_tables(b"", tags, [], "<", self.mapper)


    def test_android_relr_count_without_table_is_preserved_as_metadata(self):
        tags = {module.DT_ANDROID_RELRCOUNT: 2}
        self.assertEqual(module.collect_relr_tables(b"", tags, [], "<", self.mapper), [])
        self.assertEqual(tags[module.DT_ANDROID_RELRCOUNT], 2)

    def test_rejects_invalid_entry_size(self):
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 4, module.DT_RELRENT: 8}
        with self.assertRaises(ELFAuditError):
            module.collect_relr_tables(b"", tags, [], "<", self.mapper)


    def test_reports_overlapping_targets_across_distinct_formats(self):
        overlaps = module.find_cross_format_overlaps([
            ("REL", ["0x00001000", "0x00001004"]),
            ("RELR", ["0x00001004", "0x00001008"]),
        ])
        self.assertEqual(overlaps, [
            {"target_address": "0x00001004", "formats": ["REL", "RELR"]}
        ])

    def test_does_not_report_duplicates_within_one_format(self):
        self.assertEqual(module.find_cross_format_overlaps([
            ("DT_REL", ["0x1000", "0x1000"]),
            ("DT_REL", ["0x1000"]),
        ]), [])

    def test_rejects_non_multiple_table_size(self):
        tags = {module.DT_RELR: 0x1200, module.DT_RELRSZ: 6, module.DT_RELRENT: 4}
        with self.assertRaises(ELFAuditError):
            module.collect_relr_tables(b"", tags, [], "<", self.mapper)


if __name__ == "__main__":
    unittest.main()
