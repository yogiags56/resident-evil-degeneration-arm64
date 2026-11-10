import importlib.util, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("guest_arm_dependency_graph",ROOT/"tools"/"guest_arm_dependency_graph.py")
module=importlib.util.module_from_spec(spec); assert spec and spec.loader; spec.loader.exec_module(module)

def node(soname,needed):
    return {"soname":soname,"adapter":{"needed_libraries":needed}}

class DependencyGraphTests(unittest.TestCase):
    def test_dependency_first_order_and_edges(self):
        manifest={"root":"game.so","libraries":[node("game.so",["libc.so","libengine.so"]),node("libengine.so",["libc.so"]),node("libc.so",[])]}
        out=module.build_graph(manifest)
        self.assertTrue(out["complete"])
        self.assertEqual(out["dependency_first_load_order"],["libc.so","libengine.so","game.so"])
        self.assertEqual(len(out["edges"]),3)
    def test_reports_missing_dependency_without_guessing(self):
        out=module.build_graph({"root":"game.so","libraries":[node("game.so",["missing.so"])]})
        self.assertFalse(out["complete"])
        self.assertEqual(out["unresolved_dependencies"],[{"from":"game.so","needed":"missing.so","ordinal":0}])
        self.assertEqual(out["dependency_first_load_order"],["game.so"])
    def test_detects_cycle_and_withholds_load_order(self):
        out=module.build_graph({"root":"a.so","libraries":[node("a.so",["b.so"]),node("b.so",["a.so"])]})
        self.assertFalse(out["complete"])
        self.assertEqual(out["cycles"],[["a.so","b.so","a.so"]])
        self.assertEqual(out["dependency_first_load_order"],[])
    def test_rejects_duplicate_soname_provider(self):
        with self.assertRaises(module.DependencyGraphError):
            module.build_graph({"root":"a.so","libraries":[node("a.so",[]),node("a.so",[])]})
    def test_rejects_path_like_soname(self):
        with self.assertRaises(module.DependencyGraphError):
            module.build_graph({"root":"../a.so","libraries":[node("../a.so",[])]})
    def test_rejects_missing_root(self):
        with self.assertRaises(module.DependencyGraphError):
            module.build_graph({"root":"absent.so","libraries":[node("a.so",[])]})
    def test_preserves_duplicate_needed_entries_but_deduplicates_graph_edge(self):
        out=module.build_graph({"root":"a.so","libraries":[node("a.so",["b.so","b.so"]),node("b.so",[])]})
        self.assertEqual(len(out["edges"]),1)
        self.assertEqual(next(n for n in out["nodes"] if n["soname"]=="a.so")["needed_libraries"],["b.so","b.so"])

if __name__=="__main__": unittest.main()
