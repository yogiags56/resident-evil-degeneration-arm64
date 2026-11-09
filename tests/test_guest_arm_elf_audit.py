import importlib.util, struct, unittest
from pathlib import Path
MODULE=Path(__file__).resolve().parents[1]/"tools"/"guest_arm_elf_audit.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_elf_audit",MODULE)
audit=importlib.util.module_from_spec(SPEC); assert SPEC and SPEC.loader; SPEC.loader.exec_module(audit)
def uleb(v):
    out=bytearray()
    while True:
        b=v&127; v>>=7; out.append(b|(128 if v else 0))
        if not v:return bytes(out)
def make_elf32(attributes=None,endian="<"):
    names=b"\0.shstrtab\0.ARM.attributes\0"; attr=b""
    if attributes is not None:
        body=b"".join(uleb(k)+(v.encode()+b"\0" if isinstance(v,str) else uleb(v)) for k,v in attributes.items())
        payload=uleb(1)+struct.pack(endian+"I",4+len(body))+body; vendor=b"aeabi\0"
        attr=b"A"+struct.pack(endian+"I",4+len(vendor)+len(payload))+vendor+payload
    ident=b"\x7fELF"+bytes([1,1 if endian=="<" else 2,1])+bytes(9)
    eh,sh=52,40; shoff=eh; noff=shoff+3*sh; aoff=noff+len(names)
    header=struct.pack(endian+"HHIIIIIHHHHHH",1,40,1,0,0,shoff,0,eh,0,0,sh,3,1)
    null=bytes(sh)
    shstr=struct.pack(endian+"IIIIIIIIII",1,3,0,0,noff,len(names),0,0,1,0)
    arm=struct.pack(endian+"IIIIIIIIII",11,0x70000003,0,0,aoff,len(attr),0,0,1,0)
    return ident+header+null+shstr+arm+names+attr
class GuestArmElfAuditTests(unittest.TestCase):
    def test_arm32_attributes(self):
        r=audit.audit_elf(make_elf32({6:10,7:65,9:2,10:3,12:1}))
        self.assertEqual((r["elf_class"],r["machine_name"]),(32,"ARM"))
        d=r["arm_attributes"]["decoded"]
        self.assertEqual(d["CPU_arch"]["name"],"ARMv7")
        self.assertEqual(d["CPU_arch_profile"]["name"],"Application")
        self.assertEqual(d["THUMB_ISA_use"]["name"],"Thumb-2 permitted")
        self.assertEqual(d["FP_arch"]["name"],"VFPv3")
        self.assertEqual(d["Advanced_SIMD_arch"]["name"],"NEONv1")
    def test_big_endian(self):
        r=audit.audit_elf(make_elf32({6:6},">"))
        self.assertEqual(r["endianness"],"big")
        self.assertEqual(r["arm_attributes"]["decoded"]["CPU_arch"]["name"],"ARMv6")
    def test_missing_attributes_unknown(self):
        r=audit.audit_elf(make_elf32())
        self.assertFalse(r["arm_attributes"]["present"])
        self.assertIn("unknown",r["evidence_limits"][1])
    def test_non_elf(self):
        with self.assertRaises(audit.ELFAuditError): audit.audit_elf(b"not ELF")
    def test_truncated_program_headers(self):
        b=bytearray(make_elf32()); struct.pack_into("<I",b,28,len(b)+2); struct.pack_into("<H",b,44,1)
        with self.assertRaises(audit.ELFAuditError): audit.audit_elf(bytes(b))
    def test_bad_section_bounds(self):
        b=bytearray(make_elf32()); shoff=struct.unpack_from("<I",b,32)[0]
        struct.pack_into("<I",b,shoff+2*40+20,len(b)+10)
        with self.assertRaises(audit.ELFAuditError): audit.audit_elf(bytes(b))
    def test_malformed_attribute_length(self):
        b=bytearray(make_elf32({6:10})); b[-1]=255
        with self.assertRaises(audit.ELFAuditError): audit.audit_elf(bytes(b))
if __name__=="__main__": unittest.main()
