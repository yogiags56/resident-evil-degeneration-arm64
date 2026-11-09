#!/usr/bin/env python3
"""Reconcile section-backed, PT_DYNAMIC REL/RELA, and Android APS2 inventories."""
from __future__ import annotations
import argparse, hashlib, json, sys
from pathlib import Path
TOOLS=Path(__file__).resolve().parent
if str(TOOLS) not in sys.path: sys.path.insert(0,str(TOOLS))
from guest_arm_elf_audit import audit_elf, ELFAuditError
from guest_arm_relocation_audit import audit_relocations
from guest_arm_dynamic_relocation_segment_audit import audit as audit_dynamic, _read_dynamic
from guest_arm_aps2_relocation_audit import audit as audit_aps2
from guest_arm_relr_audit import audit as audit_relr
from guest_arm_pt_dynamic_audit import parse_program_headers
from guest_arm_relocation_target_audit import audit_target_ranges
from guest_arm_relocation_abi import get_spec
ET_EXEC,ET_DYN=2,3

def _section_tables(report, elf):
    result=[]; sections=elf.get("sections",[])
    for table in report.get("sections",[]):
        candidates=[s for s in sections if s["type"] in (4,9) and s["name"]==table["name"]]
        if len(candidates)!=1: raise ELFAuditError(f"cannot uniquely identify relocation section: {table['name']}")
        relsec=candidates[0]; is_virtual=elf["type"] in (ET_EXEC,ET_DYN); entries=[]
        for e in table["entries"]:
            entries.append({"index":len(entries),"target_domain":"virtual" if is_virtual else f"section:{table['target_section_index']}",
                "target":int(e["offset"],16) if is_virtual else e["target_section_offset"],
                "symbol_index":e["symbol_index"],"symbol":e["symbol"],"type":e["type"],
                "type_name":e["type_name"],"addend":e["addend"],
                "symbol_namespace":f"SHT_SYMTAB:{table['symbol_table_index']}","symbol_version":None})
        result.append({"source_table":f"SHT_{table['type']}:{table['name']}","encoding":table["type"],
            "kind":table["type"],"file_offset":relsec["offset"],"size":relsec["size"],
            "entry_count":len(entries),"entries":entries})
    return result

def _convert(report, namespace, packed=False):
    result=[]
    tables=report.get("packed_relocation_tables",[]) if packed else report.get("relocation_tables",[])
    for table in tables:
        entries=[]
        for e in table["entries"]:
            entries.append({"index":e["index"],"target_domain":"virtual","target":int(e["target_address"],16),
                "symbol_index":e["symbol_index"],"symbol":e.get("symbol"),"type":e["type"],
                "type_name":e["type_name"],"addend":e.get("addend"),"symbol_namespace":namespace,"symbol_version":e.get("symbol_version")})
        result.append({"source_table":table["source_tag"],"encoding":f"APS2_{table['kind']}" if packed else table["kind"],
            "kind":table["kind"],"file_offset":table["file_offset"],
            "size":table["packed_size"] if packed else table["size"],
            "entry_count":len(entries),"entries":entries})
    return result


def _convert_relr(report):
    table=report["relr_table"]
    entries=[]
    for item in table["entries"]:
        entries.append({"index":item["index"],"target_domain":"virtual",
          "target":item["target_offset"],"symbol_index":None,"symbol":None,
          "type":23,"type_name":"R_ARM_RELATIVE","addend":None,
          "symbol_namespace":"RELR:relative-only","symbol_version":None,
          "relr_encoding":item["encoding"],"table_entry_index":item["table_entry_index"]})
    return [{"source_table":"DT_RELR","encoding":"RELR","kind":"RELR",
      "file_offset":table["file_offset"],"size":table["size"],
      "entry_count":len(entries),"entries":entries}]

def _version_identity(e):
    version=e.get("symbol_version")
    if not version: return None
    return (version.get("index"),version.get("kind"),version.get("name"),version.get("library"),version.get("hidden"))
def _core(e):
    return (e["target_domain"],e["target"],e["type"],e["symbol_index"],e["addend"],_version_identity(e))
def _identity(e):
    return _core(e)+(e["symbol_namespace"],)

