#!/usr/bin/env python3
"""Cross-check ELF32 ARM relocation sites against local mapping symbols."""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path
from guest_arm_elf_audit import audit_elf, ELFAuditError
from guest_arm_instruction_inventory import parse_mapping_symbols, mapping_ranges
from guest_arm_relocation_audit import audit_relocations
SHF_EXECINSTR=0x4
CALL_JUMP_TYPES={1,10,28,29,30,50}
def _cross_check_reports(elf,reloc_report,ranges_by_index,sha):
    section_by_index={s["index"]:s for s in elf["sections"]}
    findings=[]; counts={"arm_code":0,"thumb_code":0,"data":0,"unknown":0,"non_executable":0,"call_jump_in_code":0,"call_jump_outside_code":0,"undefined_symbols":0}
    for reloc_section in reloc_report["sections"]:
        target_idx=reloc_section["target_section_index"]; target=section_by_index[target_idx]
        for entry in reloc_section["entries"]:
            off=entry["target_section_offset"]; kind="non_executable"
            if target["flags"]&SHF_EXECINSTR:
                for span in ranges_by_index.get(target_idx,[]):
                    if span["start"]<=off<span["end"]: kind=span["kind"]; break
            category={"a":"arm_code","t":"thumb_code","d":"data","unknown":"unknown"}.get(kind,"non_executable")
            counts[category]+=1; is_call_jump=entry["type"] in CALL_JUMP_TYPES
            if is_call_jump:
                counts["call_jump_in_code" if category in ("arm_code","thumb_code") else "call_jump_outside_code"]+=1
            undefined=entry["symbol_section_index"]==0
            if undefined: counts["undefined_symbols"]+=1
            finding={"relocation_section":reloc_section["name"],"target_section":target["name"],"target_section_index":target_idx,
                "target_offset":off,"relocation_type":entry["type"],"relocation_type_name":entry["type_name"],
                "symbol":entry["symbol"],"symbol_index":entry["symbol_index"],"symbol_section_index":entry["symbol_section_index"],
                "symbol_defined":not undefined,"site_classification":category,"is_call_or_jump":is_call_jump}
            if category in ("arm_code","thumb_code"):
                for span in ranges_by_index[target_idx]:
                    if span["start"]<=off<span["end"]:
                        finding["mapping_symbol"]=span.get("mapping_symbol"); finding["instruction_mode"]=span["kind"]; break
            if is_call_jump and category not in ("arm_code","thumb_code"):
                finding["warning"]="call/jump relocation does not land in a mapped ARM/Thumb code range"
            if undefined: finding["symbol_status"]="undefined/external reference; resolver target not established"
            elif entry["symbol_section_index"]==0xfff1: finding["symbol_status"]="absolute symbol"
            else:
                sym_section=section_by_index.get(entry["symbol_section_index"])
                finding["symbol_status"]="defined section symbol" if sym_section else "special or unavailable section index"
                if sym_section: finding["symbol_section"]=sym_section["name"]
            findings.append(finding)
    return {"source_size":elf.get("file_size"),"sha256":sha,"elf_class":32,"machine":"ARM",
        "mapping_symbol_sections":sum(1 for spans in ranges_by_index.values() if spans and any(x.get("mapping_symbol") for x in spans)),
        "relocation_count":len(findings),"classification_counts":counts,"findings":findings,
        "warnings":sorted(set(x["warning"] for x in findings if "warning" in x)),
        "evidence_limits":["Relocation-site classification depends on local ELF mapping symbols.",
            "CALL/JUMP type does not prove final control-flow destination or runtime resolution.",
            "Undefined symbols are not identified as Marmalade/S3E imports.",
            "REL implicit addends are not decoded; relocations are never applied.",
            "Executable ranges without mapping symbols remain unknown."]}
def cross_check(data):
    elf=audit_elf(data)
    if elf["elf_class"]!=32 or elf["machine"]!=40: raise ELFAuditError("cross-check currently requires ELF32 EM_ARM")
    mappings=parse_mapping_symbols(data,elf); reloc_report=audit_relocations(data); ranges={}
    for section in elf["sections"]:
        if section["flags"]&SHF_EXECINSTR:
            symbols=mappings.get(section["index"],[])
            ranges[section["index"]]=mapping_ranges(symbols,section["size"]) if symbols else (
                [{"kind":"unknown","start":0,"end":section["size"]}] if section["size"] else [])
    result=_cross_check_reports(elf,reloc_report,ranges,reloc_report["sha256"])
    result.update({"endianness":elf["endianness"],"mapping_symbol_sections":len(mappings)})
    return result
def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument("input",type=Path); parser.add_argument("--json",type=Path); args=parser.parse_args()
    try:
        report=cross_check(args.input.read_bytes()); report["source"]=args.input.name; rendered=json.dumps(report,indent=2)
        if args.json: args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
