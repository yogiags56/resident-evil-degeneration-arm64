import importlib.util, struct, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import guest_arm_relr_audit as module
import guest_arm_relr as canonical
from guest_arm_elf_audit import ELFAuditError

def fixture(endian="<", words=(0x1300,0x0b,0x1400), tag_triplet=(36,35,37)):
    data=bytearray(0x240); data[:4]=b"\x7fELF"; data[4:7]=bytes([1,1 if endian=="<" else 2,1])
    struct.pack_into(endian+"HHIIIIIHHHHHH",data,16,3,40,1,0x1000,52,0,0,52,32,2,40,0,0)
    struct.pack_into(endian+"IIIIIIII",data,52,1,0,0x1000,0,0x240,0x300,5,0x1000)
    struct.pack_into(endian+"IIIIIIII",data,84,2,0x100,0x1100,0,96,96,4,4)
    tags=[(tag_triplet[0],0x1200),(tag_triplet[1],len(words)*4),(tag_triplet[2],4),(0,0)]
    for i,(tag,value) in enumerate(tags): struct.pack_into(endian+"iI",data,0x100+i*8,tag,value)
    for i,value in enumerate(words): struct.pack_into(endian+"I",data,0x200+i*4,value)
    return bytes(data)

class RELRDecodeTests(unittest.TestCase):
    def test_decodes_direct_bitmap_and_later_direct_entries(self):
        report=module.audit(fixture())
        entries=report["relr_table"]["entries"]
        self.assertEqual([x["target_offset"] for x in entries],[0x1300,0x1304,0x130c,0x1400])
        self.assertEqual([x["encoding"] for x in entries],["direct","bitmap","bitmap","direct"])
        self.assertEqual(report["relocation_count"],4)

    def test_supports_big_endian_elf(self):
        report=module.audit(fixture(">",(0x1300,0x0b)))
        self.assertEqual(report["endianness"],"big")
        self.assertEqual([x["target_offset"] for x in report["relr_table"]["entries"]],[0x1300,0x1304,0x130c])

    def test_rejects_missing_dynamic_tag_pair(self):
        data=bytearray(fixture()); struct.pack_into("<iI",data,0x108,0,0)
        with self.assertRaises(ELFAuditError): module.audit(bytes(data))

    def test_rejects_wrong_entry_size(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+2*8+4,8)
        with self.assertRaises(ELFAuditError): module.audit(bytes(data))

    def test_rejects_misaligned_table_address(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+4,0x1202)
        with self.assertRaises(ELFAuditError): module.audit(bytes(data))

    def test_rejects_misaligned_direct_offset(self):
        data=bytearray(fixture(words=(0x1302,)))
        with self.assertRaises(ELFAuditError): module.audit(data)

    def test_rejects_bitmap_before_direct_offset(self):
        data=bytearray(fixture(words=(0x0b,)))
        with self.assertRaises(ELFAuditError): module.audit(data)

    def test_accepts_empty_bitmap_and_advances_cursor(self):
        report=module.audit(fixture(words=(0x1300,1,0x1380)))
        self.assertEqual([x["target_offset"] for x in report["relr_table"]["entries"]],[0x1300,0x1380])

    def test_rejects_bitmap_cursor_overflow(self):
        data=bytearray(fixture(words=(0xfffffffc,0x03)))
        with self.assertRaises(ELFAuditError): module.audit(data)


    def test_standard_and_android_decoders_have_identical_target_and_provenance_records(self):
        raw=struct.pack("<III",0x1300,0x0b,0x1400)
        expected=canonical.decode_relr_records(raw,"<")
        report=module.audit(fixture())
        self.assertEqual(report["relr_table"]["entries"],expected)

    def test_accepts_android_relr_dynamic_tag_triplet(self):
        report=module.audit(fixture(tag_triplet=(canonical.DT_ANDROID_RELR,
            canonical.DT_ANDROID_RELRSZ,canonical.DT_ANDROID_RELRENT)))
        self.assertEqual(report["relr_table"]["source_tag"],"DT_ANDROID_RELR")
        self.assertEqual(report["relr_table"]["source_tags"],
            ["DT_ANDROID_RELR","DT_ANDROID_RELRSZ","DT_ANDROID_RELRENT"])

    def test_accepts_mixed_standard_and_android_relr_tag_aliases(self):
        data=bytearray(fixture())
        # Replace standard DT_RELRSZ with its Android alias; retain address and entry-size standard tags.
        struct.pack_into("<iI",data,0x108,canonical.DT_ANDROID_RELRSZ,12)
        report=module.audit(bytes(data))
        self.assertEqual(report["relr_table"]["source_tags"],
            ["DT_RELR","DT_RELRENT","DT_ANDROID_RELRSZ"])

    def test_accepts_zero_length_relr_table(self):
        report=module.audit(fixture(words=()))
        self.assertEqual(report["relr_table"]["encoded_entry_count"],0)
        self.assertEqual(report["relocation_count"],0)
        self.assertEqual(report["relr_table"]["entries"],[])

    def test_rejects_table_outside_file_backed_load(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+4,0x1300)
        with self.assertRaises(ELFAuditError): module.audit(bytes(data))

    def test_rejects_non_multiple_of_word_size(self):
        data=bytearray(fixture()); struct.pack_into("<I",data,0x100+8,6)
        with self.assertRaises(ELFAuditError): module.audit(bytes(data))

if __name__=="__main__": unittest.main()
