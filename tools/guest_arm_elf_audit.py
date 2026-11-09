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
STRING_TAGS = {4:"CPU_raw_name",5:"CPU_name",32:"compatibility",65:"also_compatible_with",67:"conformance"}
ATTRIBUTE_NAMES = {
  6:"CPU_arch",7:"CPU_arch_profile",8:"ARM_ISA_use",9:"THUMB_ISA_use",10:"FP_arch",
  11:"WMMX_arch",12:"Advanced_SIMD_arch",13:"PCS_config",14:"ABI_PCS_R9_use",
  15:"ABI_PCS_RW_data",16:"ABI_PCS_RO_data",17:"ABI_PCS_GOT_use",18:"ABI_PCS_wchar_t",
  19:"ABI_FP_rounding",20:"ABI_FP_denormal",21:"ABI_FP_exceptions",22:"ABI_FP_user_exceptions",
  23:"ABI_FP_number_model",24:"ABI_align_needed",25:"ABI_align_preserved",26:"ABI_enum_size",
  27:"ABI_HardFP_use",28:"ABI_VFP_args",29:"ABI_WMMX_args",30:"ABI_optimization_goals",
  31:"ABI_FP_optimization_goals",34:"CPU_unaligned_access",36:"FP_HP_extension",
  38:"ABI_FP_16bit_format",42:"MPextension_use",44:"DIV_use",46:"DSP_extension",
  48:"MVE_arch",50:"PAC_extension",52:"BTI_extension",64:"nodefaults",
  66:"T2EE_use",68:"Virtualization_use"
}
ATTRIBUTE_NAMES.update({tag:name for tag,name in STRING_TAGS.items()})
def attribute_value_kind(tag):
    normalized = tag % 128 if tag >= 128 else tag
    if normalized in (32,):
        return "compatibility"
    if normalized == 64:
        return "none"
    if normalized in STRING_TAGS:
        return "string"
    if 33 <= normalized <= 127:
        return "string" if normalized & 1 else "integer"
    if normalized in ATTRIBUTE_NAMES or normalized in (1,2,3):
        return "integer"
    raise ELFAuditError(f"unknown mandatory AEABI attribute tag {tag}")

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

def parse_public_attributes(blob, endian):
    pos=0
    scopes=[]
    while pos<len(blob):
        scope_start=pos
        tag,pos=uleb(blob,pos,len(blob))
        size,=unpack(endian+"I",blob,pos,"ARM attribute scope size")
        scope_end=scope_start+size
        if size<5 or scope_end>len(blob):
            raise ELFAuditError("invalid ARM attribute scope size")
        pos+=4
        indexes=[]
        if tag in (2,3):
            while True:
                index,pos=uleb(blob,pos,scope_end)
                if index==0: break
                indexes.append(index)
        elif tag!=1:
            raise ELFAuditError(f"unknown AEABI attribute scope tag {tag}")
        values={}
        while pos<scope_end:
            attr_tag,pos=uleb(blob,pos,scope_end)
            if attr_tag==0:
                raise ELFAuditError("zero AEABI attribute tag")
            kind=attribute_value_kind(attr_tag)
            if kind=="string":
                value,pos=cstring(blob,pos,scope_end,ATTRIBUTE_NAMES.get(attr_tag,f"tag_{attr_tag}"))
            elif kind=="compatibility":
                flag,pos=uleb(blob,pos,scope_end)
                vendor_name,pos=cstring(blob,pos,scope_end,"Tag_compatibility vendor")
                value={"flag":flag,"vendor":vendor_name}
            elif kind=="none":
                value=True
            else:
                value,pos=uleb(blob,pos,scope_end)
            key=ATTRIBUTE_NAMES.get(attr_tag,f"tag_{attr_tag}")
            if key in values and values[key]!=value:
                raise ELFAuditError(f"conflicting values for AEABI attribute {key}")
            values[key]=value
        if pos!=scope_end:
            raise ELFAuditError("AEABI scope boundary mismatch")
        scopes.append({"scope":{1:"file",2:"section",3:"symbol"}[tag],
                       "indexes":indexes,"attributes":values})
    return scopes

def parse_arm_attributes(blob,endian):
    if not blob: return {"present":False,"vendors":[],"file_attributes":{},"scopes":[]}
    if blob[:1]!=b"A": raise ELFAuditError("unsupported .ARM.attributes format byte")
    pos=1; vendors=[]; all_scopes=[]; private=[]
    while pos<len(blob):
        start=pos
        length,=unpack(endian+"I",blob,pos,"vendor subsection length")
        if length<5 or start+length>len(blob): raise ELFAuditError("invalid ARM vendor subsection length")
        end=start+length
        vendor,pos=cstring(blob,pos+4,end,"vendor")
        vendor_data=blob[pos:end]
        vendors.append(vendor)
        if vendor=="aeabi":
            scopes=parse_public_attributes(vendor_data,endian)
            for scope in scopes:
                scope["vendor"]=vendor
            all_scopes.extend(scopes)
        else:
            # Private vendor payload is deliberately opaque; its schema is vendor-defined.
            private.append({"vendor":vendor,"data_size":len(vendor_data),"data_hex":vendor_data.hex()})
        pos=end
    file_attrs={}
    for scope in all_scopes:
        if scope["scope"]=="file":
            for key,value in scope["attributes"].items():
                if key in file_attrs and file_attrs[key]!=value:
                    raise ELFAuditError(f"conflicting file-scope AEABI attribute {key}")
                file_attrs[key]=value
    result={"present":True,"vendors":vendors,"file_attributes":file_attrs,
            "scopes":all_scopes,"private_vendor_payloads":private}
    decoded={}
    maps={"CPU_arch":CPU_ARCH,"CPU_arch_profile":PROFILE_NAMES,
          "THUMB_ISA_use":THUMB_USE,"FP_arch":FP_ARCH,"Advanced_SIMD_arch":SIMD_ARCH}
    for key,mapping in maps.items():
        if key in file_attrs:
            value=file_attrs[key]; decoded[key]={"value":value,"name":mapping.get(value,"unknown")}
    for key in ("ARM_ISA_use","ABI_VFP_args","CPU_unaligned_access","MVE_arch",
                "DIV_use","DSP_extension","Virtualization_use"):
        if key in file_attrs: decoded[key]=file_attrs[key]
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
    return {"file_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),"elf_class":32 if cls==1 else 64,"endianness":"little" if enc==1 else "big","type":etype,"machine":machine,"machine_name":MACHINE_NAMES.get(machine,"unknown"),"entry":f"0x{entry:x}","flags":f"0x{flags:x}","program_header_count":phnum,"section_count":shnum,"sections":[{"name":s["name"],"type":s["type"],"flags":s["flags"],"address":f"0x{next((unpack(endian+(\"I\" if cls==1 else \"Q\"),data,shoff+i*shentsize+(12 if cls==1 else 16),\"section address\")[0] for i,x in enumerate(sections) if x is s),0):x}","offset":s["offset"],"size":s["size"]} for s in sections],"arm_attributes":attrs,"evidence_limits":["ELF metadata describes the supplied file, not necessarily runtime CPU reset state.","Missing .ARM.attributes means attributes are unknown, not that extensions are absent.","This audit does not disassemble code or infer instruction usage."]}

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
