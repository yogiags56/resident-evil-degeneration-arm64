#!/usr/bin/env python3
"""Decode ELF32 EM_ARM DT_RELR relative relocation offsets without applying them."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
from guest_arm_dynamic_relocation_segment_audit import _read_dynamic, _mapped, TAGS

DT_RELRSZ=35
DT_RELR=36
DT_RELRENT=37
WORD_SIZE=4
WORD_BITS=32
MAX_RELR_SIZE=64*1024*1024
MAX_RELR_ENTRIES=4_000_000

def decode_relr(raw: bytes, endian: str):
    if len(raw)>MAX_RELR_SIZE:
        raise ELFAuditError("DT_RELR table exceeds safety limit")
    if len(raw)%WORD_SIZE:
        raise ELFAuditError("DT_RELR size is not a multiple of ELF32 word size")
    if len(raw)//WORD_SIZE>MAX_RELR_ENTRIES:
        raise ELFAuditError("DT_RELR entry count exceeds safety limit")
    words=[struct.unpack_from(endian+"I",raw,pos)[0] for pos in range(0,len(raw),WORD_SIZE)]
    targets=[]
    where=None
    for entry_index,entry in enumerate(words):
        if (entry&1)==0:
            if entry%WORD_SIZE:
                raise ELFAuditError("DT_RELR direct offset is not word-aligned")
            where=entry+WORD_SIZE
            if where>0x100000000:
                raise ELFAuditError("DT_RELR direct offset overflows ELF32 address space")
            targets.append({"index":len(targets),"table_entry_index":entry_index,
              "encoding":"direct","target_address":f"0x{entry:08x}","target_offset":entry})
            continue
        if where is None:
            raise ELFAuditError("DT_RELR bitmap appears before a direct offset")
        bitmap=entry>>1
        if bitmap==0:
            raise ELFAuditError("DT_RELR bitmap contains no relocation bits")
        for bit in range(WORD_BITS-1):
            if bitmap&(1<<bit):
                target=where+bit*WORD_SIZE
                if target>0xffffffff or target%WORD_SIZE:
                    raise ELFAuditError("DT_RELR bitmap target overflows or is misaligned")
                targets.append({"index":len(targets),"table_entry_index":entry_index,
                  "encoding":"bitmap","bitmap_bit":bit,"target_address":f"0x{target:08x}",
                  "target_offset":target})
                if len(targets)>MAX_RELR_ENTRIES:
                    raise ELFAuditError("decoded DT_RELR target count exceeds safety limit")
        where+= (WORD_BITS-1)*WORD_SIZE
        if where>0x100000000:
            raise ELFAuditError("DT_RELR bitmap cursor overflows ELF32 address space")
    return targets

def audit(data: bytes):
    ph=__import__("guest_arm_pt_dynamic_audit").parse_program_headers(data)
    dynamic,tags,loads,endian=_read_dynamic(data,ph)
    present=[tag in tags for tag in (DT_RELR,DT_RELRSZ,DT_RELRENT)]
    if any(present) and not all(present):
        raise ELFAuditError("DT_RELR, DT_RELRSZ, and DT_RELRENT must appear together")
    if not any(present):
        raise ELFAuditError("no DT_RELR dynamic table is declared")
    address,size,entry_size=tags[DT_RELR],tags[DT_RELRSZ],tags[DT_RELRENT]
    if entry_size!=WORD_SIZE:
        raise ELFAuditError("DT_RELRENT must equal 4 for ELF32")
    if size==0 or size>MAX_RELR_SIZE or size%WORD_SIZE:
        raise ELFAuditError("DT_RELRSZ is empty, oversized, or not word-aligned")
    if address%WORD_SIZE:
        raise ELFAuditError("DT_RELR table address is not word-aligned")
    offset,raw=_mapped(data,address,size,loads,"DT_RELR")
    entries=decode_relr(raw,endian)
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
      "elf_class":32,"machine":"ARM","endianness":ph["endianness"],
      "section_headers_present":ph["section_headers_present"],"dynamic_entries":dynamic,
      "dynamic_tags":{TAGS.get(k,f"DT_UNKNOWN_{k}"):v for k,v in tags.items()},
      "relr_table":{"source_tag":"DT_RELR","virtual_address":f"0x{address:08x}",
        "file_offset":offset,"size":size,"entry_size":entry_size,
        "encoded_entry_count":size//WORD_SIZE,"relocation_count":len(entries),"entries":entries},
      "relocation_count":len(entries),
      "evidence_limits":["ELF32 EM_ARM only.","RELR offsets are decoded as relative relocation target locations; no target bytes are read or modified.",
        "No addend, runtime load bias, or relocated value is inferred.",
        "RELR is reported separately; reconciliation with REL/RELA/APS2 is a separate metadata operation.",
        "No relocation is applied and no game boot is claimed."]}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input",type=Path); parser.add_argument("--json",type=Path)
    args=parser.parse_args()
    try:
        report=audit(args.input.read_bytes()); report["source"]=args.input.name
        rendered=json.dumps(report,indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True,exist_ok=True)
            args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc:
        parser.error(str(exc))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
