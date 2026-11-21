import importlib.util, struct, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; sys.path.insert(0,str(ROOT/"tools"))
spec=importlib.util.spec_from_file_location("guest_arm_pt_dynamic_audit",ROOT/"tools"/"guest_arm_pt_dynamic_audit.py")
module=importlib.util.module_from_spec(spec); assert spec and spec.loader; spec.loader.exec_module(module)
def fixture(endian="<",section_count=0):
    data=bytearray(0x240); data[:4]=b"\x7fELF"; data[4:7]=bytes([1,1 if endian=="<" else 2,1])
    # ELF32 header: ET_DYN, EM_ARM, entry, two program headers, no section table.
    struct.pack_into(endian+"HHIIIIIHHHHHH",data,16,3,40,1,0x1000,52,0,0,52,32,2,40,section_count,0)
    # PT_LOAD maps file offsets 0..0x240 at virtual addresses 0x1000..0x1240.
    struct.pack_into(endian+"IIIIIIII",data,52,1,0,0x1000,0,0x240,0x300,5,0x1000)
    # PT_DYNAMIC at file offset 0x100, virtual address 0x1100.
    struct.pack_into(endian+"IIIIIIII",data,84,2,0x100,0x1100,0,32,32,4,4)
    strings=b"\0libfixture.so\0"; straddr=0x1180
    entries=[(1,1),(5,straddr),(10,len(strings)),(0,0)]
    for i,(tag,value) in enumerate(entries): struct.pack_into(endian+"iI",data,0x100+i*8,tag,value)
    data[0x180:0x180+len(strings)]=strings
    return bytes(data)
class ProgramHeaderDynamicAuditTests(unittest.TestCase):
    def test_reads_dynamic_tags_and_dependency_without_section_headers(self):
        report=module.audit(fixture())
        self.assertFalse(report["section_headers_present"])
        self.assertEqual(report["load_segment_count"],1)
        self.assertEqual(report["dynamic"]["dependencies"],["libfixture.so"])
        self.assertEqual(report["dynamic"]["entries"][0]["tag_name"],"DT_NEEDED")
    def test_maps_virtual_addresses_only_through_file_backed_load_ranges(self):
        ph=module.parse_program_headers(fixture())
        loads=[x for x in ph["program_headers"] if x["type"]==1]
        self.assertEqual(module.map_vaddr(0x1180,12,loads),0x180)
        with self.assertRaises(module.ELFAuditError): module.map_vaddr(0x1280,8,loads)
    def test_rejects_dynamic_string_table_outside_load_file_bytes(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+8+4,0x1300)
        with self.assertRaises(module.ELFAuditError): module.audit(bytes(data))
    def test_rejects_missing_dt_null(self):
        data=bytearray(fixture()); struct.pack_into("<iI",data,0x100+24,1,1)
        with self.assertRaises(module.ELFAuditError): module.audit(bytes(data))
    def test_supports_big_endian_program_headers_and_dynamic_entries(self):
        report=module.audit(fixture(">"))
        self.assertEqual(report["endianness"],"big")
        self.assertEqual(report["dynamic"]["dependencies"],["libfixture.so"])
    def test_rejects_segment_file_size_larger_than_memory_size(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,52+16,0x400)
        with self.assertRaises(module.ELFAuditError): module.audit(bytes(data))
if __name__=="__main__": unittest.main()
