import importlib.util, struct, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; sys.path.insert(0,str(ROOT/"tools"))
spec=importlib.util.spec_from_file_location("guest_arm_dynamic_relocation_segment_audit",ROOT/"tools"/"guest_arm_dynamic_relocation_segment_audit.py")
module=importlib.util.module_from_spec(spec); assert spec and spec.loader; spec.loader.exec_module(module)
def fixture(endian="<",include_hash=True):
    data=bytearray(0x240); data[:4]=b"\x7fELF"; data[4:7]=bytes([1,1 if endian=="<" else 2,1])
    struct.pack_into(endian+"HHIIIIIHHHHHH",data,16,3,40,1,0x1000,52,0,0,52,32,2,40,0,0)
    struct.pack_into(endian+"IIIIIIII",data,52,1,0,0x1000,0,0x240,0x300,5,0x1000)
    # 12 PT_DYNAMIC entries, ending with DT_NULL.
    struct.pack_into(endian+"IIIIIIII",data,84,2,0x100,0x1100,0,96,96,4,4)
    tags=[(17,0x1200),(18,8),(19,8),(23,0x1200),(2,8),(20,17),
          (4,0x11c0),(6,0x11d0),(11,16),(5,0x1180),(10,5),(0,0)]
    if not include_hash: tags=[(t,v) for t,v in tags if t!=4]
    for i,(tag,value) in enumerate(tags): struct.pack_into(endian+"iI",data,0x100+i*8,tag,value)
    data[0x180:0x185]=b"\0foo\0"
    # SysV DT_HASH: one bucket, two symbols (null + undefined foo).
    struct.pack_into(endian+"IIII",data,0x1c0,1,2,1,0)
    struct.pack_into(endian+"IIIBBH",data,0x1d0,0,0,0,0,0,0)
    struct.pack_into(endian+"IIIBBH",data,0x1e0,1,0,0,0x12,0,0)
    struct.pack_into(endian+"II",data,0x200,0x1300,(1<<8)|22)
    return bytes(data)


def gnu_fixture(endian="<"):
    data=bytearray(fixture(endian=endian))
    # Replace DT_HASH with DT_GNU_HASH; keep all other dynamic metadata intact.
    struct.pack_into(endian+"iI",data,0x100+6*8,0x6ffffef5,0x1220)
    h=5381
    for byte in b"foo":
        h=((h<<5)+h+byte)&0xffffffff
    bloom=(1<<(h%32))|(1<<((h>>5)%32))
    struct.pack_into(endian+"IIIIIII",data,0x220,1,1,1,5,bloom,1,h|1)
    return bytes(data)


def both_hash_fixture():
    data=bytearray(gnu_fixture())
    # Restore SysV DT_HASH and add GNU hash as a second dynamic tag.
    struct.pack_into("<iI",data,0x100+6*8,4,0x11c0)
    struct.pack_into("<iI",data,0x100+11*8,0x6ffffef5,0x1220)
    struct.pack_into("<iI",data,0x100+12*8,0,0)
    struct.pack_into("<II",data,84+16,104,104)
    return bytes(data)

