#!/usr/bin/env python3
"""Map and inventory ELF32 ARM dynamic REL/RELA tables from PT_DYNAMIC."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
from guest_arm_pt_dynamic_audit import parse_program_headers, map_vaddr, PT_LOAD, PT_DYNAMIC
DT_NULL=0; DT_HASH=4; DT_STRTAB=5; DT_SYMTAB=6; DT_RELA=7; DT_RELASZ=8
DT_RELAENT=9; DT_STRSZ=10; DT_SYMENT=11; DT_REL=17; DT_RELSZ=18
DT_RELENT=19; DT_PLTREL=20; DT_JMPREL=23; DT_PLTRELSZ=2
TAGS={0:"DT_NULL",2:"DT_PLTRELSZ",4:"DT_HASH",5:"DT_STRTAB",6:"DT_SYMTAB",
7:"DT_RELA",8:"DT_RELASZ",9:"DT_RELAENT",10:"DT_STRSZ",11:"DT_SYMENT",
17:"DT_REL",18:"DT_RELSZ",19:"DT_RELENT",20:"DT_PLTREL",23:"DT_JMPREL"}
RELOCS={0:"R_ARM_NONE",1:"R_ARM_PC24",2:"R_ARM_ABS32",3:"R_ARM_REL32",
10:"R_ARM_THM_CALL",21:"R_ARM_GLOB_DAT",22:"R_ARM_JUMP_SLOT",23:"R_ARM_RELATIVE",
28:"R_ARM_CALL",29:"R_ARM_JUMP24",30:"R_ARM_THM_JUMP24",38:"R_ARM_TARGET1",
42:"R_ARM_PREL31",43:"R_ARM_MOVW_ABS_NC",44:"R_ARM_MOVT_ABS",
47:"R_ARM_THM_MOVW_ABS_NC",48:"R_ARM_THM_MOVT_ABS"}
def _read_dynamic(data, ph):
    endian=ph["endian"]; loads=[x for x in ph["program_headers"] if x["type"]==PT_LOAD]
    segments=[x for x in ph["program_headers"] if x["type"]==PT_DYNAMIC]
    if len(segments)!=1: raise ELFAuditError("exactly one PT_DYNAMIC segment is required")
    seg=segments[0]
    if not seg["filesz"] or seg["filesz"]%8: raise ELFAuditError("invalid PT_DYNAMIC size")
    entries=[]; terminated=False
    for off in range(seg["offset"],seg["offset"]+seg["filesz"],8):
        tag,value=struct.unpack_from(endian+"iI",data,off)
        entries.append({"tag":tag,"tag_name":TAGS.get(tag,f"DT_UNKNOWN_{tag&0xffffffff}"),"value":value})
        if tag==DT_NULL: terminated=True; break
    if not terminated: raise ELFAuditError("PT_DYNAMIC has no DT_NULL terminator")
    tags={}
    for item in entries:
        if item["tag"] in (DT_NULL,): continue
        if item["tag"] in tags and tags[item["tag"]]!=item["value"]:
            raise ELFAuditError(f"conflicting duplicate {item['tag_name']}")
        tags[item["tag"]]=item["value"]
    return entries,tags,loads,endian
def _mapped(data,vaddr,size,loads,label):
    try: off=map_vaddr(vaddr,size,loads)
    except ELFAuditError as exc: raise ELFAuditError(f"{label}: {exc}") from exc
    return off,data[off:off+size]
def _parse_symbols(data,tags,loads,endian,max_index):
    required=(DT_SYMTAB,DT_SYMENT,DT_STRTAB,DT_STRSZ,DT_HASH)
    if any(x not in tags for x in required):
        return None,{"available":False,"reason":"DT_HASH/DT_SYMTAB/DT_STRTAB metadata incomplete"}
    if tags[DT_SYMENT]!=16: raise ELFAuditError("DT_SYMENT must be 16 for ELF32")
    _,hashhead=_mapped(data,tags[DT_HASH],8,loads,"DT_HASH header")
    nbucket,nchain=struct.unpack(endian+"II",hashhead)
    if nchain>1_000_000 or nbucket>1_000_000: raise ELFAuditError("DT_HASH dimensions exceed safety limit")
    hash_size=8+4*(nbucket+nchain)
    _mapped(data,tags[DT_HASH],hash_size,loads,"DT_HASH table")
    if max_index>=nchain: raise ELFAuditError(f"relocation symbol index {max_index} exceeds DT_HASH nchain {nchain}")
    sym_size=nchain*16
    _,symraw=_mapped(data,tags[DT_SYMTAB],sym_size,loads,"DT_SYMTAB")
    _,strraw=_mapped(data,tags[DT_STRTAB],tags[DT_STRSZ],loads,"DT_STRTAB")
    symbols=[]
    for index in range(nchain):
        name,value,size,info,other,shndx=struct.unpack_from(endian+"IIIBBH",symraw,index*16)
        if name>=len(strraw): raise ELFAuditError("dynamic symbol name offset exceeds DT_STRSZ")
        if name:
            end=strraw.find(b"\0",name)
            if end<0: raise ELFAuditError("unterminated dynamic symbol name")
            try: symbol_name=strraw[name:end].decode("utf-8")
            except UnicodeDecodeError as exc: raise ELFAuditError("invalid dynamic symbol name encoding") from exc
        else: symbol_name=""
        symbols.append({"index":index,"name":symbol_name,"value":f"0x{value:x}",
          "size":size,"binding":info>>4,"type":info&15,"visibility":other&3,
          "section_index":shndx,"undefined":shndx==0})
    return symbols,{"available":True,"symbol_count":nchain,"hash_buckets":nbucket}
def _reloc_table(data,address,size,entry_size,kind,loads,endian,label,symbols):
    expected=8 if kind=="REL" else 12
    if entry_size!=expected: raise ELFAuditError(f"{label}: entry size must be {expected}")
    if size%entry_size: raise ELFAuditError(f"{label}: size is not a multiple of entry size")
    if size>64*1024*1024: raise ELFAuditError(f"{label}: table exceeds safety limit")
    off,raw=_mapped(data,address,size,loads,label)
    entries=[]
    for index in range(size//entry_size):
        pos=index*entry_size
        if kind=="RELA": target,info,addend=struct.unpack_from(endian+"IIi",raw,pos)
        else: target,info=struct.unpack_from(endian+"II",raw,pos); addend=None
        sym_index=info>>8; typ=info&255
        if symbols is not None:
            if sym_index>=len(symbols): raise ELFAuditError(f"{label}: symbol index {sym_index} out of range")
            sym=symbols[sym_index]
            symbol={"index":sym_index,"name":sym["name"],"undefined":sym["undefined"]}
        else: symbol={"index":sym_index,"name":None,"undefined":None}
        entries.append({"index":index,"target_address":f"0x{target:08x}",
          "symbol_index":sym_index,"symbol":symbol["name"],"symbol_undefined":symbol["undefined"],
          "type":typ,"type_name":RELOCS.get(typ,f"R_ARM_UNKNOWN_{typ}"),"addend":addend})
    return {"kind":kind,"virtual_address":f"0x{address:08x}","file_offset":off,
      "size":size,"entry_size":entry_size,"entry_count":len(entries),"entries":entries}
def audit(data):
    ph=parse_program_headers(data); entries,tags,loads,endian=_read_dynamic(data,ph)
    specs=[]
    if DT_REL in tags or DT_RELSZ in tags:
        if DT_REL not in tags or DT_RELSZ not in tags or DT_RELENT not in tags:
            raise ELFAuditError("DT_REL, DT_RELSZ, and DT_RELENT must appear together")
        specs.append(("DT_REL",tags[DT_REL],tags[DT_RELSZ],tags[DT_RELENT],"REL"))
    if DT_RELA in tags or DT_RELASZ in tags:
        if DT_RELA not in tags or DT_RELASZ not in tags or DT_RELAENT not in tags:
            raise ELFAuditError("DT_RELA, DT_RELASZ, and DT_RELAENT must appear together")
        specs.append(("DT_RELA",tags[DT_RELA],tags[DT_RELASZ],tags[DT_RELAENT],"RELA"))
    if DT_JMPREL in tags or DT_PLTRELSZ in tags:
        if DT_JMPREL not in tags or DT_PLTRELSZ not in tags or DT_PLTREL not in tags:
            raise ELFAuditError("DT_JMPREL, DT_PLTRELSZ, and DT_PLTREL must appear together")
        if tags[DT_PLTREL]==DT_REL: kind="REL"; ent=tags.get(DT_RELENT)
        elif tags[DT_PLTREL]==DT_RELA: kind="RELA"; ent=tags.get(DT_RELAENT)
        else: raise ELFAuditError("DT_PLTREL must identify DT_REL or DT_RELA")
        if ent is None: raise ELFAuditError("PLT relocation entry-size tag missing")
        specs.append(("DT_JMPREL",tags[DT_JMPREL],tags[DT_PLTRELSZ],ent,kind))
    # Map and validate every range before decoding symbol references.
    max_index=0; raw_specs=[]
    for label,address,size,ent,kind in specs:
        expected=8 if kind=="REL" else 12
        if ent!=expected or size%ent: raise ELFAuditError(f"{label}: invalid entry size or table length")
        _,raw=_mapped(data,address,size,loads,label)
        raw_specs.append((label,address,size,ent,kind,raw))
        for pos in range(0,size,ent):
            info=struct.unpack_from(endian+"I",raw,pos+4)[0]
            max_index=max(max_index,info>>8)
    symbols,symbol_info=_parse_symbols(data,tags,loads,endian,max_index) if raw_specs else (None,{"available":False,"reason":"no dynamic relocation tables"})
    tables=[]
    for label,address,size,ent,kind,_ in raw_specs:
        tables.append({"source_tag":label,**_reloc_table(data,address,size,ent,kind,loads,endian,label,symbols)})
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
      "elf_class":32,"machine":"ARM","endianness":ph["endianness"],
      "section_headers_present":ph["section_headers_present"],"dynamic_entries":entries,
      "dynamic_tags":{TAGS.get(k,f"DT_UNKNOWN_{k}"):v for k,v in tags.items()},
      "symbol_resolution":symbol_info,"relocation_tables":tables,
      "relocation_count":sum(x["entry_count"] for x in tables),
      "evidence_limits":["Only ELF32 EM_ARM dynamic REL/RELA tables described by PT_DYNAMIC are supported.",
      "Symbol names are correlated only when a valid SysV DT_HASH table bounds DT_SYMTAB; GNU hash-only files remain unresolved.",
      "DT_JMPREL is reported separately and may overlap another declared range; this stage does not apply or deduplicate relocations.",
      "Relocation target addresses are reported, not dereferenced or modified.",
      "No relocation is applied, no runtime address is calculated, and no library provider is inferred."]}
def main():
    p=argparse.ArgumentParser(description=__doc__); p.add_argument("input",type=Path); p.add_argument("--json",type=Path); a=p.parse_args()
    try:
        report=audit(a.input.read_bytes()); report["source"]=a.input.name; rendered=json.dumps(report,indent=2)
        if a.json: a.json.parent.mkdir(parents=True,exist_ok=True); a.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: p.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
