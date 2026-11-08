import os
import pathlib
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
VERIFY=ROOT/"tools/verify_android_liblzma.sh"

class ArchiveMemberArchitectureTests(unittest.TestCase):
    def make_tools(self, base, second="GOOD"):
        ar=base/"mock-ar"
        ar.write_text("""#!/usr/bin/env bash
set -euo pipefail
if [[ "$1" == t ]]; then
  echo first.o
  echo second.o
elif [[ "$1" == p ]]; then
  case "$3" in
    first.o) printf GOOD ;;
    second.o) printf %s """ + second + """ ;;
    *) exit 3 ;;
  esac
else
  exit 4
fi
""")
        ar.chmod(0o755)
        readelf=base/"mock-readelf"
        readelf.write_text("""#!/usr/bin/env bash
set -euo pipefail
value="$(cat "$2")"
if [[ "$value" == GOOD ]]; then
  echo 'Class: ELF64'
  echo 'Machine: AArch64'
else
  echo 'Class: ELF32'
  echo 'Machine: ARM'
fi
""")
        readelf.chmod(0o755)
        return ar,readelf

    def run_verify(self, second="GOOD"):
        with tempfile.TemporaryDirectory() as tmp:
            base=pathlib.Path(tmp)
            prefix=base/"prefix"
            prefix.mkdir()
            (prefix/"liblzma.a").write_bytes(b"synthetic archive marker")
            (prefix/"lzma.h").write_text("/* header */")
            ar,readelf=self.make_tools(base,second)
            return subprocess.run(
                ["bash",str(VERIFY),str(prefix),str(readelf),str(ar)],
                text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=10)

    def test_accepts_only_after_every_member_is_verified(self):
        result=self.run_verify()
        self.assertEqual(result.returncode,0,result.stdout)
        self.assertIn("all 2 liblzma archive members",result.stdout)

    def test_rejects_bad_second_member_even_when_first_is_aarch64(self):
        result=self.run_verify("BAD")
        self.assertNotEqual(result.returncode,0)
        self.assertIn("second.o",result.stdout)
        self.assertIn("not ELF64",result.stdout)

if __name__=="__main__":
    unittest.main()
