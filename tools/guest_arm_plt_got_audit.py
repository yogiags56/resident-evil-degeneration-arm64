#!/usr/bin/env python3
"""Cross-reference ELF32 ARM PLT/GOT dynamic tags and JUMP_SLOT relocations."""
from __future__ import annotations
import argparse, json
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
from guest_arm_dynamic_import_inventory import inventory
DT_PLTRELSZ=2
DT_PLTGOT=3
DT_PLTREL=20
DT_JMPREL=23
DT_REL=17
DT_RELA=7
def audit_plt_got(data):
    base=inventory(data)
    tags={}
    for section in base["dynamic_sections"]:
        for entry in section["entries"]:
            if entry["tag"] in (DT_PLTGOT,DT_JMPREL,DT_PLTRELSZ,DT_PLTREL):
                tags[entry["tag"]]=entry["value"]
    relocation_sections=[]
    slots=[]
    for section in base["dynamic_relocation_sections"]:
        section_slots=[]
        for entry in section["entries"]:
            if entry["type_name"]!="R_ARM_JUMP_SLOT": continue
            slot={"relocation_section":section["name"],"target_section":section["target_section"],
                "slot_address":entry["offset"],"symbol_index":entry["symbol_index"],
                "symbol":entry["symbol"],"symbol_undefined":entry["symbol_undefined"],
                "relocation_type":entry["type_name"]}
            section_slots.append(slot); slots.append(slot)
        if section_slots:
            relocation_sections.append({"name":section["name"],"jump_slot_count":len(section_slots),"slots":section_slots})
    pltrel_kind={DT_REL:"REL",DT_RELA:"RELA"}.get(tags.get(DT_PLTREL),"unknown")
    return {"source_size":base["source_size"],"sha256":base["sha256"],
      "elf_class":base["elf_class"],"machine":base["machine"],
      "dynamic_plt_tags":{"DT_PLTGOT":tags.get(DT_PLTGOT),
        "DT_JMPREL":tags.get(DT_JMPREL),"DT_PLTRELSZ":tags.get(DT_PLTRELSZ),
        "DT_PLTREL":tags.get(DT_PLTREL),"DT_PLTREL_kind":pltrel_kind},
      "jump_slot_count":len(slots),"jump_slots":slots,
      "relocation_sections":relocation_sections,
      "evidence_limits":["This stage classifies section-backed R_ARM_JUMP_SLOT records; it does not decode architecture-specific PLT instruction stubs.",
        "DT_JMPREL virtual-address contents are not independently mapped from PT_LOAD in this stage.",
        "DT_PLTGOT is reported as metadata; individual GOT contents and runtime-resolved addresses are not inferred.",
        "No relocation is applied and no import provider is guessed.",
        "PT_DYNAMIC-only stripped ELF and packed Android relocation formats are deferred."]}
def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument("input",type=Path); parser.add_argument("--json",type=Path); args=parser.parse_args()
    try:
        report=audit_plt_got(args.input.read_bytes()); report["source"]=args.input.name
        rendered=json.dumps(report,indent=2)
        if args.json: args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
