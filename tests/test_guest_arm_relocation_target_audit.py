import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import guest_arm_relocation_target_audit as module
from guest_arm_elf_audit import ELFAuditError

LOAD={"index":0,"type":1,"offset":0x100,"vaddr":0x1000,"filesz":0x100,"memsz":0x200}
class TargetRangeTests(unittest.TestCase):
    def test_classifies_file_backed_target(self):
        r=module.classify_target(0x1080,4,[LOAD])
        self.assertEqual(r["status"],"file_backed")
        self.assertEqual(r["matches"][0]["target_file_offset"],0x180)

    def test_classifies_zero_fill_bss_target(self):
        r=module.classify_target(0x1180,4,[LOAD])
        self.assertEqual(r["status"],"zero_fill_bss")
        self.assertIsNone(r["matches"][0]["target_file_offset"])

    def test_classifies_target_crossing_file_to_bss(self):
        r=module.classify_target(0x10fe,4,[LOAD])
        self.assertEqual(r["status"],"crosses_file_backed_into_bss")

    def test_classifies_target_crossing_segment_end(self):
        r=module.classify_target(0x11fe,4,[LOAD])
        self.assertEqual(r["status"],"crosses_segment_end")

    def test_reports_target_outside_all_load_segments(self):
        r=module.classify_target(0x1200,4,[LOAD])
        self.assertEqual(r["status"],"outside_all_pt_load_segments")

    def test_reports_overlapping_load_segments_as_ambiguous(self):
        other={**LOAD,"index":1,"offset":0x300,"vaddr":0x1080,"filesz":0x80,"memsz":0x100}
        r=module.classify_target(0x1090,4,[LOAD,other])
        self.assertEqual(r["status"],"ambiguous_overlapping_pt_load_segments")
        self.assertEqual(len(r["matches"]),2)

    def test_unknown_relocation_width_is_not_guessed(self):
        r=module.classify_target(0x1080,None,[LOAD])
        self.assertEqual(r["status"],"unknown_width_file_backed")

    def test_rejects_elf32_range_overflow(self):
        r=module.classify_target(0xfffffffe,4,[LOAD])
        self.assertEqual(r["status"],"target_range_overflows_elf32")

    def test_r_arm_none_does_not_access_target_memory(self):
        r=module.classify_target(0xdeadbeef,0,[LOAD])
        self.assertEqual(r["status"],"no_target_access")
        self.assertEqual(r["matches"],[])

    def test_rejects_invalid_width(self):
        with self.assertRaises(ELFAuditError):
            module.classify_target(0x1080,0,[LOAD])

if __name__=="__main__": unittest.main()
