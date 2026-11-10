#!/usr/bin/env python3
"""Section-header-independent ELF32 ARM PT_DYNAMIC/PT_LOAD metadata auditor."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
PT_LOAD=1
PT_DYNAMIC=2
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
DT_VERSYM=0x6ffffff0
TAG_NAMES={0:"DT_NULL",1:"DT_NEEDED",2:"DT_PLTRELSZ",3:"DT_PLTGOT",4:"DT_HASH",
5:"DT_STRTAB",6:"DT_SYMTAB",7:"DT_RELA",8:"DT_RELASZ",9:"DT_RELAENT",10:"DT_STRSZ",
11:"DT_SYMENT",14:"DT_SONAME",15:"DT_RPATH",17:"DT_REL",18:"DT_RELSZ",19:"DT_RELENT",
20:"DT_PLTREL",23:"DT_JMPREL",29:"DT_RUNPATH",0x6ffffff0:"DT_VERSYM"}
STRING_TAGS={DT_NEEDED,DT_SONAME,DT_RPATH,DT_RUNPATH}
def _unpack(fmt,data,off,label):
    size=struct.calcsize(fmt)
    if off<0 or off+size>len(data): raise ELFAuditError(label+" out of bounds")
    return struct.unpack_from(fmt,data,off)
def _cstring(data,off,label):
    if off<0 or off>=len(data): raise ELFAuditError(label+" offset out of range")
    end=data.find(b"\0",off)
    if end<0: raise ELFAuditError("unterminated "+label)
    try: return data[off:end].decode("utf-8")
    except UnicodeDecodeError as exc: raise ELFAuditError("invalid "+label+" encoding") from exc
def parse_program_headers(data):
    if len(data)<52 or data[:4]!=b"\x7fELF": raise ELFAuditError("input is not a complete ELF file")
    ident=data[4:16]; cls,enc,version=ident[0],ident[1],ident[2]
    if cls!=1: raise ELFAuditError("PT_DYNAMIC audit currently requires ELF32")
    if enc not in (1,2) or version!=1: raise ELFAuditError("unsupported ELF byte order or ident version")
    endian="<" if enc==1 else ">"
    fields=_unpack(endian+"HHIIIIIHHHHHH",data,16,"ELF header")
    etype,machine,eversion,entry,phoff,shoff,flags,ehsize,phentsize,phnum,shentsize,shnum,shstrndx=fields
    if machine!=40: raise ELFAuditError("PT_DYNAMIC audit currently requires EM_ARM")
    if eversion!=1 or ehsize<52 or ehsize>len(data): raise ELFAuditError("invalid ELF32 header")
    if phnum==0 or phentsize<32 or phoff+phentsize*phnum>len(data): raise ELFAuditError("program header table missing or out of bounds")
    headers=[]
    for index in range(phnum):
        p=_unpack(endian+"IIIIIIII",data,phoff+index*phentsize,"program header")
        ptype,offset,vaddr,paddr,filesz,memsz,pflags,align=p
        if filesz>memsz: raise ELFAuditError("segment file size exceeds memory size")
        if offset+filesz>len(data): raise ELFAuditError("segment file range exceeds input")
        if align not in (0,1) and (align&(align-1)): raise ELFAuditError("segment alignment is not a power of two")
        if align>1 and (vaddr-offset)%align: raise ELFAuditError("segment address/offset alignment mismatch")
        headers.append({"index":index,"type":ptype,"offset":offset,"vaddr":vaddr,"paddr":paddr,
          "filesz":filesz,"memsz":memsz,"flags":pflags,"align":align})
    return {"endianness":"little" if enc==1 else "big","endian":endian,"type":etype,
      "machine":machine,"entry":entry,"flags":flags,"program_headers":headers,
      "section_header_count":shnum,"section_headers_present":shnum!=0}
def map_vaddr(vaddr,size,loads):
    if size<0: raise ELFAuditError("negative virtual range size")
    matches=[]
    for seg in loads:
        start=seg["vaddr"]; end=start+seg["filesz"]
        if vaddr>=start and vaddr+size<=end: matches.append(seg["offset"]+(vaddr-start))
    if len(matches)!=1: raise ELFAuditError("virtual range is not uniquely file-backed by PT_LOAD")
    return matches[0]
def parse_dynamic(data,phdrs):
    loads=[p for p in phdrs["program_headers"] if p["type"]==PT_LOAD]
    dynamics=[p for p in phdrs["program_headers"] if p["type"]==PT_DYNAMIC]
    if not dynamics: return {"present":False,"entries":[],"dependencies":[],"string_table":None}
    if len(dynamics)!=1: raise ELFAuditError("multiple PT_DYNAMIC segments are unsupported")
    dyn=dynamics[0]
    if dyn["filesz"]==0 or dyn["filesz"]%8: raise ELFAuditError("invalid PT_DYNAMIC file size")
    entries=[]; terminated=False; strings_vaddr=None; strings_size=None
    for off in range(dyn["offset"],dyn["offset"]+dyn["filesz"],8):
        tag,value=struct.unpack_from(phdrs["endian"]+"iI",data,off)
        item={"tag":tag,"tag_name":TAG_NAMES.get(tag,"DT_UNKNOWN_"+str(tag&0xffffffff)),"value":value}
        entries.append(item)
        if tag==DT_STRTAB: strings_vaddr=value
        if tag==DT_STRSZ: strings_size=value
        if tag==DT_NULL: terminated=True; break
    if not terminated: raise ELFAuditError("PT_DYNAMIC lacks DT_NULL terminator")
    strings=None; str_off=None
    if strings_vaddr is not None and strings_size is not None:
        str_off=map_vaddr(strings_vaddr,strings_size,loads)
        strings=data[str_off:str_off+strings_size]
        for item in entries:
            if item["tag"] in STRING_TAGS:
                item["string"]=_cstring(strings,item["value"],item["tag_name"])
    dependencies=[x["string"] for x in entries if x["tag"]==DT_NEEDED]
    return {"present":True,"segment_index":dyn["index"],"file_offset":dyn["offset"],
      "file_size":dyn["filesz"],"entries":entries,"dependencies":dependencies,
      "string_table":{"virtual_address":strings_vaddr,"size":strings_size,"file_offset":str_off} if strings is not None else None}
def audit(data):
    ph=parse_program_headers(data)
    loads=[p for p in ph["program_headers"] if p["type"]==PT_LOAD]
    dynamic=parse_dynamic(data,ph)
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
      "elf_class":32,"machine":"ARM","endianness":ph["endianness"],"entry":f"0x{ph['entry']:08x}",
      "section_headers_present":ph["section_headers_present"],"section_header_count":ph["section_header_count"],
      "program_headers":ph["program_headers"],"load_segment_count":len(loads),"dynamic":dynamic,
      "evidence_limits":["Only ELF32 EM_ARM program headers and PT_DYNAMIC entries are parsed.",
      "Virtual addresses map only through unique PT_LOAD file-backed ranges; BSS-only bytes are not read from file.",
      "DT_NEEDED and related strings are decoded only when DT_STRTAB and DT_STRSZ are both present.",
      "Dynamic symbol and relocation tables are identified by tags but are not decoded in this stage.",
      "No relocation is applied and no library provider is inferred."]}
def main():
    p=argparse.ArgumentParser(description=__doc__); p.add_argument("input",type=Path); p.add_argument("--json",type=Path); a=p.parse_args()
    try:
        report=audit(a.input.read_bytes()); report["source"]=a.input.name; rendered=json.dumps(report,indent=2)
        if a.json: a.json.parent.mkdir(parents=True,exist_ok=True); a.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: p.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
