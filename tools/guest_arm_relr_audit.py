#!/usr/bin/env python3
"""Decode ELF32 EM_ARM DT_RELR relative relocation offsets without applying them."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
from guest_arm_dynamic_relocation_segment_audit import _read_dynamic, _mapped, TAGS
from guest_arm_relr import (collect_relr_tables, decode_relr_records, DT_RELR, DT_RELRSZ, DT_RELRENT,
    DT_ANDROID_RELR, DT_ANDROID_RELRSZ, DT_ANDROID_RELRENT, DT_ANDROID_RELRCOUNT)

WORD_SIZE=4
MAX_RELR_SIZE=64*1024*1024


def audit(data: bytes):
    ph=__import__("guest_arm_pt_dynamic_audit").parse_program_headers(data)
    dynamic,tags,loads,endian=_read_dynamic(data,ph)
    tables=collect_relr_tables(data,tags,loads,endian,_mapped)
    if not tables:
        raise ELFAuditError("no RELR dynamic table is declared")
    table=tables[0]
    size=table["size"]
    if size:
        offset,raw=_mapped(data,int(table["virtual_address"],16),size,loads,"DT_RELR")
        entries=decode_relr_records(raw,endian)
    else:
        offset,entries=0,[]
    table_names={
        DT_RELR:"DT_RELR",DT_RELRSZ:"DT_RELRSZ",DT_RELRENT:"DT_RELRENT",
        DT_ANDROID_RELR:"DT_ANDROID_RELR",DT_ANDROID_RELRSZ:"DT_ANDROID_RELRSZ",
        DT_ANDROID_RELRENT:"DT_ANDROID_RELRENT",DT_ANDROID_RELRCOUNT:"DT_ANDROID_RELRCOUNT",
    }
    dynamic_tag_names={**TAGS,**table_names}
    table_report={
        "source_tag":table["source_tag"],"source_tags":table["source_tags"],
        "virtual_address":table["virtual_address"],"file_offset":offset,
        "size":size,"entry_size":table["entry_size"],
        "encoded_entry_count":table["entry_count"],
        "relocation_count":len(entries),"entries":entries,
        "android_relr_count_hint":table.get("android_relr_count_hint"),
    }
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
      "elf_class":32,"machine":"ARM","endianness":ph["endianness"],
      "section_headers_present":ph["section_headers_present"],"dynamic_entries":dynamic,
      "dynamic_tags":{dynamic_tag_names.get(k,f"DT_UNKNOWN_{k}"):v for k,v in tags.items()},
      "relr_table":table_report,"relocation_count":len(entries),
      "evidence_limits":["ELF32 EM_ARM only.",
        "RELR offsets are decoded as relative relocation target locations; no target bytes are read or modified.",
        "No addend, runtime load bias, or relocated value is inferred.",
        "Standard and Android RELR tags are validated by the shared canonical decoder.",
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
