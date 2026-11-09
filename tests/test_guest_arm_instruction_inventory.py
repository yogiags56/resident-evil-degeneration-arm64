import importlib.util, struct, sys, types, unittest
from pathlib import Path
MODULE=Path(__file__).resolve().parents[1]/"tools"/"guest_arm_instruction_inventory.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_instruction_inventory",MODULE)
inventory=importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.path.insert(0,str(MODULE.parent))
SPEC.loader.exec_module(inventory)

class FakeInstruction:
    def __init__(self,address,size,mnemonic,op_str=""):
        self.address=address; self.size=size; self.mnemonic=mnemonic; self.op_str=op_str

class FakeEngine:
    def __init__(self,mode): self.mode=mode; self.detail=True
    def disasm(self,data,address,count=1):
        if data.startswith(b"\xaa\xbb"):
            yield FakeInstruction(address,2,"mov","r0, r1")
        elif data.startswith(b"\xcc\xdd\xee\xff"):
            yield FakeInstruction(address,4,"bl","0x1234")

class FakeCapstone(types.SimpleNamespace):
    def __init__(self):
        super().__init__(CS_ARCH_ARM=1,CS_MODE_ARM=0,CS_MODE_THUMB=16)
        self.__version__="test-fake"
    def Cs(self,arch,mode):
        self.last_mode=mode
        return FakeEngine(mode)

def make_elf32(code=b"",flags=0x6):
    names=b"\0.shstrtab\0.text\0"
    eh,sh=52,40; shoff=eh; noff=shoff+3*sh; coff=noff+len(names)
    ident=b"\x7fELF"+bytes([1,1,1])+bytes(9)
    header=struct.pack("<HHIIIIIHHHHHH",2,40,1,0,0,shoff,0,eh,0,0,sh,3,1)
    null=bytes(sh)
    shstr=struct.pack("<IIIIIIIIII",1,3,0,0,noff,len(names),0,0,1,0)
    text=struct.pack("<IIIIIIIIII",11,1,flags,0x1000,coff,len(code),0,0,4,0)
    return ident+header+null+shstr+text+names+code

class GuestArmInstructionInventoryTests(unittest.TestCase):
    def test_thumb_counts_and_undecoded_gaps(self):
        fake=FakeCapstone()
        result=inventory.inventory(make_elf32(b"\xaa\xbb\x00\x00\xcc\xdd\xee\xff"),"thumb",fake)
        self.assertEqual(result["decoder"]["version"],"test-fake")
        self.assertEqual(result["summary"]["decoded_instructions"],2)
        self.assertEqual(result["summary"]["decoded_bytes"],6)
        self.assertEqual(result["summary"]["undecoded_bytes"],2)
        self.assertEqual(result["summary"]["mnemonics"],{"bl":1,"mov":1})
        self.assertEqual(result["sections"][0]["undecoded_ranges"][0]["offset"],2)
        self.assertEqual(fake.last_mode,16)
    def test_arm_mode_is_explicit(self):
        fake=FakeCapstone()
        result=inventory.inventory(make_elf32(b"\xaa\xbb\xcc\xdd\xee\xff"),"arm",fake)
        self.assertEqual(result["mode"],"arm")
        self.assertEqual(fake.last_mode,0)
    def test_non_arm_elf_rejected(self):
        b=bytearray(make_elf32(b"\xaa\xbb"))
        struct.pack_into("<H",b,18,183)
        with self.assertRaises(inventory.ELFAuditError):
            inventory.inventory(bytes(b),"arm",FakeCapstone())
    def test_non_executable_section_ignored(self):
        result=inventory.inventory(make_elf32(b"\xaa\xbb",flags=0),"thumb",FakeCapstone())
        self.assertEqual(result["summary"]["executable_sections"],0)
    def test_requires_capstone_if_not_injected(self):
        with self.assertRaises(RuntimeError):
            inventory.inventory(make_elf32(b"\xaa\xbb"),"thumb",None)
    def test_mode_must_be_explicit(self):
        with self.assertRaises(inventory.ELFAuditError):
            inventory.inventory(make_elf32(b"\xaa\xbb"),"auto",FakeCapstone())

if __name__=="__main__": unittest.main()
