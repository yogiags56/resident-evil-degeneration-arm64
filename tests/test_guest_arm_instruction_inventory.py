import importlib.util, struct, sys, types, unittest
from pathlib import Path
MODULE=Path(__file__).resolve().parents[1]/"tools"/"guest_arm_instruction_inventory.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_instruction_inventory",MODULE)
inventory=importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.path.insert(0,str(MODULE.parent)); SPEC.loader.exec_module(inventory)
class FakeInstruction:
    def __init__(self,address,size,mnemonic,op_str=""): self.address=address; self.size=size; self.mnemonic=mnemonic; self.op_str=op_str
class FakeEngine:
    def __init__(self,mode): self.mode=mode; self.detail=True
    def disasm(self,data,address,count=1):
        if data.startswith(b"\xaa\xbb"): yield FakeInstruction(address,2,"mov","r0, r1")
        elif data.startswith(b"\xcc\xdd\xee\xff"): yield FakeInstruction(address,4,"bl","0x1234")
class FakeCapstone(types.SimpleNamespace):
    def __init__(self): super().__init__(CS_ARCH_ARM=1,CS_MODE_ARM=0,CS_MODE_THUMB=16); self.__version__="test-fake"
    def Cs(self,arch,mode): self.last_mode=mode; return FakeEngine(mode)
def make_elf32(code=b"",symbols=(),flags=0x6):
    shnames=b"\0.shstrtab\0.text\0.strtab\0.symtab\0"; names=b"\0$a\0$t\0$d\0"
    sym_entries=[bytes(16)]
    for name,value in symbols: sym_entries.append(struct.pack("<IIIBBH",names.index(name.encode()+b"\0"),value,0,0,0,2))
    symtab=b"".join(sym_entries); eh,sh=52,40; shoff=eh; n=5; cursor=shoff+n*sh
    shstr_off=cursor; cursor+=len(shnames); text_off=cursor; cursor+=len(code); str_off=cursor; cursor+=len(names); sym_off=cursor
    ident=b"\x7fELF"+bytes([1,1,1])+bytes(9)
    header=struct.pack("<HHIIIIIHHHHHH",1,40,1,0,0,shoff,0,eh,0,0,sh,n,1)
    shdr=[bytes(sh),struct.pack("<IIIIIIIIII",1,3,0,0,shstr_off,len(shnames),0,0,1,0),
      struct.pack("<IIIIIIIIII",11,1,flags,0,text_off,len(code),0,0,4,0),
      struct.pack("<IIIIIIIIII",19,3,0,0,str_off,len(names),0,0,1,0),
      struct.pack("<IIIIIIIIII",27,2,0,0,sym_off,len(symtab),3,1,4,16)]
    return ident+header+b"".join(shdr)+shnames+code+names+symtab
class GuestArmInstructionInventoryTests(unittest.TestCase):
    def test_mapping_symbols_split_arm_thumb_and_data(self):
        data=make_elf32(b"\xaa\xbb\xcc\xdd\xee\xff\x00\x00",(("$a",0),("$t",4),("$d",6)))
        result=inventory.inventory(data,None,FakeCapstone()); ranges=result["sections"][0]["ranges"]
        self.assertEqual([r["kind"] for r in ranges],["a","t","d"]); self.assertEqual([r["size"] for r in ranges],[4,2,2])
        self.assertEqual(result["summary"]["decoded_instructions"],2); self.assertEqual(result["summary"]["mapping_data_bytes"],2)
        self.assertEqual(result["summary"]["unmapped_bytes"],0); self.assertEqual(result["mode_source"],"ELF mapping symbols")
    def test_unknown_prefix_is_not_guessed(self):
        data=make_elf32(b"\xaa\xbb\xcc\xdd",(("$t",2),)); result=inventory.inventory(data,None,FakeCapstone())
        self.assertEqual(result["summary"]["unmapped_bytes"],2)
    def test_fallback_mode_required_without_mapping_symbols(self):
        data=make_elf32(b"\xaa\xbb")
        with self.assertRaises(inventory.ELFAuditError): inventory.inventory(data,None,FakeCapstone())
        self.assertEqual(inventory.inventory(data,"thumb",FakeCapstone())["mode_source"],"explicit fallback")
    def test_conflicting_mapping_symbols_at_same_offset_rejected(self):
        data=make_elf32(b"\xaa\xbb\xcc\xdd",(("$a",0),("$t",0)))
        with self.assertRaises(inventory.ELFAuditError): inventory.inventory(data,None,FakeCapstone())
    def test_non_arm_elf_rejected(self):
        b=bytearray(make_elf32(b"\xaa\xbb")); struct.pack_into("<H",b,18,183)
        with self.assertRaises(inventory.ELFAuditError): inventory.inventory(bytes(b),"arm",FakeCapstone())
if __name__=="__main__": unittest.main()
