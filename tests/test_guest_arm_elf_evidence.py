import importlib.util
import hashlib
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

MODULE=Path(__file__).resolve().parents[1]/"tools"/"guest_arm_elf_evidence.py"
SPEC=importlib.util.spec_from_file_location("guest_arm_elf_evidence",MODULE)
evidence=importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(evidence)

class GuestArmElfEvidenceTests(unittest.TestCase):
    def test_readelf_capture_records_hash_version_and_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/"guest.elf"
            path.write_bytes(b"\x7fELFsynthetic")
            completed=subprocess.CompletedProcess(args=[],returncode=0,stdout="GNU readelf 2.x",stderr="")
            inspected=subprocess.CompletedProcess(args=[],returncode=1,stdout="ELF header output",stderr="unsupported note")
            with mock.patch.object(evidence.shutil,"which",return_value="/usr/bin/readelf"), \
                 mock.patch.object(evidence.subprocess,"run",side_effect=[completed,inspected]) as run:
                result=evidence.capture(path,"readelf",7)
            self.assertEqual(result["sha256"],hashlib.sha256(path.read_bytes()).hexdigest())
            self.assertEqual(result["tool"]["version"],"GNU readelf 2.x")
            self.assertEqual(result["exit_code"],1)
            self.assertEqual(result["stdout"],"ELF header output")
            self.assertEqual(run.call_count,2)
            self.assertEqual(run.call_args.kwargs["timeout"],7)
            self.assertNotIn("shell",run.call_args.kwargs)

    def test_auto_prefers_readelf_then_falls_back_to_llvm(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/"x.elf"; path.write_bytes(b"ELF")
            def which(name):
                return None if name=="readelf" else "/usr/bin/llvm-readobj"
            done=subprocess.CompletedProcess(args=[],returncode=0,stdout="LLVM version",stderr="")
            with mock.patch.object(evidence.shutil,"which",side_effect=which), \
                 mock.patch.object(evidence.subprocess,"run",return_value=done) as run:
                result=evidence.capture(path,"auto")
            self.assertEqual(result["tool"]["name"],"llvm-readobj")
            self.assertIn("--arm-attributes",run.call_args.args[0])

    def test_missing_tool_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/"x.elf"; path.write_bytes(b"ELF")
            with mock.patch.object(evidence.shutil,"which",return_value=None):
                with self.assertRaises(RuntimeError):
                    evidence.capture(path,"readelf")

if __name__=="__main__":
    unittest.main()
