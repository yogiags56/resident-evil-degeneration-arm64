import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import guest_arm_aps2_relocation_audit as module


def sleb(value):
    out = bytearray()
    more = True
    while more:
        byte = value & 0x7f
        value >>= 7
        sign = byte & 0x40
        more = not ((value == 0 and not sign) or (value == -1 and sign))
        out.append(byte | (0x80 if more else 0))
    return bytes(out)


def packed(kind="REL", reloc_type=22):
    out = bytearray(b"APS2")
    out += sleb(2) + sleb(0x1300)
    flags = 3 if kind == "REL" else 15
    out += sleb(2) + sleb(flags)
    out += sleb(4) + sleb((1 << 8) | reloc_type)
    if kind == "RELA":
        out += sleb(5)
    return bytes(out)


def elf_fixture(kind="REL", reloc_type=22):
    data = bytearray(0x300)
    data[:7] = b"\x7fELF" + bytes([1, 1, 1])
    struct.pack_into("<HHIIIIIHHHHHH", data, 16, 3, 40, 1, 0x1000, 52, 0, 0, 52, 32, 2, 40, 0, 0)
    struct.pack_into("<IIIIIIII", data, 52, 1, 0, 0x1000, 0, 0x300, 0x400, 5, 0x1000)
    struct.pack_into("<IIIIIIII", data, 84, 2, 0x100, 0x1100, 0, 0x78, 0x78, 4, 4)
    tags = [(0x6000000f if kind == "REL" else 0x60000011, 0x1220),
            (0x60000010 if kind == "REL" else 0x60000012, len(packed(kind, reloc_type))),
            (0, 0)]
    for i, (tag, value) in enumerate(tags):
        struct.pack_into("<iI", data, 0x100 + i * 8, tag, value)
    blob = packed(kind, reloc_type)
    data[0x220:0x220 + len(blob)] = blob
    return bytes(data)


def elf_fixture_with_symbols(kind="REL", reloc_type=20, symbol_size=37, section_index=2):
    data = bytearray(elf_fixture(kind))
    tags = [
        (0x6000000f if kind == "REL" else 0x60000011, 0x1220),
        (0x60000010 if kind == "REL" else 0x60000012, len(packed(kind, reloc_type))),
        (4, 0x11c0),   # DT_HASH
        (6, 0x11d0),   # DT_SYMTAB
        (11, 16),      # DT_SYMENT
        (5, 0x1180),   # DT_STRTAB
        (10, 5),       # DT_STRSZ
        (0, 0),
    ]
    struct.pack_into("<IIIIIIII", data, 84, 2, 0x100, 0x1100, 0, len(tags) * 8, len(tags) * 8, 4, 4)
    for i, (tag, value) in enumerate(tags):
        struct.pack_into("<iI", data, 0x100 + i * 8, tag, value)
    data[0x180:0x185] = b"\0foo\0"
    struct.pack_into("<IIII", data, 0x1c0, 1, 2, 1, 0)
    struct.pack_into("<IIIBBH", data, 0x1d0, 0, 0, 0, 0, 0, 0)
    struct.pack_into("<IIIBBH", data, 0x1e0, 1, 0x1300, symbol_size, 0x12, 0, section_index)
    blob = packed(kind, reloc_type)
    data[0x220:0x220 + len(blob)] = blob
    return bytes(data)


class SlebTests(unittest.TestCase):
    def test_signed_values_round_trip(self):
        for value in (-2147483648, -8193, -65, -64, -1, 0, 63, 64, 8192, 2147483647):
            self.assertEqual(module._sleb128(sleb(value), 0)[0], value)


    def test_signed_32_bit_sleb_fifth_byte_boundaries(self):
        self.assertEqual(module._sleb128(b"\x80\x80\x80\x80\x07", 0)[0], 0x70000000)
        self.assertEqual(module._sleb128(b"\x80\x80\x80\x80\x78", 0)[0], -0x80000000)
        for last in (0x08, 0x77):
            with self.subTest(last=last), self.assertRaises(module.ELFAuditError):
                module._sleb128(b"\x80\x80\x80\x80" + bytes([last]), 0)

    def test_rejects_fifth_byte_continuation_even_with_valid_payload(self):
        with self.assertRaises(module.ELFAuditError):
            module._sleb128(b"\x80\x80\x80\x80\x87", 0)

    def test_rejects_truncated_and_overlong_values(self):
        with self.assertRaises(module.ELFAuditError):
            module._sleb128(b"\x80", 0)
        with self.assertRaises(module.ELFAuditError):
            module._sleb128(b"\x80\x80\x80\x80\x80", 0)


