import importlib.util, struct, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; sys.path.insert(0,str(ROOT/"tools"))
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path); module=importlib.util.module_from_spec(spec)
    assert spec and spec.loader; spec.loader.exec_module(module); return module
fixture=load("dynamic_fixture",ROOT/"tests"/"test_guest_arm_dynamic_import_inventory.py")
plt=load("guest_arm_plt_got_audit",ROOT/"tools"/"guest_arm_plt_got_audit.py")
class PltGotCrossReferenceTests(unittest.TestCase):
    def test_jump_slot_is_correlated_to_undefined_dynamic_symbol(self):
        report=plt.audit_plt_got(fixture.make_elf())
        self.assertEqual(report["jump_slot_count"],1)
        slot=report["jump_slots"][0]
        self.assertEqual(slot["relocation_type"],"R_ARM_JUMP_SLOT")
        self.assertEqual(slot["symbol"],"external_api")
        self.assertTrue(slot["symbol_undefined"])
    def test_missing_plt_tags_remain_unknown(self):
        report=plt.audit_plt_got(fixture.make_elf())
        self.assertIsNone(report["dynamic_plt_tags"]["DT_PLTGOT"])
        self.assertIsNone(report["dynamic_plt_tags"]["DT_JMPREL"])
        self.assertEqual(report["dynamic_plt_tags"]["DT_PLTREL_kind"],"unknown")
    def test_no_jump_slot_is_reported_as_empty_inventory(self):
        data=bytearray(fixture.make_elf())
        # The fixture relocation's low byte is the relocation type (22 = JUMP_SLOT).
        elf=load("guest_elf_audit_for_plt_test",ROOT/"tools"/"guest_arm_elf_audit.py").audit_elf(bytes(data))
        relsec=next(s for s in elf["sections"] if s["type"]==9)
        struct.pack_into("<I",data,relsec["offset"]+4,(1<<8)|2)
        report=plt.audit_plt_got(bytes(data))
        self.assertEqual(report["jump_slot_count"],0)
if __name__=="__main__": unittest.main()