def reconcile_tables(tables):
    records=[{**e,"source_table":t["source_table"]} for t in tables for e in t["entries"]]
    exact={}; core={}
    for e in records:
        exact.setdefault(_identity(e),[]).append(e)
        core.setdefault(_core(e),[]).append(e)
    duplicates=[]
    for key,items in exact.items():
        sources=sorted({x["source_table"] for x in items})
        if len(sources)>1:
            duplicates.append({"target_domain":key[0],"target":key[1],"type":key[2],
                "symbol_index":key[3],"addend":key[4],"symbol_version":items[0].get("symbol_version"),"symbol_namespace":key[6],
                "sources":sources,"occurrences":len(items)})
    candidates=[]
    for key,items in core.items():
        sources=sorted({x["source_table"] for x in items})
        namespaces=sorted({x["symbol_namespace"] for x in items})
        if len(sources)>1 and len(namespaces)>1:
            candidates.append({"target_domain":key[0],"target":key[1],"type":key[2],
                "symbol_index":key[3],"addend":key[4],"symbol_version":items[0].get("symbol_version"),"symbol_namespaces":namespaces,
                "sources":sources,"classification":"candidate_only_symbol_table_identity_not_proven"})
    overlaps=[]
    for i,left in enumerate(tables):
        a,b=left["file_offset"],left["file_offset"]+left["size"]
        for right in tables[i+1:]:
            c,d=right["file_offset"],right["file_offset"]+right["size"]
            start,end=max(a,c),min(b,d)
            if start<end:
                overlaps.append({"left":left["source_table"],"right":right["source_table"],
                    "overlap_file_offset":start,"overlap_size":end-start,
                    "relationship":"exact_same_range" if a==c and b==d else "partial_overlap"})
    return {"table_count":len(tables),"raw_relocation_count":len(records),
        "duplicate_groups":duplicates,"duplicate_group_count":len(duplicates),
        "namespace_ambiguous_matches":candidates,"namespace_ambiguous_match_count":len(candidates),
        "file_range_overlaps":overlaps,"file_range_overlap_count":len(overlaps),
        "records":records,"policy":[
            "No source table or relocation record is removed or silently deduplicated.",
            "Exact duplicates require matching target, type, symbol index, addend, symbol version identity, and symbol-table namespace.",
            "Matching tuples from different symbol-table namespaces are candidates only.",
            "REL implicit addends remain null and are not reconstructed from target bytes.",
            "File-range overlap is reported independently from semantic record equality."]}

def audit(data):
    # PT_DYNAMIC/PT_LOAD parsing is the primary path and must work even when
    # section headers have been stripped from the ELF.
    ph=parse_program_headers(data)
    if len(data)<52 or data[4]!=1 or ph["machine"]!=40:
        raise ELFAuditError("Stage 120F requires ELF32 EM_ARM")
    elf=None
    sec_tables=[]
    if ph["section_headers_present"]:
        try:
            elf=audit_elf(data)
            sec_report=audit_relocations(data)
            sec_tables=_section_tables(sec_report,elf)
            sec_status={"available":True,"reason":None}
        except ELFAuditError as exc:
            sec_status={"available":False,"reason":str(exc)}
    else:
        sec_status={"available":False,"reason":"ELF has no section headers; section-backed inventory unavailable"}
    dyn=audit_dynamic(data)
    _,tags,_,_=_read_dynamic(data,ph)
    has_aps2=any(tag in tags for tag in (0x6000000f,0x60000011))
    has_relr=any(tag in tags for tag in (35,36,37))
    aps=audit_aps2(data) if has_aps2 else {"packed_relocation_tables":[],"symbol_resolution":{"available":False,"reason":"no APS2 dynamic tags"}}
    relr=audit_relr(data) if has_relr else None
    symaddr=dyn.get("dynamic_tags",{}).get("DT_SYMTAB")
    namespace=f"DT_SYMTAB@0x{symaddr:08x}" if symaddr is not None else "DT_SYMTAB:unavailable"
    tables=sec_tables+_convert(dyn,namespace)+_convert(aps,namespace,packed=True)+( _convert_relr(relr) if relr else [])
    reconciliation=reconcile_tables(tables)
    target_ranges=audit_target_ranges({"tables":tables},ph)
    abi_counts={}
    for table in tables:
        for entry in table["entries"]:
            code=entry.get("type")
            spec=get_spec(code)
            key=spec["name"] if spec else f"R_ARM_UNKNOWN_{code}"
            abi_counts[key]=abi_counts.get(key,0)+1
    abi_known=sum(count for name,count in abi_counts.items() if not name.startswith("R_ARM_UNKNOWN_"))
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
        "elf_class":32,"machine":"ARM","endianness":ph["endianness"],"elf_type":ph["type"],
        "section_headers_present":ph["section_headers_present"],"section_backed_audit":sec_status,
        "dynamic_symbol_resolution":dyn.get("symbol_resolution"),
        "aps2_symbol_resolution":aps.get("symbol_resolution"),"relr_relocation_count":relr.get("relocation_count",0) if relr else 0,"tables":tables,
        "reconciliation":reconciliation,"target_range_audit":target_ranges,
        "relocation_abi":{"registry_entry_count":len(__import__("guest_arm_relocation_abi").SPECS),
            "known_relocation_count":abi_known,"unknown_relocation_count":sum(abi_counts.values())-abi_known,
            "type_counts":abi_counts,"unknown_types_preserved":True},
        "evidence_limits":[
            "ELF32 EM_ARM only; input is parsed read-only.",
            "Section-backed reconciliation uses the conventional SHT_SYMTAB Stage 119X parser; unsupported combinations are reported unavailable.",
            "Dynamic, APS2, and RELR inventories use PT_DYNAMIC/PT_LOAD; RELR has no symbol index or addend.",
            "Different symbol-table namespaces are never equated by index alone.",
            "REL implicit addends are not reconstructed from target memory.",
            "Relocation target ranges are classified against PT_LOAD without dereferencing target bytes.",
            "No relocation is applied and no provider or game boot is inferred."]}

def main():
    p=argparse.ArgumentParser(description=__doc__); p.add_argument("input",type=Path); p.add_argument("--json",type=Path); a=p.parse_args()
    try:
        report=audit(a.input.read_bytes()); report["source"]=a.input.name
        rendered=json.dumps(report,indent=2)
        if a.json: a.json.parent.mkdir(parents=True,exist_ok=True); a.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: p.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
