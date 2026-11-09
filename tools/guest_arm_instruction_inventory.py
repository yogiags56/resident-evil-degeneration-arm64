#!/usr/bin/env python3
"""Inventory ARM/Thumb instructions in executable ELF sections using Capstone."""
from __future__ import annotations
import argparse, collections, hashlib, importlib, json
from pathlib import Path
from guest_arm_elf_audit import audit_elf, ELFAuditError

SHF_EXECINSTR=0x4

def decode_section(code: bytes, address: int, mode: str, capstone_module, max_samples: int=200):
    arch=capstone_module.CS_ARCH_ARM
    cs_mode=capstone_module.CS_MODE_ARM if mode=="arm" else capstone_module.CS_MODE_THUMB
    if mode=="thumb" and hasattr(capstone_module,"CS_MODE_MCLASS"):
        # Do not assume M-profile; Thumb mode only is intentionally selected.
        pass
    engine=capstone_module.Cs(arch,cs_mode)
    engine.detail=False
    cursor=0
    decoded_bytes=0
    counts=collections.Counter()
    samples=[]
    gaps=[]
    unit=4 if mode=="arm" else 2
    while cursor<len(code):
        insn=next(engine.disasm(code[cursor:],address+cursor,count=1),None)
        if insn is None or insn.size<=0 or insn.size>len(code)-cursor or insn.address!=address+cursor:
            gap_start=cursor
            cursor=min(len(code),cursor+unit)
            gaps.append((gap_start,cursor))
            continue
        counts[insn.mnemonic]+=1
        decoded_bytes+=insn.size
        if len(samples)<max_samples:
            samples.append({"address":f"0x{insn.address:x}","size":insn.size,
                            "mnemonic":insn.mnemonic,"operands":insn.op_str})
        cursor+=insn.size
    merged=[]
    for start,end in gaps:
        if merged and merged[-1][1]==start: merged[-1][1]=end
        else: merged.append([start,end])
    return {"size":len(code),"decoded_bytes":decoded_bytes,
            "undecoded_bytes":len(code)-decoded_bytes,
            "decoded_instructions":sum(counts.values()),
            "mnemonics":dict(sorted(counts.items())),
            "undecoded_ranges":[{"offset":start,"address":f"0x{address+start:x}","size":end-start,
                                 "bytes":code[start:end].hex()} for start,end in merged],
            "sample_instructions":samples}

def inventory(data: bytes, mode: str, capstone_module=None):
    elf=audit_elf(data)
    if elf["elf_class"]!=32 or elf["machine"]!=40:
        raise ELFAuditError("instruction inventory currently requires ELF32 with e_machine=EM_ARM")
    if mode not in ("arm","thumb"):
        raise ELFAuditError("mode must be explicitly arm or thumb")
    if capstone_module is None:
        try: capstone_module=importlib.import_module("capstone")
        except ImportError as exc:
            raise RuntimeError("Capstone is required; install a reviewed/pinned capstone package (see Stage 119V documentation)") from exc
    sections=[]
    total_bytes=total_decoded=total_insns=0
    all_counts=collections.Counter()
    for section in elf["sections"]:
        if not (section["flags"] & SHF_EXECINSTR) or section["size"]==0:
            continue
        offset=section["offset"]; size=section["size"]
        if offset+size>len(data):
            raise ELFAuditError(f"executable section {section['name']} exceeds input bounds")
        code=data[offset:offset+size]
        result=decode_section(code,int(section["address"],16),mode,capstone_module)
        result.update({"name":section["name"],"address":section["address"],"file_offset":offset})
        sections.append(result)
        total_bytes+=size; total_decoded+=result["decoded_bytes"]
        total_insns+=result["decoded_instructions"]; all_counts.update(result["mnemonics"])
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
            "elf_class":elf["elf_class"],"machine":elf["machine_name"],
            "endianness":elf["endianness"],"mode":mode,
            "decoder":{"name":"Capstone","version":getattr(capstone_module,"__version__","unknown")},
            "summary":{"executable_sections":len(sections),"raw_executable_bytes":total_bytes,
                       "decoded_bytes":total_decoded,"undecoded_bytes":total_bytes-total_decoded,
                       "decoded_instructions":total_insns,"mnemonics":dict(sorted(all_counts.items()))},
            "sections":sections,
            "evidence_limits":[
                "ARM/Thumb mode is selected explicitly for every executable section; mixed-mode mapping symbols are not interpreted.",
                "Undecoded ranges are decoder failures or unsupported encodings, not proof of invalid code.",
                "Counts describe decoded bytes only; ELF attributes are not treated as proof of instruction use.",
                "No proprietary input binary is stored or modified."]}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input",type=Path)
    parser.add_argument("--mode",choices=("arm","thumb"),required=True,
                        help="explicit decoding mode; mixed ARM/Thumb mapping symbols are not auto-detected")
    parser.add_argument("--json",type=Path)
    args=parser.parse_args()
    try:
        report=inventory(args.input.read_bytes(),args.mode)
        report["source"]=args.input.name
        rendered=json.dumps(report,indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True,exist_ok=True)
            args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError,RuntimeError) as exc:
        parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
