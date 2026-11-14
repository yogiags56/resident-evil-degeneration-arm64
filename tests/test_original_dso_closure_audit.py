import importlib.util, json, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("audit_original_dso_closure", ROOT / "tools" / "audit_original_dso_closure.py")
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)
manifest = json.loads((ROOT / "reports" / "STAGE123O_LIBJIROBIODG_PROVIDER_MANIFEST.json").read_text())
policy = json.loads((ROOT / "reports" / "STAGE125F_PROVIDER_POLICY.json").read_text())

class OriginalDsoClosureAuditTests(unittest.TestCase):
    def test_exact_six_dt_needed_entries_have_explicit_policy(self):
        result = module.audit(manifest, policy)
        self.assertEqual(result["dt_needed_count"], 6)
        self.assertTrue(result["dt_needed_policy_coverage"])
        self.assertEqual(result["dependency_only_sonames"], ["libm.so"])

    def test_libm_is_not_fabricated_as_export_provider(self):
        libm = policy["soname_policy"]["libm.so"]
        self.assertEqual(libm["mode"], "dependency_only")
        self.assertEqual(libm["provided_symbols"], [])
        self.assertEqual(libm["relocation_symbols"], [])

    def test_known_guest_provider_exports_are_covered(self):
        result = module.audit(manifest, policy)
        self.assertTrue({"dlopen", "inflate", "_Znwj", "__android_log_print", "__aeabi_dadd", "malloc"}.issubset(result["provider_symbol_mapping"]))

    def test_unimplemented_imports_keep_production_gate_closed(self):
        result = module.audit(manifest, policy)
        self.assertGreater(len(result["uncovered_relocation_symbols"]), 0)
        self.assertFalse(result["relocation_symbol_closure_complete"])
        self.assertFalse(result["production_loader_gate"])
        self.assertIn("pthread_create", result["uncovered_relocation_symbols"])
        self.assertIn("socket", result["uncovered_relocation_symbols"])

    def test_rejects_missing_soname_policy(self):
        bad = json.loads(json.dumps(policy))
        del bad["soname_policy"]["libm.so"]
        with self.assertRaises(module.AuditError):
            module.audit(manifest, bad)

    def test_rejects_provider_symbol_claim_not_in_original_relocations(self):
        bad = json.loads(json.dumps(policy))
        bad["soname_policy"]["libc.so"]["provided_symbols"].append("invented_symbol")
        result = module.audit(manifest, bad)
        self.assertIn("invented_symbol", result["unexpected_declared_provider_symbols"])
        self.assertFalse(result["production_loader_gate"])

    def test_rejects_duplicate_provider_assignment(self):
        bad = json.loads(json.dumps(policy))
        bad["soname_policy"]["libm.so"]["mode"] = "guest_provider"
        bad["soname_policy"]["libm.so"]["provided_symbols"] = ["malloc"]
        with self.assertRaises(module.AuditError):
            module.audit(manifest, bad)

if __name__ == "__main__":
    unittest.main()
