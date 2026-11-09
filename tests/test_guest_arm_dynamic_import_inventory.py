import importlib.util, struct, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; sys.path.insert(0,str(ROOT/"tools"))
MODULE=ROOT/"tools"/"guest_arm_dynamic_import_inventory.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_dynamic_import_inventory",MODULE)
module=importlib.util.module_from_spec(SPEC); assert SPEC and SPEC.loader; SPEC.loader.exec_module(module)
def make_elf():
    shnames=b"\0.shstrtab\0.text\0.dynstr\0.dynsym\0.dynamic\0.rel.dyn\0"
    strings=b"\0libfixture.so\0external_api\0"
    liboff=strings.index(b"libfixture.so"); symoff=strings.index(b"external_api")
    dynsym=bytes(16)+struct.pack("<IIIBBH",symoff,0,0,0x12,0,0)
    dynamic=struct.pack("<iI",1,liboff)+struct.pack("<iI",0,0)
    rel=struct.pack("<II",0x1000,(1<<8)|22)
    eh,sh=52,40; count=7; cursor=eh+count*sh
    shstr_off=cursor; cursor+=len(shnames)
    text_off=cursor; cursor+=16
    str_off=cursor; cursor+=len(strings)
    sym_off=cursor; cursor+=len(dynsym)
    dyn_off=cursor; cursor+=len(dynamic)
    rel_off=cursor
    ident=b"\x7fELF"+bytes([1,1,1])+bytes(9)
    header=struct.pack("<HHIIIIIHHHHHH",3,40,1,0,0,eh,0,eh,0,0,sh,count,1)
    headers=[bytes(sh),
      struct.pack("<IIIIIIIIII",1,3,0,0,shstr_off,len(shnames),0,0,1,0),
      struct.pack("<IIIIIIIIII",11,1,6,0x1000,text_off,16,0,0,4,0),
      struct.pack("<IIIIIIIIII",17,3,0,0,str_off,len(strings),0,0,1,0),
      struct.pack("<IIIIIIIIII",25,11,0,0,sym_off,len(dynsym),3,1,4,16),
      struct.pack("<IIIIIIIIII",33,6,0,0,dyn_off,len(dynamic),3,0,4,8),
      struct.pack("<IIIIIIIIII",42,9,0,0,rel_off,len(rel),4,2,4,8)]
    return ident+header+b"".join(headers)+shnames+bytes(16)+strings+dynsym+dynamic+rel
class DynamicImportInventoryTests(unittest.TestCase):
    def test_reads_undefined_dynamic_symbols_dependencies_and_relocations(self):
        report=module.inventory(make_elf())
        self.assertEqual(report["dynamic_dependencies"],["libfixture.so"])
        self.assertEqual(report["undefined_dynamic_symbol_count"],1)
        sym=report["dynamic_symbol_tables"][0]["undefined_symbols"][0]
        self.assertEqual(sym["name"],"external_api"); self.assertTrue(sym["undefined"])
        rel=report["dynamic_relocation_sections"][0]["entries"][0]
        self.assertEqual(rel["type_name"],"R_ARM_JUMP_SLOT")
        self.assertEqual(rel["symbol"],"external_api")
    def test_preserves_unknown_dynamic_tags(self):
        data=make_elf(); elf=module.audit_elf(data)
        dynamic=next(s for s in elf["sections"] if s["type"]==6)
        raw=bytearray(data); struct.pack_into("<iI",raw,dynamic["offset"],0x60001234,7)
        report=module.inventory(bytes(raw))
        tag=report["dynamic_sections"][0]["entries"][0]
        self.assertEqual(tag["tag_name"],"DT_UNKNOWN_1610621492")
    def test_rejects_invalid_dynamic_symbol_string_table_link(self):
        data=bytearray(make_elf()); elf=module.audit_elf(bytes(data))
        dynsym=next(s for s in elf["sections"] if s["type"]==11)
        shoff=struct.unpack_from("<I",data,32)[0]
        struct.pack_into("<I",data,shoff+dynsym["index"]*40+24,1)
        with self.assertRaises(module.ELFAuditError): module.inventory(bytes(data))
    def test_rejects_dynamic_table_without_dt_null(self):
        data=bytearray(make_elf()); elf=module.audit_elf(bytes(data))
        dynamic=next(s for s in elf["sections"] if s["type"]==6)
        struct.pack_into("<iI",data,dynamic["offset"]+8,1,0)
        with self.assertRaises(module.ELFAuditError): module.inventory(bytes(data))
if __name__=="__main__": unittest.main()
