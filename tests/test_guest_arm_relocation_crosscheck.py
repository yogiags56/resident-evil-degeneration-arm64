import importlib.util, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
MODULE=ROOT/"tools"/"guest_arm_relocation_crosscheck.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_relocation_crosscheck",MODULE)
cross=importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(cross)
def fixtures():
    elf={"file_size":128,"sections":[{"index":2,"name":".text","size":12,"flags":4}]}
    reloc={"sections":[{"name":".rel.text","target_section_index":2,"entries":[
      {"target_section_offset":0,"type":28,"type_name":"R_ARM_CALL","symbol":"internal","symbol_index":1,"symbol_section_index":2},
      {"target_section_offset":4,"type":2,"type_name":"R_ARM_ABS32","symbol":"literal","symbol_index":2,"symbol_section_index":2},
      {"target_section_offset":8,"type":28,"type_name":"R_ARM_CALL","symbol":"external","symbol_index":3,"symbol_section_index":0}]}]}
    spans={2:[{"kind":"a","start":0,"end":4,"mapping_symbol":"$a"},{"kind":"d","start":4,"end":8,"mapping_symbol":"$d"},{"kind":"t","start":8,"end":12,"mapping_symbol":"$t"}]}
    return elf,reloc,spans
class RelocationMappingCrossCheckTests(unittest.TestCase):
    def test_classifies_arm_data_and_thumb(self):
        elf,reloc,spans=fixtures(); result=cross._cross_check_reports(elf,reloc,spans,"abc")
        self.assertEqual([x["site_classification"] for x in result["findings"]],["arm_code","data","thumb_code"])
        self.assertEqual(result["classification_counts"]["call_jump_in_code"],2)
    def test_warns_for_call_in_data(self):
        elf,reloc,spans=fixtures(); reloc["sections"][0]["entries"][0]["target_section_offset"]=4
        result=cross._cross_check_reports(elf,reloc,spans,"abc")
        self.assertIn("warning",result["findings"][0]); self.assertEqual(result["classification_counts"]["call_jump_outside_code"],1)
    def test_undefined_symbol_not_assumed_to_be_s3e_import(self):
        elf,reloc,spans=fixtures(); result=cross._cross_check_reports(elf,reloc,spans,"abc")
        self.assertFalse(result["findings"][2]["symbol_defined"])
        self.assertIn("resolver target not established",result["findings"][2]["symbol_status"])
    def test_unmapped_executable_range_remains_unknown(self):
        elf,reloc,spans=fixtures(); spans[2]=[{"kind":"unknown","start":0,"end":12}]
        result=cross._cross_check_reports(elf,reloc,spans,"abc")
        self.assertEqual(result["classification_counts"]["unknown"],3)
    def test_non_executable_section_not_called_code(self):
        elf,reloc,spans=fixtures(); elf["sections"][0]["flags"]=0
        result=cross._cross_check_reports(elf,reloc,spans,"abc")
        self.assertEqual(result["classification_counts"]["non_executable"],3)
if __name__=="__main__": unittest.main()
