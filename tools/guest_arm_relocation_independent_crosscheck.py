#!/usr/bin/env python3
"""Cross-check ELF32 ARM section relocations with GNU/LLVM readelf; read-only."""
from __future__ import annotations
import argparse, collections, json, re, shutil, subprocess
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
from guest_arm_relocation_audit import audit_relocations, RELOC_NAMES
from guest_arm_relocation_abi import SPECS

ROW=re.compile(r"^\\s*([0-9a-fA-F]+)\\s+([0-9a-fA-F]+)\\s+(R_ARM_[A-Z0-9_]+)\\b")

def parse_readelf_relocations(output):
    rows=[]
    for line in output.splitlines():
        match=ROW.match(line)
        if match:
            rows.append({"offset":int(match.group(1),16),"info":int(match.group(2),16),
                         "type_name":match.group(3)})
    return rows

def _internal_rows(report):
    rows=[]
    for section in report.get("sections",[]):
        for entry in section.get("entries",[]):
            rows.append({"offset":int(entry["offset"],16),"type":entry["type"],
                         "type_name":entry["type_name"],"section":section["name"]})
    return rows

def compare_rows(internal, external):
    reverse={spec["name"]:code for code,spec in SPECS.items()}
    reverse.update({name:code for code,name in RELOC_NAMES.items()})
    supported=[]; unsupported=[]
    for row in external:
        code=reverse.get(row["type_name"])
        if code is None:
            unsupported.append(row["type_name"])
        else:
            supported.append((row["offset"],code,row["type_name"]))
    expected=[(row["offset"],row["type"],row["type_name"]) for row in internal
              if row["type_name"] in reverse]
    expected_counts=collections.Counter(expected)
    actual_counts=collections.Counter(supported)
    missing=list((expected_counts-actual_counts).elements())
    unexpected=list((actual_counts-expected_counts).elements())
    return {"status":"passed" if not missing and not unexpected else "mismatch",
            "internal_supported_count":sum(expected_counts.values()),
            "external_supported_count":sum(actual_counts.values()),
            "missing_from_readelf":[{"offset":f"0x{x[0]:x}","type":x[2]} for x in missing],
            "unexpected_from_readelf":[{"offset":f"0x{x[0]:x}","type":x[2]} for x in unexpected],
            "unsupported_external_types":sorted(set(unsupported)),
            "comparison":"multiset of relocation offset and AAELF32 type; symbol names and implicit addends are not compared"}

def cross_check(path, tool=None):
    path=Path(path)
    report=audit_relocations(path.read_bytes())
    selected=tool or shutil.which("readelf") or shutil.which("llvm-readelf")
    if not selected:
        return {"status":"skipped","reason":"GNU readelf and llvm-readelf are unavailable",
                "internal_relocation_count":report["relocation_count"],
                "external_tool":None,"evidence_limits":["No independent comparison was performed."]}
    result=subprocess.run([selected,"-rW",str(path)],capture_output=True,text=True,check=False)
    if result.returncode:
        raise ELFAuditError(f"independent ELF tool failed ({selected}): {result.stderr.strip()}")
    external=parse_readelf_relocations(result.stdout)
    comparison=compare_rows(_internal_rows(report),external)
    comparison.update({"external_tool":selected,"internal_relocation_count":report["relocation_count"],
                       "external_relocation_rows":len(external),"source_sha256":report["sha256"],
                       "evidence_limits":["Only section-backed ELF32 EM_ARM SHT_REL/SHT_RELA entries are compared.",
                         "Comparison checks relocation offset and type, not symbol resolution or REL implicit addends.",
                         "Unsupported external relocation names are reported separately and are not silently equated.",
                         "The input is never executed and no relocation is applied."]})
    return comparison

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input",type=Path); parser.add_argument("--tool")
    parser.add_argument("--json",type=Path); args=parser.parse_args()
    try:
        report=cross_check(args.input,args.tool)
        rendered=json.dumps(report,indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True,exist_ok=True)
            args.json.write_text(rendered+"\\n",encoding="utf-8")
        print(rendered)
        return 0 if report["status"] in ("passed","skipped") else 2
    except (OSError,ELFAuditError) as exc:
        parser.error(str(exc))
if __name__=="__main__": raise SystemExit(main())
