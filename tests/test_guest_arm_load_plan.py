import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import guest_arm_load_plan as module
from guest_arm_elf_audit import ELFAuditError


def fixture(etype=3, segments=None, entry=0x1100):
    if segments is None:
        segments = [
            # offset, vaddr, filesz, memsz, flags, align
            (0x1000, 0x1000, 0x100, 0x180, 5, 0x1000),
        ]
    phoff = 52
    phnum = len(segments)
    data = bytearray(max(0x2000, phoff + phnum * 32))
    data[:16] = b"\\x7fELF" + bytes([1, 1, 1]) + bytes(9)
    struct.pack_into(
        "<HHIIIIIHHHHHH", data, 16,
        etype, 40, 1, entry, phoff, 0, 0, 52, 32, phnum, 40, 0, 0,
    )
    for index, (offset, vaddr, filesz, memsz, flags, align) in enumerate(segments):
        if offset + filesz > len(data):
            data.extend(bytes(offset + filesz - len(data)))
        struct.pack_into(
            "<IIIIIIII", data, phoff + index * 32,
            1, offset, vaddr, 0, filesz, memsz, flags, align,
        )
    return bytes(data)


class LoadPlanTests(unittest.TestCase):
    def test_reports_file_backed_and_bss_zero_fill_ranges(self):
        report = module.plan_load_segments(fixture())
        segment = report["segments"][0]
        self.assertEqual(segment["file_backed_range"], [0x1000, 0x1100])
        self.assertEqual(segment["zero_fill_range"], [0x1100, 0x1180])
        self.assertEqual(segment["page_range"], [0x1000, 0x2000])
        self.assertEqual(segment["permissions"], ["R", "X"])
        self.assertFalse(report["memory_modified"])

    def test_et_dyn_requires_runtime_load_bias_without_calculating_it(self):
        report = module.plan_load_segments(fixture(etype=3))
        self.assertEqual(report["load_bias_policy"], "required_at_runtime_not_calculated")

    def test_et_exec_keeps_fixed_virtual_address_policy(self):
        report = module.plan_load_segments(fixture(etype=2))
        self.assertEqual(report["load_bias_policy"], "fixed_virtual_addresses_no_bias")

    def test_reports_page_overlap_and_combined_wx_permissions(self):
        segments = [
            (0x1000, 0x1000, 0x100, 0x500, 5, 0x1000),
            (0x2000, 0x1400, 0x100, 0x300, 6, 0x1000),
        ]
        report = module.plan_load_segments(fixture(segments=segments))
        self.assertEqual(report["page_overlap_count"], 1)
        self.assertTrue(report["page_overlaps"][0]["writable_executable"])
        self.assertIn(
            "page overlap combines writable and executable permissions",
            report["warnings"],
        )

    def test_rejects_memory_range_overflow(self):
        segments = [(0x1000, 0xfffffff0, 0x10, 0x40, 5, 1)]
        with self.assertRaises(ELFAuditError):
            module.plan_load_segments(fixture(segments=segments))

    def test_rejects_non_power_of_two_page_size(self):
        with self.assertRaises(ELFAuditError):
            module.plan_load_segments(fixture(), page_size=3000)

    def test_rejects_non_executable_entry_point(self):
        report = module.plan_load_segments(fixture(entry=0x1180))
        self.assertEqual(report["entry_executable_owners"], [])
        self.assertIn(
            "ELF entry point is not inside an executable PT_LOAD memory range",
            report["warnings"],
        )

    def test_rejects_unsupported_elf_type(self):
        with self.assertRaises(ELFAuditError):
            module.plan_load_segments(fixture(etype=1))

    def test_rejects_no_pt_load_segments(self):
        data = bytearray(fixture())
        struct.pack_into("<H", data, 44, 0)
        with self.assertRaises(ELFAuditError):
            module.plan_load_segments(bytes(data))


if __name__ == "__main__":
    unittest.main()
