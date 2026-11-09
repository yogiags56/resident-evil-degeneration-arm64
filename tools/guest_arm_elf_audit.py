#!/usr/bin/env python3
"""Read-only ELF/ARM ABI metadata auditor; never loads or executes the input."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from typing import Any

class ELFAuditError(ValueError):
    pass

MACHINE_NAMES = {40: "ARM", 183: "AArch64"}
CPU_ARCH = {0:"pre-v4",1:"ARMv4",2:"ARMv4T",3:"ARMv5T",4:"ARMv5TE",5:"ARMv5TEJ",6:"ARMv6",7:"ARMv6KZ",8:"ARMv6T2",9:"ARMv6K",10:"ARMv7",11:"ARMv6-M",12:"ARMv6S-M",13:"ARMv7E-M",14:"ARMv8-A",15:"ARMv8-R",16:"ARMv8-M.baseline",17:"ARMv8-M.mainline",18:"ARMv8.1-A",19:"ARMv8.2-A",20:"ARMv8.3-A",21:"ARMv8.1-M.mainline",22:"ARMv9-A"}
PROFILE_NAMES = {65:"Application",82:"Realtime",77:"Microcontroller",83:"Application/Realtime"}
THUMB_USE = {0:"not permitted",1:"Thumb-1 permitted",2:"Thumb-2 permitted"}
FP_ARCH = {0:"none",1:"VFPv1",2:"VFPv2",3:"VFPv3",4:"VFPv3-D16",5:"VFPv4",6:"ARMv8-A FP"}
SIMD_ARCH = {0:"none",1:"NEONv1",2:"ARMv8-A SIMD"}
STRING_TAGS = {4:"CPU_raw_name",5:"CPU_name",65:"also_compatible_with"}

def unpack(fmt, data, off, label):
    size=struct.calcsize(fmt)
    if off<0 or off+size>len(data): raise ELFAuditError(f"{label} out of bounds")
    return struct.unpack_from(fmt,data,off)

def cstring(data, off, end, label):
    if off<0 or off>=end or end>len(data): raise ELFAuditError(f"{label} string out of bounds")
    stop=data.find(b"\0",off,end)
    if stop<0: raise ELFAuditError(f"unterminated {label} string")
    try: value=data[off:stop].decode("ascii")
    except UnicodeDecodeError as exc: raise ELFAuditError(f"non-ASCII {label} string") from exc
    return value,stop+1

def uleb(data,off,end):
    value=shift=0
    for _ in range(5):
        if off>=end: raise ELFAuditError("truncated ULEB128")
        byte=data[off]; off+=1; value|=(byte&127)<<shift
        if not byte&128: return value,off
        shift+=7
    raise ELFAuditError("ULEB128 exceeds 32-bit encoding limit")

def parse_arm_attributes(blob,endian):
    if not blob: return {"present":False,"vendors":[],"file_attributes":{}}
    if blob[:1]!=b"A": raise ELFAuditError("unsupported .ARM.attributes format byte")
    pos=1; attrs={}; vendors=[]
    while pos<len(blob):
        start=pos
        length,=unpack(endian+"I",blob,pos,"vendor subsection length")
        if length<5 or start+length>len(blob): raise ELFAuditError("invalid ARM vendor subsection length")
        end=start+length
        vendor,pos=cstring(blob,pos+4,end,"vendor"); vendors.append(vendor)
        while pos<end:
            tag,pos=uleb(blob,pos,end)
            if tag!=1: raise ELFAuditError(f"unsupported ARM attribute subsection tag {tag}")
            flen,=unpack(endian+"I",blob,pos,"file attribute length")
            fend=pos+flen
            if flen<5 or fend>end: raise ELFAuditError("invalid ARM file attribute length")
            pos+=4
            while pos<fend:
                atag,pos=uleb(blob,pos,fend)
                if atag in STRING_TAGS: value,pos=cstring(blob,pos,fend,STRING_TAGS[atag])
                else: value,pos=uleb(blob,pos,fend)
                attrs[STRING_TAGS.get(atag,f"tag_{atag}")]=value
            if pos!=fend: raise ELFAuditError("ARM file attribute boundary mismatch")
        if pos!=end: raise ELFAuditError("ARM vendor subsection boundary mismatch")
    result={"present":True,"vendors":vendors,"file_attributes":attrs}
    if vendors==["aeabi"]:
        decoded={}
        maps={"tag_6":("CPU_arch",CPU_ARCH),"tag_7":("CPU_arch_profile",PROFILE_NAMES),"tag_9":("THUMB_ISA_use",THUMB_USE),"tag_10":("FP_arch",FP_ARCH),"tag_12":("Advanced_SIMD_arch",SIMD_ARCH)}
        for key,(name,mapping) in maps.items():
            if key in attrs:
                value=attrs[key]; decoded[name]={"value":value,"name":mapping.get(value,"unknown")}
        for key,name in (("tag_8","ARM_ISA_use"),("tag_28","ABI_VFP_args")):
            if key in attrs: decoded[name]=attrs[key]
        result["decoded"]=decoded
    return result

def audit_elf(data):
    if len(data)<16 or data[:4]!=b"\x7fELF": raise ELFAuditError("input is not an ELF file")
    cls,enc,ident_version=data[4],data[5],data[6]
    if cls not in (1,2) or enc not in (1,2) or ident_version!=1: raise ELFAuditError("unsupported ELF class, byte order, or ident version")
    endian="<" if enc==1 else ">"
    fmt=endian+("HHIIIIIHHHHHH" if cls==1 else "HHIQQQIHHHHHH")
    fields=unpack(fmt,data,16,"ELF header")
    etype,machine,version,entry,phoff,shoff,flags,ehsize,phentsize,phnum,shentsize,shnum,shstrndx=fields
    expected_eh,expected_ph,expected_sh=(52,32,40) if cls==1 else (64,56,64)
    if ehsize<expected_eh or ehsize>len(data): raise ELFAuditError("invalid ELF header size")
    if phnum and (phentsize<expected_ph or phoff+phentsize*phnum>len(data)): raise ELFAuditError("program header table out of bounds")
    if shnum==0: raise ELFAuditError("extended/empty section table is not supported")
    if shentsize<expected_sh or shoff+shentsize*shnum>len(data): raise ELFAuditError("section header table out of bounds")
    if shstrndx>=shnum: raise ELFAuditError("section-name string table index out of range")
    sections=[]
    for i in range(shnum):
        off=shoff+i*shentsize
        sh=unpack(endian+("IIIIIIIIII" if cls==1 else "IIQQQQIIQQ"),data,off,"section header")
        name,stype,sflags,addr,foff,size,link,info,align,entsize=sh
        if stype!=8 and foff+size>len(data): raise ELFAuditError(f"section {i} data out of bounds")
        sections.append({"name_offset":name,"type":stype,"flags":sflags,"offset":foff,"size":size})
    ns=sections[shstrndx]; names=data[ns["offset"]:ns["offset"]+ns["size"]]
    for s in sections: s["name"],_=cstring(names,s["name_offset"],len(names),"section name")
    arm=next((s for s in sections if s["name"]==".ARM.attributes"),None)
    attrs=parse_arm_attributes(data[arm["offset"]:arm["offset"]+arm["size"]],endian) if arm else {"present":False,"vendors":[],"file_attributes":{}}
    return {"file_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),"elf_class":32 if cls==1 else 64,"endianness":"little" if enc==1 else "big","type":etype,"machine":machine,"machine_name":MACHINE_NAMES.get(machine,"unknown"),"entry":f"0x{entry:x}","flags":f"0x{flags:x}","program_header_count":phnum,"section_count":shnum,"sections":[{"name":s["name"],"type":s["type"],"size":s["size"]} for s in sections],"arm_attributes":attrs,"evidence_limits":["ELF metadata describes the supplied file, not necessarily runtime CPU reset state.","Missing .ARM.attributes means attributes are unknown, not that extensions are absent.","This audit does not disassemble code or infer instruction usage."]}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input",type=Path,help="locally supplied ELF executable/shared object")
    parser.add_argument("--json",type=Path,help="optional JSON report destination")
    args=parser.parse_args()
    try:
        data=args.input.read_bytes(); report=audit_elf(data); report["source"]=args.input.name
        rendered=json.dumps(report,indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
