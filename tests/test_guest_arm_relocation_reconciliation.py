import sys, unittest, struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import guest_arm_relocation_reconciliation as module
from test_guest_arm_aps2_relocation_audit import elf_fixture, elf_fixture_with_symbols, packed as aps2_packed
from test_guest_arm_relr_audit import fixture as relr_fixture
from test_guest_arm_dynamic_relocation_segment_audit import fixture as dynamic_fixture


def combined_dynamic_aps2_fixture():
    data=bytearray(dynamic_fixture())
    # Mark dynamic symbol #1 as a defined 37-byte object.
    struct.pack_into("<I",data,0x1e0+8,37)
    struct.pack_into("<H",data,0x1e0+14,2)
    # Expand PT_LOAD memory to include the packed relocation targets in BSS.
    struct.pack_into("<I",data,52+20,0x400)
    # Append APS2 tags before the new DT_NULL entry.
    struct.pack_into("<IIIIIIII",data,84,2,0x100,0x1100,0,112,112,4,4)
    tags=[(0x6000000f,0x1220),(0x60000010,len(aps2_packed("REL",20))),(0,0)]
    for i,(tag,value) in enumerate(tags):
        struct.pack_into("<iI",data,0x100+(11+i)*8,tag,value)
    # Make the conventional DT_REL entry R_ARM_COPY as well.
    struct.pack_into("<I",data,0x204,(1<<8)|20)
    blob=aps2_packed("REL",20)
    data[0x220:0x220+len(blob)]=blob
    return bytes(data)

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
        self.assertFalse(r["section_backed_audit"]["available"])
        self.assertIn("no section headers",r["section_backed_audit"]["reason"])

    def test_symbol_versions_participate_in_duplicate_identity(self):
        old=entry(); old["symbol_version"]={"index":2,"kind":"requirement","name":"VER_1","library":"libfoo.so","hidden":False}
        new=dict(old); new["symbol_version"]={"index":3,"kind":"requirement","name":"VER_2","library":"libfoo.so","hidden":False}
        r=module.reconcile_tables([table("DT_REL",0x200,8,[old]),table("DT_JMPREL",0x200,8,[new])])
        self.assertEqual(r["duplicate_group_count"],0)
        self.assertEqual(r["namespace_ambiguous_match_count"],0)

    def test_same_version_different_namespaces_remains_candidate_only(self):
        a=entry(namespace="DT_SYMTAB@0x1000")
        b=entry(namespace="SHT_SYMTAB:3")
        version={"index":2,"kind":"requirement","name":"VER_1","library":"libfoo.so","hidden":False}
        a["symbol_version"]=version; b["symbol_version"]=dict(version)
        r=module.reconcile_tables([table("DT_REL",0x200,8,[a]),table("SHT_REL:.rel.dyn",0x200,8,[b])])
        self.assertEqual(r["duplicate_group_count"],0)
        self.assertEqual(r["namespace_ambiguous_match_count"],1)


    def test_dynamic_rel_and_aps2_share_proven_symbol_metadata_namespace(self):
        report=module.audit(combined_dynamic_aps2_fixture())
        ordinary=[t for t in report["tables"] if t["source_table"]=="DT_REL"][0]["entries"][0]
        packed=[t for t in report["tables"] if t["encoding"]=="APS2_REL"][0]["entries"][0]
        for field in ("symbol","symbol_size","symbol_defined","symbol_section_index","symbol_version","symbol_namespace"):
            self.assertEqual(ordinary.get(field),packed.get(field),field)
        self.assertEqual(packed["symbol_size"],37)
        self.assertTrue(packed["symbol_defined"])
        self.assertEqual(packed["symbol_namespace"],"DT_SYMTAB@0x000011d0")
        self.assertEqual(packed["target_access_width"],37)

    def test_aps2_symbol_size_and_namespace_reach_copy_target_audit(self):
        report=module.audit(elf_fixture_with_symbols("REL",20,37,2))
        aps=[t for t in report["tables"] if t["encoding"]=="APS2_REL"][0]
        entry=aps["entries"][0]
        self.assertEqual(entry["symbol"],"foo")
        self.assertEqual(entry["symbol_size"],37)
        self.assertTrue(entry["symbol_defined"])
        self.assertEqual(entry["symbol_namespace"],"DT_SYMTAB@0x000011d0")
        self.assertEqual(entry["target_access_width"],37)
        self.assertEqual(entry["target_range"]["width_source"],"validated_symbol_st_size")

    def test_aps2_copy_symbol_size_is_not_used_for_undefined_symbol(self):
        report=module.audit(elf_fixture_with_symbols("REL",20,37,0))
        aps=[t for t in report["tables"] if t["encoding"]=="APS2_REL"][0]
        entry=aps["entries"][0]
        self.assertEqual(entry["symbol_size"],37)
        self.assertFalse(entry["symbol_defined"])
        self.assertEqual(entry["symbol_section_index"],0)
        self.assertIsNone(entry["target_access_width"])
        self.assertEqual(entry["target_range"]["width_source"],"unresolved_copy_symbol_size")

    def test_aps2_namespace_remains_unresolved_without_bounded_hash(self):
        report=module.audit(elf_fixture("REL",20))
        aps=[t for t in report["tables"] if t["encoding"]=="APS2_REL"][0]
        entry=aps["entries"][0]
        self.assertEqual(entry["symbol_namespace"],"DT_SYMTAB:unresolved")
        self.assertIsNone(entry.get("symbol_size"))
        self.assertFalse(entry["target_range"]["width_known"])


    def test_integrates_android_relr_aliases_in_full_reconciliation(self):
        from guest_arm_relr import DT_ANDROID_RELR, DT_ANDROID_RELRSZ, DT_ANDROID_RELRENT
        report=module.audit(relr_fixture(tag_triplet=(
            DT_ANDROID_RELR,DT_ANDROID_RELRSZ,DT_ANDROID_RELRENT)))
        relr=[t for t in report["tables"] if t["encoding"]=="RELR"][0]
        self.assertEqual(relr["entry_count"],4)
        self.assertEqual(relr["entries"][0]["type_name"],"R_ARM_RELATIVE")
        self.assertEqual(report["relr_relocation_count"],4)

    def test_integrates_relr_without_fabricating_symbol_or_addend(self):
        report=module.audit(relr_fixture())
        self.assertEqual(report["relr_relocation_count"],4)
        relr=[t for t in report["tables"] if t["encoding"]=="RELR"][0]
        self.assertEqual(relr["entry_count"],4)
        self.assertIsNone(relr["entries"][0]["symbol_index"])
        self.assertIsNone(relr["entries"][0]["addend"])
        self.assertEqual(relr["entries"][1]["relr_encoding"],"bitmap")


    def test_integrates_target_range_report_and_bss_classification(self):
        data=bytearray(relr_fixture())
        # PT_LOAD p_memsz extends beyond p_filesz, creating a BSS range.
        import struct
        struct.pack_into("<I",data,52+20,0x500)
        report=module.audit(bytes(data))
        targets=report["target_range_audit"]["targets"]
        self.assertEqual(report["target_range_audit"]["target_count"],4)
        self.assertGreater(report["relocation_abi"]["registry_entry_count"],40)
        self.assertTrue(report["relocation_abi"]["unknown_types_preserved"])
        self.assertEqual(targets[0]["status"],"zero_fill_bss")
        relr=[t for t in report["tables"] if t["encoding"]=="RELR"][0]
        self.assertEqual(relr["entries"][0]["target_range"]["status"],"zero_fill_bss")
        self.assertEqual(relr["entries"][0]["target_access_width"],4)

    def test_rejects_non_elf(self):
        with self.assertRaises(module.ELFAuditError): module.audit(b"not ELF")

if __name__=="__main__": unittest.main()
