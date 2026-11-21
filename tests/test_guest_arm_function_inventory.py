import importlib.util, struct, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; sys.path.insert(0,str(ROOT/"tools"))
MODULE=ROOT/"tools"/"guest_arm_function_inventory.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_function_inventory",MODULE)
module=importlib.util.module_from_spec(SPEC); assert SPEC and SPEC.loader; SPEC.loader.exec_module(module)
def make_elf(functions):
    names=b"\0.shstrtab\0.text\0.strtab\0.symtab\0.rel.text\0"
    strings=b"\0arm_func\0thumb_func\0data_func\0external_func\0zero_func\0"
    nameoff={n:strings.index(n.encode()) for n in ("arm_func","thumb_func","data_func","external_func","zero_func")}
    symbols=[bytes(16)]
    for name,value,size,shndx in functions:
        symbols.append(struct.pack("<IIIBBH",nameoff[name],value,size,0x12,0,shndx))
    symdata=b"".join(symbols); rel=struct.pack("<II",0,(2<<8)|28)
    eh,sh=52,40; count=6; cursor=eh+count*sh
    shstr_off=cursor; cursor+=len(names); text_off=cursor; cursor+=16
    str_off=cursor; cursor+=len(strings); sym_off=cursor; cursor+=len(symdata); rel_off=cursor
    ident=b"\x7fELF"+bytes([1,1,1])+bytes(9)
    header=struct.pack("<HHIIIIIHHHHHH",1,40,1,0,0,eh,0,eh,0,0,sh,count,1)
    headers=[bytes(sh),struct.pack("<IIIIIIIIII",1,3,0,0,shstr_off,len(names),0,0,1,0),
      struct.pack("<IIIIIIIIII",11,1,6,0,text_off,16,0,0,4,0),
      struct.pack("<IIIIIIIIII",19,3,0,0,str_off,len(strings),0,0,1,0),
      struct.pack("<IIIIIIIIII",27,2,0,0,sym_off,len(symdata),3,1,4,16),
      struct.pack("<IIIIIIIIII",35,9,0,0,rel_off,len(rel),4,2,4,8)]
    return ident+header+b"".join(headers)+names+bytes(16)+strings+symdata+rel
class FunctionSymbolInventoryTests(unittest.TestCase):
    def test_symbols_classified_arm_thumb_data_undefined_and_zero_size(self):
        data=make_elf([("arm_func",0,4,2),("thumb_func",5,4,2),("data_func",8,4,2),("external_func",0,0,0),("zero_func",12,0,2)])
        elf=module.audit_elf(data); symbols=module.parse_function_symbols(data,elf)
        mappings={2:[{"kind":"a","offset":0,"name":"$a"},{"kind":"t","offset":4,"name":"$t"},{"kind":"d","offset":8,"name":"$d"}]}
        counts=module._classify_symbols(symbols,elf,mappings)
        self.assertEqual([s["mapping_classification"] for s in symbols],["arm","thumb","data","unresolved","zero-size"])
        self.assertTrue(symbols[1]["thumb_state_bit"]); self.assertEqual(symbols[1]["start_offset"],4)
        self.assertEqual(counts["arm"],1); self.assertEqual(counts["thumb"],1)
    def test_function_extent_crossing_modes_is_mixed(self):
        data=make_elf([("arm_func",2,5,2)])
        elf=module.audit_elf(data); symbols=module.parse_function_symbols(data,elf)
        module._classify_symbols(symbols,elf,{2:[{"kind":"a","offset":0,"name":"$a"},{"kind":"t","offset":4,"name":"$t"}]})
        self.assertEqual(symbols[0]["mapping_classification"],"mixed")
    def test_out_of_section_extent_is_reported(self):
        data=make_elf([("arm_func",12,8,2)])
        elf=module.audit_elf(data); symbols=module.parse_function_symbols(data,elf)
        module._classify_symbols(symbols,elf,{2:[{"kind":"a","offset":0,"name":"$a"}]})
        self.assertEqual(symbols[0]["mapping_classification"],"extends-past-section")
    def test_undefined_function_symbol_retains_external_status(self):
        data=make_elf([("external_func",0,0,0)])
        elf=module.audit_elf(data); symbols=module.parse_function_symbols(data,elf)
        self.assertEqual(symbols[0]["section_status"],"undefined/external")
    def test_bad_symbol_table_link_is_rejected(self):
        data=bytearray(make_elf([("arm_func",0,4,2)])); struct.pack_into("<I",data,52+4*40+24,99)
        with self.assertRaises(module.ELFAuditError): module.parse_function_symbols(bytes(data),module.audit_elf(bytes(data)))
if __name__=="__main__": unittest.main()
