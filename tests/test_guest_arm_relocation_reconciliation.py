import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import guest_arm_relocation_reconciliation as module
from test_guest_arm_aps2_relocation_audit import elf_fixture

def entry(target=0x1304,typ=22,sym=1,addend=None,namespace="DT_SYMTAB@0x1000"):
    return {"index":0,"target_domain":"virtual","target":target,"symbol_index":sym,
        "symbol":"foo","type":typ,"type_name":"R_ARM_JUMP_SLOT","addend":addend,
        "symbol_namespace":namespace}
def table(source,offset,size,entries,kind="REL"):
    return {"source_table":source,"encoding":kind,"kind":kind,"file_offset":offset,
        "size":size,"entry_count":len(entries),"entries":entries}

class ReconciliationTests(unittest.TestCase):
    def test_exact_duplicates_preserve_raw_records(self):
        e=entry(); r=module.reconcile_tables([table("DT_REL",0x200,8,[e]),table("DT_JMPREL",0x200,8,[e])])
        self.assertEqual(r["raw_relocation_count"],2)
        self.assertEqual(r["duplicate_group_count"],1)
        self.assertEqual(r["file_range_overlaps"][0]["relationship"],"exact_same_range")
    def test_partial_overlap(self):
        r=module.reconcile_tables([table("DT_REL",0x200,16,[]),table("DT_ANDROID_REL",0x20c,12,[])])
        self.assertEqual(r["file_range_overlaps"][0]["relationship"],"partial_overlap")
        self.assertEqual(r["file_range_overlaps"][0]["overlap_size"],4)
    def test_different_symbol_namespaces_are_candidate_only(self):
        r=module.reconcile_tables([table("SHT_REL:.rel.dyn",0x200,8,[entry(namespace="SHT_SYMTAB:3")]),
            table("DT_REL",0x200,8,[entry(namespace="DT_SYMTAB@0x1000")])])
        self.assertEqual(r["duplicate_group_count"],0)
        self.assertEqual(r["namespace_ambiguous_match_count"],1)
    def test_different_addends_not_duplicate(self):
        r=module.reconcile_tables([table("DT_RELA",0x200,12,[entry(addend=4)],"RELA"),
            table("DT_ANDROID_RELA",0x220,8,[entry(addend=8)],"RELA")])
        self.assertEqual(r["duplicate_group_count"],0)
    def test_integrated_aps2_audit(self):
        r=module.audit(elf_fixture("REL"))
        self.assertEqual(r["reconciliation"]["raw_relocation_count"],2)
        self.assertEqual(r["tables"][0]["encoding"],"APS2_REL")
        self.assertTrue(r["section_backed_audit"]["available"])
    def test_rejects_non_elf(self):
        with self.assertRaises(module.ELFAuditError): module.audit(b"not ELF")

if __name__=="__main__": unittest.main()
