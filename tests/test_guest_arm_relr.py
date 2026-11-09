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


class RELRDecodeTests(unittest.TestCase):
    def test_direct_and_bitmap_entries_expand_in_order(self):
        blob = words([0x1000, 0x0b, 0x1084])
        self.assertEqual(module.decode_relr(blob), [0x1000, 0x1004, 0x100c, 0x1084])

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


class RELRTagTests(unittest.TestCase):
    @staticmethod
    def mapper(data, address, size, loads, label):
        offset = address - 0x1000
        if offset < 0 or offset + size > len(data):
            raise ELFAuditError(f"{label}: outside fixture")
        return offset, data[offset:offset + size]

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
