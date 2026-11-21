#!/usr/bin/env python3
"""Inventory ELF32 ARM function symbols and correlate relocation references."""
from __future__ import annotations
import argparse, collections, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import audit_elf, ELFAuditError
from guest_arm_instruction_inventory import parse_mapping_symbols, mapping_ranges
from guest_arm_relocation_audit import audit_relocations
SHT_SYMTAB=2
SHT_DYNSYM=11
STT_FUNC=2
SHF_EXECINSTR=0x4
SHN_UNDEF=0
SHN_ABS=0xfff1
CALL_JUMP_TYPES={1,10,28,29,30,50}
def _cstring(blob,offset):
    if offset==0: return ""
    if offset<0 or offset>=len(blob): raise ELFAuditError("symbol name offset out of range")
    end=blob.find(b"\0",offset)
    if end<0: raise ELFAuditError("unterminated symbol name")
    try: return blob[offset:end].decode("utf-8")
    except UnicodeDecodeError as exc: raise ELFAuditError("invalid symbol name") from exc
def parse_function_symbols(data,elf):
    if elf["elf_class"]!=32 or elf["machine"]!=40: raise ELFAuditError("function-symbol inventory currently requires ELF32 EM_ARM")
    endian="<" if elf["endianness"]=="little" else ">"; sections=elf["sections"]; output=[]
    for table in sections:
        if table["type"] not in (SHT_SYMTAB,SHT_DYNSYM): continue
        if table["link"]>=len(sections): raise ELFAuditError("symbol table string-table link out of range")
        strings=sections[table["link"]]
        if strings["type"]!=3: raise ELFAuditError("symbol table link does not reference SHT_STRTAB")
        if table["entsize"]<16 or table["size"]%table["entsize"]: raise ELFAuditError("invalid ELF32 symbol-table entry size")
        if table["offset"]+table["size"]>len(data) or strings["offset"]+strings["size"]>len(data):
            raise ELFAuditError("symbol or string table exceeds file bounds")
        strtab=data[strings["offset"]:strings["offset"]+strings["size"]]
        for index in range(table["size"]//table["entsize"]):
            off=table["offset"]+index*table["entsize"]
            name_off,value,size,info,other,shndx=struct.unpack_from(endian+"IIIBBH",data,off)
            if (info&15)!=STT_FUNC: continue
            name=_cstring(strtab,name_off)
            record={"symbol_table_index":table["index"],"symbol_table":table["name"],"symbol_index":index,
                "name":name,"value":value,"size":size,"binding":info>>4,"visibility":other&3,
                "section_index":shndx,"thumb_state_bit":bool(value&1)}
            if shndx==SHN_UNDEF:
                record.update({"section_status":"undefined/external","start_offset":None,"mapping_classification":"unresolved"})
            elif shndx==SHN_ABS:
                record.update({"section_status":"absolute","start_offset":None,"mapping_classification":"absolute"})
            elif shndx>=0xff00 or shndx>=len(sections):
                record.update({"section_status":"special/unavailable","start_offset":None,"mapping_classification":"unknown-special"})
            else:
                section=sections[shndx]; raw_start=value&~1
                start=raw_start if elf["type"]==1 else raw_start-int(section["address"],16)
                record["section"]=section["name"]; record["start_offset"]=start
                if start<0 or start>section["size"]:
                    record.update({"section_status":"out-of-section","mapping_classification":"out-of-bounds","overlaps":[]})
                elif not (section["flags"]&SHF_EXECINSTR):
                    record.update({"section_status":"defined-non-executable","mapping_classification":"non-executable","overlaps":[]})
                else:
                    record["section_status"]="defined-executable"; record["end_offset"]=start+size
                    record["extent_within_section"]=start+size<=section["size"]; record["zero_size"]=size==0
            output.append(record)
    return output
def _classify_symbols(symbols,elf,mappings):
    sections=elf["sections"]; section_by_index={s["index"]:s for s in sections}; counts=collections.Counter()
    for sym in symbols:
        if sym["section_status"]!="defined-executable":
            counts[sym["mapping_classification"]]+=1; continue
        idx=sym["section_index"]; section=section_by_index[idx]
        ranges=mapping_ranges(mappings.get(idx,[]),section["size"]) if mappings.get(idx) else (
            [{"kind":"unknown","start":0,"end":section["size"]}] if section["size"] else [])
        start=sym["start_offset"]; size=sym["size"]
        end=start+size if size else min(start+1,section["size"])
        overlaps=[]
        for span in ranges:
            left=max(start,span["start"]); right=min(end,span["end"])
            if left<right: overlaps.append({"kind":span["kind"],"start":left,"end":right,"size":right-left,"mapping_symbol":span.get("mapping_symbol")})
        kinds=sorted({x["kind"] for x in overlaps}); sym["overlaps"]=overlaps
        if not size: classification="zero-size"
        elif not sym["extent_within_section"]: classification="extends-past-section"
        elif not overlaps: classification="unmapped"
        elif len(kinds)>1: classification="mixed"
        elif kinds[0]=="a": classification="arm"
        elif kinds[0]=="t": classification="thumb"
        elif kinds[0]=="d": classification="data"
        else: classification="unknown"
        sym["mapping_classification"]=classification; counts[classification]+=1
    return dict(counts)
def inventory(data):
    elf=audit_elf(data); symbols=parse_function_symbols(data,elf); mappings=parse_mapping_symbols(data,elf)
    counts=_classify_symbols(symbols,elf,mappings); relocation_report=audit_relocations(data); refs=collections.defaultdict(list)
    for relsec in relocation_report["sections"]:
        for entry in relsec["entries"]:
            if entry["type"] not in CALL_JUMP_TYPES: continue
            key=(relsec["symbol_table_index"],entry["symbol_index"])
            refs[key].append({"relocation_section":relsec["name"],"relocation_type":entry["type_name"],
                "site_offset":entry["target_section_offset"],"target_symbol_name":entry["symbol"]})
    for sym in symbols: sym["call_jump_references"]=refs.get((sym["symbol_table_index"],sym["symbol_index"]),[])
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),"elf_class":32,"machine":"ARM",
        "endianness":elf["endianness"],"function_symbol_count":len(symbols),"function_symbols_by_classification":counts,
        "mapping_symbol_sections":len(mappings),"call_jump_relocation_references":sum(len(x["call_jump_references"]) for x in symbols),
        "symbols":symbols,"evidence_limits":["Only STT_FUNC entries in conventional ELF32 SYMTAB/DYNSYM are inventoried.",
        "A zero st_size symbol has no proven extent; only its start location is classified.",
        "ARM Thumb function state is recorded from st_value bit zero; the bit is cleared for section offset calculation.",
        "Mapping overlap describes ELF metadata, not proof of valid function boundaries or runtime control flow.",
        "Relocation references are joined by symbol-table and symbol index; no relocation is applied.",
        "No symbol name or import provider is inferred when absent."]}
def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument("input",type=Path); parser.add_argument("--json",type=Path); args=parser.parse_args()
    try:
        report=inventory(args.input.read_bytes()); report["source"]=args.input.name; rendered=json.dumps(report,indent=2)
        if args.json: args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
