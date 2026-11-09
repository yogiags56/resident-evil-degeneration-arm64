"""Differentially validate PT_DYNAMIC RELR decoding against GNU readelf.

The fixture deliberately exposes the same RELR bytes through both PT_DYNAMIC
and SHT_RELR. The project audit must consume the dynamic tags; readelf supplies
an independent section-backed oracle.
"""
import re
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))

import guest_arm_relr_audit as relr_audit
import guest_arm_relocation_reconciliation as reconciliation
from test_guest_arm_relr_audit import fixture as dynamic_fixture


def dynamic_and_section_relr_fixture():
    data = bytearray(dynamic_fixture())
    # Keep the original PT_LOAD and PT_DYNAMIC layout. Add section headers
    # outside PT_LOAD so the dynamic audit remains independent of sections.
    data.extend(bytes(0x340 - len(data)))
    names = b"\0.relr.dyn\0.shstrtab\0"
    data[0x240:0x240 + len(names)] = names
    struct.pack_into("<I", data, 32, 0x280)       # e_shoff
    struct.pack_into("<H", data, 46, 40)         # e_shentsize
    struct.pack_into("<H", data, 48, 3)          # e_shnum
    struct.pack_into("<H", data, 50, 2)          # e_shstrndx
    # SHT_RELR: same bytes already referenced by DT_RELR at file offset 0x200.
    struct.pack_into("<IIIIIIIIII", data, 0x280 + 40,
                     1, 19, 2, 0x1200, 0x200, 12, 0, 0, 4, 4)
    struct.pack_into("<IIIIIIIIII", data, 0x280 + 80,
                     11, 3, 0, 0, 0x240, len(names), 0, 0, 1, 0)
    return bytes(data)


class DynamicRELRDifferentialTests(unittest.TestCase):
    def test_pt_dynamic_expansion_matches_gnu_readelf_sht_relr(self):
        readelf = shutil.which("readelf")
        if readelf is None:
            self.skipTest("GNU readelf is unavailable")

        blob = dynamic_and_section_relr_fixture()
        expected = [0x1300, 0x1304, 0x130c, 0x1400]

        # This call uses PT_DYNAMIC/PT_LOAD and must not depend on section data.
        report = relr_audit.audit(blob)
        actual = [entry["target_offset"]
                  for entry in report["relr_table"]["entries"]]
        self.assertEqual(actual, expected)

        with tempfile.NamedTemporaryFile(suffix=".elf") as file:
            file.write(blob)
            file.flush()
            result = subprocess.run(
                [readelf, "-rW", file.name],
                check=True, capture_output=True, text=True,
            )
        readelf_targets = {
            int(value, 16)
            for value in re.findall(
                r"(?m)^\s*([0-9a-fA-F]{8})\s*$", result.stdout
            )
        }
        self.assertEqual(readelf_targets, set(expected), result.stdout)

    def test_unified_reconciliation_preserves_dynamic_relr_records(self):
        report = reconciliation.audit(dynamic_and_section_relr_fixture())
        relr_tables = [table for table in report["tables"]
                       if table["encoding"] == "RELR"]
        self.assertEqual(len(relr_tables), 1)
        entries = relr_tables[0]["entries"]
        self.assertEqual([entry["target"] for entry in entries],
                         [0x1300, 0x1304, 0x130c, 0x1400])
        self.assertTrue(all(entry["symbol_index"] is None for entry in entries))
        self.assertTrue(all(entry["addend"] is None for entry in entries))


if __name__ == "__main__":
    unittest.main()