class DynamicRelocationSegmentTests(unittest.TestCase):
    def test_maps_rel_and_jmprel_through_pt_load_and_resolves_symbol(self):
        report=module.audit(fixture())
        self.assertEqual(report["relocation_count"],2)
        self.assertEqual([x["source_tag"] for x in report["relocation_tables"]],["DT_REL","DT_JMPREL"])
        entry=report["relocation_tables"][0]["entries"][0]
        self.assertEqual(entry["type_name"],"R_ARM_JUMP_SLOT")
        self.assertEqual(entry["symbol"],"foo")
        self.assertTrue(entry["symbol_undefined"])
        self.assertEqual(report["relocation_tables"][0]["file_offset"],0x200)
    def test_missing_sysv_hash_keeps_symbol_identity_unresolved(self):
        report=module.audit(fixture(include_hash=False))
        self.assertFalse(report["symbol_resolution"]["available"])
        self.assertIsNone(report["relocation_tables"][0]["entries"][0]["symbol"])
    def test_rejects_incomplete_rel_tag_triplet(self):
        data=bytearray(fixture())
        # Remove DT_RELENT by replacing its tag with DT_NULL.
        struct.pack_into("<iI",data,0x100+16,0,0)
        with self.assertRaises(module.ELFAuditError): module.audit(bytes(data))
    def test_rejects_unsupported_plt_relocation_kind(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+5*8+4,99)
        with self.assertRaises(module.ELFAuditError): module.audit(bytes(data))
    def test_rejects_relocation_table_outside_file_backed_load_range(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+4,0x1300)
        with self.assertRaises(module.ELFAuditError): module.audit(bytes(data))
    def test_supports_big_endian_dynamic_and_relocation_entries(self):
        report=module.audit(fixture(">"))
        self.assertEqual(report["endianness"],"big")
        self.assertEqual(report["relocation_tables"][0]["entries"][0]["symbol"],"foo")

    def test_gnu_hash_only_bounds_and_resolves_dynamic_symbols(self):
        report=module.audit(gnu_fixture())
        self.assertTrue(report["symbol_resolution"]["available"])
        self.assertEqual(report["symbol_resolution"]["hash_source"],"gnu_hash_only")
        self.assertEqual(report["symbol_resolution"]["symbol_count"],2)
        self.assertEqual(report["relocation_tables"][0]["entries"][0]["symbol"],"foo")
        self.assertEqual(report["symbol_resolution"]["gnu_hash"]["bucket_count"],1)

    def test_rejects_gnu_hash_bloom_filter_missing_symbol_bits(self):
        data=bytearray(gnu_fixture())
        struct.pack_into("<I",data,0x230,0)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))

    def test_rejects_gnu_hash_chain_hash_mismatch(self):
        data=bytearray(gnu_fixture())
        struct.pack_into("<I",data,0x238,0x12345679)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))

    def test_rejects_non_power_of_two_gnu_bloom_size(self):
        data=bytearray(gnu_fixture())
        struct.pack_into("<I",data,0x22c,3)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))


    def test_rejects_gnu_hash_bloom_shift_outside_word_width(self):
        data=bytearray(gnu_fixture())
        struct.pack_into("<I",data,0x22c,32)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))


    def test_cross_checks_gnu_hash_against_sysv_nchain(self):
        report=module.audit(both_hash_fixture())
        self.assertEqual(report["symbol_resolution"]["hash_source"],"gnu_bound_within_sysv_nchain")
        self.assertEqual(report["symbol_resolution"]["sysv_symbol_count"],2)
        self.assertEqual(report["symbol_resolution"]["symbol_count"],2)

    def test_rejects_gnu_symbol_bound_larger_than_sysv_nchain(self):
        data=bytearray(both_hash_fixture())
        struct.pack_into("<I",data,0x1c4,1)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))


    def test_gnu_hash_only_supports_big_endian_elf(self):
        report=module.audit(gnu_fixture(">"))
        self.assertEqual(report["endianness"],"big")
        self.assertEqual(report["relocation_tables"][0]["entries"][0]["symbol"],"foo")

    def test_rejects_unterminated_gnu_hash_chain(self):
        data=bytearray(gnu_fixture())
        struct.pack_into("<I",data,0x238,0x12345678)
        struct.pack_into("<I",data,0x23c,0x12345678)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))

    def test_rejects_gnu_hash_bucket_before_symoffset(self):
        data=bytearray(gnu_fixture())
        struct.pack_into("<I",data,0x234,0)
        # A nonzero bucket below symoffset is invalid; zero is the empty-bucket marker,
        # so use a nonzero value that is lower than symoffset only when symoffset > 1.
        struct.pack_into("<I",data,0x224,2)
        struct.pack_into("<I",data,0x234,1)
        with self.assertRaises(module.ELFAuditError):
            module.audit(bytes(data))

    def test_rejects_invalid_rel_entry_size(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+2*8+4,12)
        with self.assertRaises(module.ELFAuditError): module.audit(bytes(data))
if __name__=="__main__": unittest.main()
