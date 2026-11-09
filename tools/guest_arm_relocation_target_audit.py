#!/usr/bin/env python3
"""Classify relocation target ranges against ELF32 PT_LOAD memory mappings."""
from __future__ import annotations
from guest_arm_elf_audit import ELFAuditError
from guest_arm_relocation_abi import known_widths, get_spec

# ARM ELF relocation access widths. Unknown types intentionally remain unknown.
ARM_WIDTHS=known_widths()
MAX_TARGETS=4_000_000
ADDRESS_LIMIT=0x100000000

def classify_target(target, width, loads, alignment=1):
    if not isinstance(alignment,int) or alignment<=0:
        raise ELFAuditError("relocation target alignment must be positive")
    if width is not None and (not isinstance(width,int) or width<0):
        raise ELFAuditError("relocation target width must be nonnegative or unknown")
    if width==0:
        return {"status":"no_target_access","matches":[]}
    if not isinstance(target,int) or target<0 or target>=ADDRESS_LIMIT:
        return {"status":"invalid_target_address","matches":[]}
    end=target+(width or 0)
    if end>ADDRESS_LIMIT:
        return {"status":"target_range_overflows_elf32","matches":[]}
    matches=[]
    for segment in loads:
        start=segment["vaddr"]
        memory_end=start+segment["memsz"]
        file_end=start+segment["filesz"]
        if memory_end>ADDRESS_LIMIT:
            # Program-header parser normally prevents this, but don't trust
            # caller-provided segment dictionaries.
            raise ELFAuditError("PT_LOAD memory range overflows ELF32")
        if target<start or target>=memory_end:
            continue
        if width is not None and end>memory_end:
            region="crosses_segment_end"
        elif target<file_end and (width is None or end<=file_end):
            region="file_backed"
        elif target>=file_end:
            region="zero_fill_bss"
        else:
            region="crosses_file_backed_into_bss"
        matches.append({
            "segment_index":segment["index"],
            "segment_vaddr":f"0x{start:08x}",
            "segment_memsz":segment["memsz"],
            "segment_filesz":segment["filesz"],
            "region":region,
            "target_file_offset":segment["offset"]+(target-start) if target<file_end else None,
        })
    if not matches:
        status="outside_all_pt_load_segments"
    elif len(matches)>1:
        status="ambiguous_overlapping_pt_load_segments"
    else:
        status=matches[0]["region"]
    if width is None and len(matches)==1:
        status="unknown_width_"+status
    return {"status":status,"matches":matches,"required_alignment":alignment,
        "alignment_valid":target%alignment==0}

def audit_target_ranges(reconciliation, program_headers):
    loads=[p for p in program_headers["program_headers"] if p["type"]==1]
    report=[]
    counts={}
    for table in reconciliation.get("tables",[]):
        for entry in table.get("entries",[]):
            if entry.get("target_domain")!="virtual":
                item={"status":"non_virtual_target_domain","matches":[]}
                width=None
            else:
                typ=entry.get("type")
                spec=get_spec(typ)
                width=spec["width"] if spec else None
                alignment=spec["alignment"] if spec else 1
                if entry.get("encoding")=="RELR" or table.get("encoding")=="RELR":
                    width=4
                    alignment=4
                item=classify_target(entry.get("target"),width,loads,alignment)
            item.update({
                "source_table":table.get("source_table"),
                "entry_index":entry.get("index"),
                "target_domain":entry.get("target_domain"),
                "target":entry.get("target"),
                "relocation_type":entry.get("type"),
                "relocation_type_name":entry.get("type_name"),
                "abi_spec":get_spec(entry.get("type")),
                "required_alignment":item.get("required_alignment"),
                "alignment_valid":item.get("alignment_valid"),
                "access_width":width,
                "width_known":width is not None,
            })
            report.append(item)
            counts[item["status"]]=counts.get(item["status"],0)+1
            entry["target_range"]=item
            entry["target_access_width"]=width
    if len(report)>MAX_TARGETS:
        raise ELFAuditError("relocation target count exceeds safety limit")
    return {
        "target_count":len(report),
        "status_counts":counts,
        "targets":report,
        "policy":[
            "PT_LOAD memory ranges are half-open [p_vaddr, p_vaddr + p_memsz).",
            "File-backed bytes occupy [p_vaddr, p_vaddr + p_filesz); the remaining memory range is zero-fill/BSS.",
            "Only explicitly known ARM relocation widths are validated; unknown types retain unknown width.",
            "No relocation target bytes are read or modified and no load bias is calculated.",
        ],
    }
