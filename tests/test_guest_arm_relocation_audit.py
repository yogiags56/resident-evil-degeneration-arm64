import importlib.util, struct, sys, unittest
from pathlib import Path
MODULE=Path(__file__).resolve().parents[1]/"tools"/"guest_arm_relocation_audit.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_relocation_audit",MODULE)
audit=importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.path.insert(0,str(MODULE.parent)); SPEC.loader.exec_module(audit)
def make_elf32(relocs=((0,(1<<8)|28),(4,(1<<8)|29)),bad_symbol=False,rel_type=9):
    shnames=b"\0.shstrtab\0.text\0.strtab\0.symtab\0.rel.text\0"; strings=b"\0target_func\0"
    symbols=bytes(16)+struct.pack("<IIIBBH",1,0x1000,8,0x12,0,2)
    reloc=b"".join(struct.pack("<II",off,info) for off,info in relocs)
    if bad_symbol: reloc=struct.pack("<II",0,(9<<8)|28)
    eh,sh=52,40; count=6; shoff=eh; cursor=shoff+count*sh
    shstr_off=cursor; cursor+=len(shnames); text_off=cursor; cursor+=8
    str_off=cursor; cursor+=len(strings); sym_off=cursor; cursor+=len(symbols); rel_off=cursor
    ident=b"\x7fELF"+bytes([1,1,1])+bytes(9)
    header=struct.pack("<HHIIIIIHHHHHH",1,40,1,0,0,shoff,0,eh,0,0,sh,count,1)
    shdr=[bytes(sh),struct.pack("<IIIIIIIIII",1,3,0,0,shstr_off,len(shnames),0,0,1,0),
      struct.pack("<IIIIIIIIII",11,1,6,0,text_off,8,0,0,4,0),struct.pack("<IIIIIIIIII",19,3,0,0,str_off,len(strings),0,0,1,0),
      struct.pack("<IIIIIIIIII",27,2,0,0,sym_off,len(symbols),3,1,4,16),
      struct.pack("<IIIIIIIIII",35,rel_type,0,0,rel_off,len(reloc),4,2,4,8)]
    return ident+header+b"".join(shdr)+shnames+b"\0"*8+strings+symbols+reloc
class GuestArmRelocationAuditTests(unittest.TestCase):
    def test_rel_entries_resolve_symbol_and_names(self):
        result=audit.audit_relocations(make_elf32()); self.assertEqual(result["relocation_count"],2)
        entries=result["sections"][0]["entries"]; self.assertEqual(entries[0]["type_name"],"R_ARM_CALL")
        self.assertEqual(entries[1]["type_name"],"R_ARM_JUMP24"); self.assertEqual(entries[0]["symbol"],"target_func")
        self.assertIsNone(entries[0]["addend"])
    def test_unknown_relocation_preserved(self):
        result=audit.audit_relocations(make_elf32(((0,(1<<8)|250),)))
        self.assertEqual(result["sections"][0]["entries"][0]["type_name"],"R_ARM_UNKNOWN_250")
    def test_bad_symbol_index_rejected(self):
        with self.assertRaises(audit.ELFAuditError): audit.audit_relocations(make_elf32(bad_symbol=True))
    def test_bad_relocation_section_link_rejected(self):
        data=bytearray(make_elf32()); struct.pack_into("<I",data,52+5*40+24,99)
        with self.assertRaises(audit.ELFAuditError): audit.audit_relocations(bytes(data))
    def test_bad_target_offset_rejected(self):
        with self.assertRaises(audit.ELFAuditError): audit.audit_relocations(make_elf32(((8,(1<<8)|28),)))
    def test_rela_addend(self):
        data=bytearray(make_elf32(rel_type=4)); rel_off=len(data)-16
        data=data[:rel_off]+struct.pack("<IIi",4,(1<<8)|28,-12)
        sh=52+5*40; struct.pack_into("<I",data,sh+20,12); struct.pack_into("<I",data,sh+36,12)
        result=audit.audit_relocations(bytes(data)); self.assertEqual(result["sections"][0]["entries"][0]["addend"],-12)
if __name__=="__main__": unittest.main()
