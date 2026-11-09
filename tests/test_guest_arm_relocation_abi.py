import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import guest_arm_relocation_abi as module
from guest_arm_relocation_target_audit import ARM_WIDTHS

class ArmRelocationAbiTests(unittest.TestCase):
    def test_data_relocations_have_distinct_access_widths(self):
        self.assertEqual(module.get_spec(8)["width"],1)   # ABS8
        self.assertEqual(module.get_spec(5)["width"],2)   # ABS16
        self.assertEqual(module.get_spec(2)["width"],4)   # ABS32

    def test_thumb_instruction_relocations_cover_full_instruction_pair(self):
        spec=module.get_spec(10)  # THM_CALL
        self.assertEqual(spec["width"],4)
        self.assertEqual(spec["alignment"],2)
        self.assertEqual(spec["class"],"thumb32")

    def test_thumb16_relocations_are_two_bytes(self):
        for code in (7,11,52,102,103):
            self.assertEqual(module.get_spec(code)["width"],2)

    def test_copy_width_is_symbol_size_dependent(self):
        spec=module.get_spec(20)
        self.assertIsNone(spec["width"])
        self.assertEqual(spec["width_source"],"symbol_st_size")

    def test_none_has_zero_access(self):
        self.assertEqual(module.get_spec(0)["width"],0)
        self.assertEqual(ARM_WIDTHS[0],0)

    def test_unknown_relocation_is_not_guessed(self):
        self.assertIsNone(module.get_spec(250))
        self.assertNotIn(250,ARM_WIDTHS)

    def test_platform_specific_target1_is_not_claimed_universal(self):
        self.assertEqual(module.get_spec(38)["kind"],"platform_specific")


    def test_relative_relocation_requires_no_symbol(self):
        spec=module.contract_for(23,symbol_index=1)
        self.assertEqual(spec["symbol_policy"],"must_be_zero_or_absent")
        self.assertIn("unexpected_symbol_index_for_symbol_free_relocation",spec["contract_issues"])

    def test_jump_slot_requires_nonzero_symbol(self):
        spec=module.contract_for(22,symbol_index=0)
        self.assertIn("required_symbol_index_is_zero",spec["contract_issues"])

    def test_tls_dtpmod_allows_null_symbol(self):
        spec=module.contract_for(17,symbol_index=0)
        self.assertEqual(spec["symbol_policy"],"may_be_zero")
        self.assertEqual(spec["contract_issues"],[])

    def test_copy_requires_symbol_and_variable_size(self):
        spec=module.contract_for(20,symbol_index=0)
        self.assertIn("required_symbol_index_is_zero",spec["contract_issues"])
        self.assertEqual(module.get_spec(20)["width_source"],"symbol_st_size")

    def test_et_rel_section_domain_is_valid(self):
        spec=module.contract_for(2,symbol_index=1,target_domain="section:4")
        self.assertEqual(spec["contract_issues"],[])

    def test_unknown_type_contract_is_not_guessed(self):
        spec=module.contract_for(250,symbol_index=1)
        self.assertFalse(spec["known"])
        self.assertEqual(spec["symbol_policy"],"unknown")

    def test_registry_entries_have_required_metadata(self):
        for code,spec in module.SPECS.items():
            self.assertIn("name",spec)
            self.assertIn("width",spec)
            self.assertIn("alignment",spec)
            self.assertIn("class",spec)
            self.assertIn("kind",spec)
            self.assertIn("semantics",spec)
            if spec["width"] is not None:
                self.assertGreaterEqual(spec["width"],0)
            self.assertGreater(spec["alignment"],0)

if __name__=="__main__": unittest.main()
