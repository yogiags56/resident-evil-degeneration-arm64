import sys, tempfile, unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import guest_arm_relocation_independent_crosscheck as cross
import guest_arm_relocation_target_audit as target
from guest_arm_relocation_abi import get_spec
from test_guest_arm_relocation_audit import make_elf32

class IndependentReadelfCrossCheckTests(unittest.TestCase):
    SAMPLE="""Relocation section '.rel.text' at offset 0x200 contains 2 entries:
 Offset     Info    Type                Sym.Value  Sym. Name
00001000  0000011c R_ARM_CALL             00001000   target_func
00001004  0000011d R_ARM_JUMP24           00001000   target_func
"""
    def test_parses_readelf_rows(self):
        rows=cross.parse_readelf_relocations(self.SAMPLE)
        self.assertEqual(len(rows),2)
        self.assertEqual(rows[0]["offset"],0x1000)
        self.assertEqual(rows[1]["type_name"],"R_ARM_JUMP24")

    def test_matching_offset_and_type_passes(self):
        internal=[{"offset":0x1000,"type":28,"type_name":"R_ARM_CALL"},
                  {"offset":0x1004,"type":29,"type_name":"R_ARM_JUMP24"}]
        result=cross.compare_rows(internal,cross.parse_readelf_relocations(self.SAMPLE))
        self.assertEqual(result["status"],"passed")
        self.assertEqual(result["internal_supported_count"],2)

    def test_wrong_external_offset_is_mismatch(self):
        internal=[{"offset":0x1000,"type":28,"type_name":"R_ARM_CALL"}]
        external=[{"offset":0x1004,"info":0x11c,"type_name":"R_ARM_CALL"}]
        result=cross.compare_rows(internal,external)
        self.assertEqual(result["status"],"mismatch")
        self.assertEqual(len(result["missing_from_readelf"]),1)

    def test_unsupported_external_type_is_reported_not_guessed(self):
        result=cross.compare_rows([], [{"offset":0,"info":0,"type_name":"R_ARM_PRIVATE_16"}])
        self.assertEqual(result["status"],"passed")
        self.assertEqual(result["unsupported_external_types"],["R_ARM_PRIVATE_16"])

    def test_tool_absence_is_explicit_skip(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/"fixture.elf"; path.write_bytes(make_elf32())
            with patch.object(cross.shutil,"which",return_value=None):
                result=cross.cross_check(path)
        self.assertEqual(result["status"],"skipped")
        self.assertIn("unavailable",result["reason"])

class CopyRelocationWidthTests(unittest.TestCase):
    def test_copy_width_uses_defined_symbol_size_and_namespace(self):
        entry={"type":20,"symbol_size":37,"symbol_defined":True,
               "symbol_index":2,"symbol_namespace":"DT_SYMTAB@0x1000"}
        self.assertEqual(target.resolve_access_width(entry,get_spec(20)),(37,"validated_symbol_st_size"))

    def test_copy_width_stays_unknown_for_undefined_symbol(self):
        entry={"type":20,"symbol_size":37,"symbol_defined":False,
               "symbol_index":2,"symbol_namespace":"DT_SYMTAB@0x1000"}
        self.assertEqual(target.resolve_access_width(entry,get_spec(20)),(None,"unresolved_copy_symbol_size"))

    def test_copy_width_stays_unknown_without_namespace(self):
        entry={"type":20,"symbol_size":37,"symbol_defined":True,"symbol_index":2}
        self.assertEqual(target.resolve_access_width(entry,get_spec(20)),(None,"unresolved_copy_symbol_size"))

    def test_copy_width_stays_unknown_for_zero_or_missing_size(self):
        for size in (None,0,-1,"37"):
            entry={"type":20,"symbol_size":size,"symbol_defined":True,
                   "symbol_index":2,"symbol_namespace":"SHT_SYMTAB:3"}
            self.assertEqual(target.resolve_access_width(entry,get_spec(20))[0],None)

    def test_copy_requires_nonzero_symbol_index(self):
        entry={"type":20,"symbol_size":37,"symbol_defined":True,
               "symbol_index":0,"symbol_namespace":"DT_SYMTAB@0x1000"}
        self.assertEqual(target.resolve_access_width(entry,get_spec(20))[0],None)

if __name__=="__main__": unittest.main()
