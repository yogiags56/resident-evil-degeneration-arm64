import importlib.util, sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
spec=importlib.util.spec_from_file_location("guest_arm_symbol_adapter",ROOT/"tools"/"guest_arm_symbol_adapter.py")
module=importlib.util.module_from_spec(spec); assert spec and spec.loader; spec.loader.exec_module(module)

class SymbolAdapterTests(unittest.TestCase):
    def report(self):
        return {
            "sha256":"abc",
            "symbol_resolution":{"available":True},
            "needed_libraries":["libc.so","libm.so"],
            "symbol_versioning":{"available":True},
            "dynamic_symbols":[
                {"index":0,"name":"","binding":0,"visibility":0,"undefined":True,"value":"0x0","version":None},
                {"index":1,"name":"puts","binding":1,"visibility":0,"undefined":True,"value":"0x0",
                 "version":{"kind":"requirement","name":"GLIBC_2.0","library":"libc.so","hidden":False}},
                {"index":2,"name":"optional","binding":2,"visibility":0,"undefined":True,"value":"0x0","version":None},
                {"index":3,"name":"exported","binding":1,"visibility":0,"undefined":False,"value":"0x1234",
                 "version":{"kind":"definition","name":"GAME_1.0","hidden":False}},
                {"index":4,"name":"private","binding":1,"visibility":2,"undefined":False,"value":"0x5678","version":None},
                {"index":5,"name":"local","binding":0,"visibility":0,"undefined":False,"value":"0x9999","version":None}
            ]
        }
    def test_extracts_ordered_dependencies_requests_and_providers(self):
        out=module.adapt(self.report())
        self.assertEqual(out["needed_libraries"],["libc.so","libm.so"])
        self.assertEqual([x["name"] for x in out["undefined_requests"]],["puts","optional"])
        self.assertEqual(out["undefined_requests"][0]["version_name"],"GLIBC_2.0")
        self.assertEqual(out["undefined_requests"][0]["version_library"],"libc.so")
        self.assertEqual([x["name"] for x in out["provider_definitions"]],["exported"])
        self.assertEqual(out["provider_definitions"][0]["value"],0x1234)
    def test_requires_bounded_dynamic_symbols(self):
        report=self.report(); report["symbol_resolution"]["available"]=False
        with self.assertRaises(module.SymbolAdapterError): module.adapt(report)
    def test_rejects_invalid_binding(self):
        report=self.report(); report["dynamic_symbols"][1]["binding"]=9
        with self.assertRaises(module.SymbolAdapterError): module.adapt(report)
    def test_rejects_bad_provider_value(self):
        report=self.report(); report["dynamic_symbols"][3]["value"]="bad"
        with self.assertRaises(module.SymbolAdapterError): module.adapt(report)
    def test_preserves_duplicate_dependency_order_if_present(self):
        report=self.report(); report["needed_libraries"]=["libx.so","libx.so"]
        self.assertEqual(module.adapt(report)["needed_libraries"],["libx.so","libx.so"])

if __name__=="__main__": unittest.main()