class APS2DecodeTests(unittest.TestCase):
    def test_decodes_grouped_rel_entries(self):
        entries = module.decode_aps2(packed("REL"), "REL")
        self.assertEqual([e["target_address"] for e in entries], ["0x00001304", "0x00001308"])
        self.assertEqual(entries[0]["type_name"], "R_ARM_JUMP_SLOT")
        self.assertEqual([e["type"] for e in entries], [22, 22])
        self.assertEqual([e["symbol_index"] for e in entries], [1, 1])
        self.assertIsNone(entries[0]["addend"])

    def test_decodes_rela_cumulative_addends(self):
        entries = module.decode_aps2(packed("RELA"), "RELA")
        self.assertEqual([e["addend"] for e in entries], [5, 5])

    def test_rejects_bad_magic_and_trailing_bytes(self):
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(b"NOPE", "REL")
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(packed() + b"\0", "REL")

    def test_rejects_invalid_group_flags_and_sizes(self):
        blob = b"APS2" + sleb(1) + sleb(0) + sleb(1) + sleb(4)
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(blob, "REL")
        blob = b"APS2" + sleb(1) + sleb(0) + sleb(2) + sleb(0)
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(blob, "REL")

    def test_rejects_negative_offset_delta_and_overflow(self):
        blob = b"APS2" + sleb(1) + sleb(4) + sleb(1) + sleb(0) + sleb(-1) + sleb(0)
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(blob, "REL")
        blob = b"APS2" + sleb(1) + sleb(-1) + sleb(1) + sleb(0) + sleb(1) + sleb(0)
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(blob, "REL")

    def test_unsigned_elf32_initial_offset_with_high_bit(self):
        blob = b"APS2" + sleb(1) + sleb(-0x80000000)
        blob += sleb(1) + sleb(3) + sleb(4) + sleb((1 << 8) | 22)
        entry = module.decode_aps2(blob, "REL")[0]
        self.assertEqual(entry["target_address"], "0x80000004")

    def test_rejects_addends_in_rel_stream(self):
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(packed("RELA"), "REL")

    def test_audits_dynamic_aps2_rel_and_rela(self):
        for kind in ("REL", "RELA"):
            report = module.audit(elf_fixture(kind))
            self.assertEqual(report["relocation_count"], 2)
            self.assertEqual(report["packed_relocation_tables"][0]["kind"], kind)
            self.assertEqual(report["packed_relocation_tables"][0]["file_offset"], 0x220)

    def test_aps2_entries_preserve_symbol_size_and_definition_state(self):
        report = module.audit(elf_fixture_with_symbols())
        entry = report["packed_relocation_tables"][0]["entries"][0]
        self.assertEqual(entry["symbol"], "foo")
        self.assertEqual(entry["symbol_size"], 37)
        self.assertTrue(entry["symbol_defined"])
        self.assertEqual(entry["symbol_section_index"], 2)

    def test_aps2_keeps_copy_size_unresolved_without_bounded_symbol_table(self):
        report = module.audit(elf_fixture("REL"))
        entry = report["packed_relocation_tables"][0]["entries"][0]
        self.assertIsNone(entry["symbol_size"])
        self.assertIsNone(entry["symbol_defined"])

    def test_requires_paired_dynamic_tags(self):
        data = bytearray(elf_fixture())
        struct.pack_into("<iI", data, 0x108, 0, 0)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))

    def test_rejects_non_file_backed_packed_table(self):
        data = bytearray(elf_fixture())
        struct.pack_into("<I", data, 0x108, 0x1400)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))


    def test_decodes_ungrouped_absolute_r_info_values(self):
        blob = bytearray(b"APS2")
        blob += sleb(2) + sleb(0x2000)
        blob += sleb(2) + sleb(0)  # two records, no grouped fields
        blob += sleb(4) + sleb((1 << 8) | 22)
        blob += sleb(4) + sleb((2 << 8) | 23)
        entries = module.decode_aps2(bytes(blob), "REL")
        self.assertEqual([e["symbol_index"] for e in entries], [1, 2])
        self.assertEqual([e["type"] for e in entries], [22, 23])

    def test_rela_addend_without_addend_grouping_accumulates_per_entry(self):
        blob = b"APS2" + sleb(2) + sleb(0x2000)
        blob += sleb(2) + sleb(9)
        blob += sleb((1 << 8) | 22)
        blob += sleb(4) + sleb(5)
        blob += sleb(4) + sleb(3)
        entries = module.decode_aps2(blob, "RELA")
        self.assertEqual([e["addend"] for e in entries], [5, 8])

    def test_rela_addend_without_has_addend_resets_to_zero_per_group(self):
        blob = b"APS2" + sleb(2) + sleb(0x2000)
        blob += sleb(1) + sleb(15)
        blob += sleb(4) + sleb((1 << 8) | 22) + sleb(5)
        blob += sleb(1) + sleb(3)
        blob += sleb(4) + sleb((1 << 8) | 22)
        entries = module.decode_aps2(blob, "RELA")
        self.assertEqual([e["addend"] for e in entries], [5, 0])


    @staticmethod
    def _group_blob(kind, flags):
        out = bytearray(b"APS2")
        out += sleb(2) + sleb(0x1000)
        out += sleb(2) + sleb(flags)
        if flags & module.GROUPED_BY_OFFSET_DELTA:
            out += sleb(4)
        if flags & module.GROUPED_BY_INFO:
            out += sleb((1 << 8) | 22)
        if kind == "RELA" and flags & module.HAS_ADDEND and flags & module.GROUPED_BY_ADDEND:
            out += sleb(5)
        for index in range(2):
            if not flags & module.GROUPED_BY_OFFSET_DELTA:
                out += sleb(4 if index == 0 else 8)
            if not flags & module.GROUPED_BY_INFO:
                out += sleb(((1 + index) << 8) | (22 + index))
            if kind == "RELA" and flags & module.HAS_ADDEND and not flags & module.GROUPED_BY_ADDEND:
                out += sleb(5 if index == 0 else 3)
        return bytes(out)

    def test_all_legal_rel_group_flag_combinations(self):
        for flags in (0, 1, 2, 3):
            with self.subTest(flags=flags):
                entries = module.decode_aps2(self._group_blob("REL", flags), "REL")
                self.assertEqual([e["target_address"] for e in entries], ["0x00001004", "0x0000100c"])
                if flags & module.GROUPED_BY_INFO:
                    self.assertEqual([(e["symbol_index"], e["type"]) for e in entries], [(1, 22), (1, 22)])
                else:
                    self.assertEqual([(e["symbol_index"], e["type"]) for e in entries], [(1, 22), (2, 23)])

    def test_all_legal_rela_group_flag_combinations(self):
        legal = (0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15)
        for flags in legal:
            with self.subTest(flags=flags):
                entries = module.decode_aps2(self._group_blob("RELA", flags), "RELA")
                self.assertEqual(len(entries), 2)
                if flags & module.HAS_ADDEND:
                    if flags & module.GROUPED_BY_ADDEND:
                        self.assertEqual([e["addend"] for e in entries], [5, 5])
                    else:
                        self.assertEqual([e["addend"] for e in entries], [5, 8])
                else:
                    self.assertEqual([e["addend"] for e in entries], [0, 0])

    def test_rejects_every_illegal_rel_group_flag_combination(self):
        for flags in (4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15):
            with self.subTest(flags=flags), self.assertRaises(module.ELFAuditError):
                module.decode_aps2(self._group_blob("REL", flags), "REL")

    def test_rejects_rela_grouped_addend_without_has_addend(self):
        for flags in (4, 5, 6, 7):
            with self.subTest(flags=flags), self.assertRaises(module.ELFAuditError):
                module.decode_aps2(self._group_blob("RELA", flags), "RELA")

    def test_rela_addend_state_transitions_across_groups(self):
        blob = b"APS2" + sleb(3) + sleb(0x2000)
        # First group: per-entry addend deltas +5, then +3.
        blob += sleb(2) + sleb(8)
        blob += sleb(4) + sleb((1 << 8) | 22) + sleb(5)
        blob += sleb(4) + sleb((1 << 8) | 22) + sleb(3)
        # Second group: HAS_ADDEND absent resets the accumulated addend.
        blob += sleb(1) + sleb(0)
        blob += sleb(4) + sleb((1 << 8) | 22)
        # Third group: grouped delta is applied to the reset value.
        blob += sleb(1) + sleb(13) + sleb((1 << 8) | 22) + sleb(-2)
        entries = module.decode_aps2(blob, "RELA")
        self.assertEqual([e["addend"] for e in entries], [5, 8, 0])

    def test_offset_delta_accumulation_across_groups(self):
        blob = b"APS2" + sleb(3) + sleb(0x1000)
        blob += sleb(2) + sleb(3) + sleb(4) + sleb((1 << 8) | 22)
        blob += sleb(1) + sleb(0) + sleb(8) + sleb((2 << 8) | 23)
        entries = module.decode_aps2(blob, "REL")
        self.assertEqual([e["target_address"] for e in entries],
                         ["0x00001004", "0x00001008", "0x00001010"])

    def test_rejects_group_size_larger_than_remaining_count(self):
        blob = b"APS2" + sleb(1) + sleb(0) + sleb(2) + sleb(0)
        with self.assertRaises(module.ELFAuditError):
            module.decode_aps2(blob, "REL")

    def test_rejects_truncated_group_header_and_each_payload_field(self):
        cases = (
            b"APS2" + sleb(1) + sleb(0),
            b"APS2" + sleb(1) + sleb(0) + sleb(1),
            b"APS2" + sleb(1) + sleb(0) + sleb(1) + sleb(2),
            b"APS2" + sleb(1) + sleb(0) + sleb(1) + sleb(1),
            b"APS2" + sleb(1) + sleb(0) + sleb(1) + sleb(0) + sleb(4),
        )
        for blob in cases:
            with self.subTest(blob=blob), self.assertRaises(module.ELFAuditError):
                module.decode_aps2(blob, "REL")

    def test_signed_sleb_r_info_preserves_elf32_bit_pattern(self):
        blob = b"APS2" + sleb(1) + sleb(0)
        blob += sleb(1) + sleb(0) + sleb(4) + sleb(-1)
        entry = module.decode_aps2(blob, "REL")[0]
        self.assertEqual(entry["symbol_index"], 0xffffff)
        self.assertEqual(entry["type"], 0xff)

if __name__ == "__main__":
    unittest.main()
