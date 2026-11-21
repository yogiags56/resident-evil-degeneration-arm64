#!/usr/bin/env python3
"""Read-only ELF32 ARM dynamic import, dependency, and relocation inventory."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import audit_elf, ELFAuditError
SHT_DYNSYM=11
SHT_DYNAMIC=6
SHT_REL=9
SHT_RELA=4
DT_NULL=0
DT_NEEDED=1
DT_PLTRELSZ=2
DT_PLTGOT=3
DT_HASH=4
DT_STRTAB=5
DT_SYMTAB=6
DT_RELA=7
DT_RELASZ=8
DT_RELAENT=9
DT_STRSZ=10
DT_SYMENT=11
DT_SONAME=14
DT_RPATH=15
DT_REL=17
DT_RELSZ=18
DT_RELENT=19
DT_PLTREL=20
DT_JMPREL=23
DT_RUNPATH=29
DT_FLAGS=30
DT_VERSYM=0x6ffffff0
DT_VERDEF=0x6ffffffc
DT_VERDEFNUM=0x6ffffffd
DT_VERNEED=0x6ffffffe
DT_VERNEEDNUM=0x6fffffff
DYNAMIC_NAMES={DT_NEEDED:"DT_NEEDED",DT_PLTRELSZ:"DT_PLTRELSZ",DT_PLTGOT:"DT_PLTGOT",
 DT_HASH:"DT_HASH",DT_STRTAB:"DT_STRTAB",DT_SYMTAB:"DT_SYMTAB",DT_RELA:"DT_RELA",
 DT_RELASZ:"DT_RELASZ",DT_RELAENT:"DT_RELAENT",DT_STRSZ:"DT_STRSZ",DT_SYMENT:"DT_SYMENT",
 DT_SONAME:"DT_SONAME",DT_RPATH:"DT_RPATH",DT_REL:"DT_REL",DT_RELSZ:"DT_RELSZ",
 DT_RELENT:"DT_RELENT",DT_PLTREL:"DT_PLTREL",DT_JMPREL:"DT_JMPREL",DT_RUNPATH:"DT_RUNPATH",
 DT_FLAGS:"DT_FLAGS",DT_VERSYM:"DT_VERSYM",DT_VERDEF:"DT_VERDEF",DT_VERDEFNUM:"DT_VERDEFNUM",
 DT_VERNEED:"DT_VERNEED",DT_VERNEEDNUM:"DT_VERNEEDNUM"}
RELOC_NAMES={0:"R_ARM_NONE",1:"R_ARM_PC24",2:"R_ARM_ABS32",3:"R_ARM_REL32",
 10:"R_ARM_THM_CALL",21:"R_ARM_GLOB_DAT",22:"R_ARM_JUMP_SLOT",23:"R_ARM_RELATIVE",
 28:"R_ARM_CALL",29:"R_ARM_JUMP24",30:"R_ARM_THM_JUMP24",38:"R_ARM_TARGET1",
 42:"R_ARM_PREL31",43:"R_ARM_MOVW_ABS_NC",44:"R_ARM_MOVT_ABS",
 47:"R_ARM_THM_MOVW_ABS_NC",48:"R_ARM_THM_MOVT_ABS"}
STRING_TAGS={DT_NEEDED,DT_SONAME,DT_RPATH,DT_RUNPATH}
def _cstring(blob,off,label):
    if off<0 or off>=len(blob): raise ELFAuditError(f"{label} string offset out of range")
    end=blob.find(b"\0",off)
    if end<0: raise ELFAuditError(f"unterminated {label} string")
    try: return blob[off:end].decode("utf-8")
    except UnicodeDecodeError as exc: raise ELFAuditError(f"invalid {label} string encoding") from exc
def _table_bytes(data,section,label):
    off,size=section["offset"],section["size"]
    if off<0 or size<0 or off+size>len(data): raise ELFAuditError(f"{label} exceeds file bounds")
    return data[off:off+size]
def _symbols(data,elf,section):
    sections=elf["sections"]
    if section["type"]!=SHT_DYNSYM: raise ELFAuditError("dynamic symbol section has wrong type")
    if section["link"]>=len(sections): raise ELFAuditError("DYNSYM string-table link out of range")
    strings=sections[section["link"]]
    if strings["type"]!=3: raise ELFAuditError("DYNSYM link does not reference SHT_STRTAB")
    entsize=section["entsize"]
    if entsize<16 or entsize==0 or section["size"]%entsize: raise ELFAuditError("invalid DYNSYM entry size")
    names=_table_bytes(data,strings,"dynamic string table"); raw=_table_bytes(data,section,"dynamic symbol table")
    endian="<" if elf["endianness"]=="little" else ">"; result=[]
    for index in range(section["size"]//entsize):
        name,value,size,info,other,shndx=struct.unpack_from(endian+"IIIBBH",raw,index*entsize)
        result.append({"index":index,"name":_cstring(names,name,"dynamic symbol") if name else "",
          "value":value,"size":size,"binding":info>>4,"type":info&15,
          "visibility":other&3,"section_index":shndx,
          "defined":shndx!=0,"undefined":shndx==0})
    return result,names
def _dynamic_entries(data,elf,section,names):
    if section["type"]!=SHT_DYNAMIC: raise ELFAuditError("dynamic section has wrong type")
    if section["link"]>=len(elf["sections"]): raise ELFAuditError("SHT_DYNAMIC string-table link out of range")
    if elf["sections"][section["link"]]["type"]!=3: raise ELFAuditError("SHT_DYNAMIC link does not reference SHT_STRTAB")
    entsize=section["entsize"] or 8
    if entsize<8 or section["size"]%entsize: raise ELFAuditError("invalid SHT_DYNAMIC entry size")
    raw=_table_bytes(data,section,"dynamic table"); endian="<" if elf["endianness"]=="little" else ">"; out=[]
    terminated=False
    for index in range(section["size"]//entsize):
        tag,value=struct.unpack_from(endian+"iI",raw,index*entsize)
        if tag==DT_NULL:
            terminated=True
            out.append({"tag":tag,"tag_name":"DT_NULL","value":value})
            break
        item={"tag":tag,"tag_name":DYNAMIC_NAMES.get(tag,f"DT_UNKNOWN_{tag & 0xffffffff}"),"value":value}
        if tag in STRING_TAGS: item["string"]=_cstring(names,value,item["tag_name"])
        out.append(item)
    if not terminated: raise ELFAuditError("SHT_DYNAMIC has no DT_NULL terminator")
    return out
def _dynamic_relocations(data,elf,dynsym):
    sections=elf["sections"]; endian="<" if elf["endianness"]=="little" else ">"; reports=[]
    for section in sections:
        if section["type"] not in (SHT_REL,SHT_RELA): continue
        if section["link"]>=len(sections) or sections[section["link"]]["type"]!=SHT_DYNSYM: continue
        if section["info"]>=len(sections): raise ELFAuditError(f"invalid target section index in {section['name']}")
        target=sections[section["info"]]; rela=section["type"]==SHT_RELA
        expected=12 if rela else 8; entsize=section["entsize"]
        if entsize<expected or section["size"]%entsize: raise ELFAuditError(f"invalid dynamic relocation entry size in {section['name']}")
        raw=_table_bytes(data,section,"dynamic relocation section"); entries=[]
        for index in range(section["size"]//entsize):
            pos=index*entsize
            if rela: offset,info,addend=struct.unpack_from(endian+"IIi",raw,pos)
            else: offset,info=struct.unpack_from(endian+"II",raw,pos); addend=None
            sym_index=info>>8; typ=info&255
            if sym_index>=len(dynsym): raise ELFAuditError(f"dynamic relocation symbol index {sym_index} out of range")
            target_off=offset if elf["type"]==1 else offset-int(target["address"],16)
            if target_off<0 or target_off>=target["size"]: raise ELFAuditError(f"dynamic relocation offset outside {target['name']}")
            sym=dynsym[sym_index]
            entries.append({"offset":f"0x{offset:x}","target_section_offset":target_off,
              "symbol_index":sym_index,"symbol":sym["name"],"symbol_undefined":sym["undefined"],
              "type":typ,"type_name":RELOC_NAMES.get(typ,f"R_ARM_UNKNOWN_{typ}"),"addend":addend})
        reports.append({"name":section["name"],"type":"RELA" if rela else "REL",
          "symbol_table_index":section["link"],"target_section":target["name"],
          "entry_count":len(entries),"entries":entries})
    return reports
def inventory(data):
    elf=audit_elf(data)
    if elf["elf_class"]!=32 or elf["machine"]!=40: raise ELFAuditError("dynamic import inventory currently requires ELF32 EM_ARM")
    sections=elf["sections"]; dynsym_sections=[s for s in sections if s["type"]==SHT_DYNSYM]
    dynamic_sections=[s for s in sections if s["type"]==SHT_DYNAMIC]
    symbol_reports=[]; all_undefined=[]; dynamic_reports=[]
    for section in dynsym_sections:
        symbols,names=_symbols(data,elf,section)
        undefined=[s for s in symbols if s["undefined"] and s["name"]]
        symbol_reports.append({"section":section["name"],"section_index":section["index"],
          "symbol_count":len(symbols),"undefined_named_count":len(undefined),
          "undefined_symbols":undefined})
        all_undefined.extend(undefined)
    for section in dynamic_sections:
        linked=sections[section["link"]] if section["link"]<len(sections) else None
        if linked is None or linked["type"]!=3: raise ELFAuditError("dynamic section has invalid linked string table")
        names=_table_bytes(data,linked,"dynamic string table")
        entries=_dynamic_entries(data,elf,section,names)
        dynamic_reports.append({"section":section["name"],"section_index":section["index"],"entries":entries})
    relocations=[]
    for symbols_section in dynsym_sections:
        symbols,_=_symbols(data,elf,symbols_section)
        relocations.extend(_dynamic_relocations(data,elf,symbols))
    dependencies=[]
    for report in dynamic_reports:
        for entry in report["entries"]:
            if entry["tag"]==DT_NEEDED: dependencies.append(entry["string"])
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
      "elf_class":32,"machine":"ARM","endianness":elf["endianness"],
      "dynamic_symbol_tables":symbol_reports,"undefined_dynamic_symbol_count":len(all_undefined),
      "dynamic_dependencies":dependencies,"dynamic_sections":dynamic_reports,
      "dynamic_relocation_sections":relocations,
      "dynamic_relocation_count":sum(x["entry_count"] for x in relocations),
      "evidence_limits":["Only ELF32 EM_ARM conventional SHT_DYNSYM, SHT_DYNAMIC, SHT_REL and SHT_RELA sections are supported.",
        "Undefined dynamic symbols are candidates for external resolution; their provider/API identity is not inferred.",
        "Dynamic relocation records are reported, not applied; final runtime addresses are not calculated.",
        "Symbol version sections are not decoded in this stage; DT_VERSYM/DT_VERNEED tags are preserved numerically.",
        "Stripped binaries may omit section headers; PT_DYNAMIC-only fallback is deferred.",
        "No proprietary game binary is included or analyzed."]}
def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument("input",type=Path); parser.add_argument("--json",type=Path); args=parser.parse_args()
    try:
        report=inventory(args.input.read_bytes()); report["source"]=args.input.name
        rendered=json.dumps(report,indent=2)
        if args.json: args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
