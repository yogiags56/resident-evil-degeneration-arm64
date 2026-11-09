#!/usr/bin/env python3
"""Read-only ELF32 ARM relocation auditor; never applies relocations or executes input."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import audit_elf, ELFAuditError
SHT_SYMTAB=2
SHT_REL=9
SHT_RELA=4
RELOC_NAMES={0:"R_ARM_NONE",1:"R_ARM_PC24",2:"R_ARM_ABS32",3:"R_ARM_REL32",10:"R_ARM_THM_CALL",21:"R_ARM_GLOB_DAT",22:"R_ARM_JUMP_SLOT",23:"R_ARM_RELATIVE",28:"R_ARM_CALL",29:"R_ARM_JUMP24",30:"R_ARM_THM_JUMP24",38:"R_ARM_TARGET1",40:"R_ARM_V4BX",42:"R_ARM_PREL31",43:"R_ARM_MOVW_ABS_NC",44:"R_ARM_MOVT_ABS",47:"R_ARM_THM_MOVW_ABS_NC",48:"R_ARM_THM_MOVT_ABS",50:"R_ARM_THM_JUMP19",51:"R_ARM_THM_JUMP6",52:"R_ARM_THM_ALU_PREL_11_0",53:"R_ARM_THM_PC12",54:"R_ARM_ABS32_NOI",55:"R_ARM_REL32_NOI"}
def cstring(blob, offset):
    if offset<0 or offset>=len(blob): raise ELFAuditError("symbol name offset out of range")
    end=blob.find(b"\0",offset)
    if end<0: raise ELFAuditError("unterminated symbol name")
    try: return blob[offset:end].decode("utf-8")
    except UnicodeDecodeError as exc: raise ELFAuditError("invalid symbol name encoding") from exc
def load_symbols(data, elf, symtab):
    if symtab["type"]!=SHT_SYMTAB: raise ELFAuditError("relocation link must reference SHT_SYMTAB")
    sections=elf["sections"]; link=symtab["link"]
    if link>=len(sections) or sections[link]["type"]!=3: raise ELFAuditError("symbol string-table link is invalid")
    strings=sections[link]
    if symtab["entsize"]<16 or symtab["size"]%symtab["entsize"]: raise ELFAuditError("invalid ELF32 symbol-table entry size")
    if symtab["offset"]+symtab["size"]>len(data) or strings["offset"]+strings["size"]>len(data): raise ELFAuditError("symbol or string table exceeds file bounds")
    names=data[strings["offset"]:strings["offset"]+strings["size"]]
    endian="<" if elf["endianness"]=="little" else ">"; result=[]
    for n in range(symtab["size"]//symtab["entsize"]):
        off=symtab["offset"]+n*symtab["entsize"]
        name,value,size,info,other,shndx=struct.unpack_from(endian+"IIIBBH",data,off)
        result.append({"name":cstring(names,name) if name else "","value":value,"size":size,"binding":info>>4,"type":info&15,"section_index":shndx})
    return result
def audit_relocations(data):
    elf=audit_elf(data)
    if elf["elf_class"]!=32 or elf["machine"]!=40: raise ELFAuditError("relocation audit currently requires ELF32 EM_ARM")
    endian="<" if elf["endianness"]=="little" else ">"; sections=elf["sections"]; reports=[]; total=0
    for relsec in sections:
        if relsec["type"] not in (SHT_REL,SHT_RELA): continue
        is_rela=relsec["type"]==SHT_RELA; expected=12 if is_rela else 8
        if relsec["entsize"]<expected or relsec["size"]%relsec["entsize"]: raise ELFAuditError(f"invalid relocation entry size in {relsec['name']}")
        if relsec["offset"]+relsec["size"]>len(data): raise ELFAuditError("relocation table exceeds file bounds")
        if relsec["link"]>=len(sections) or relsec["info"]>=len(sections): raise ELFAuditError(f"invalid sh_link/sh_info in {relsec['name']}")
        symtab=sections[relsec["link"]]
        if symtab["type"]!=SHT_SYMTAB: raise ELFAuditError(f"{relsec['name']} does not link to SHT_SYMTAB")
        target=sections[relsec["info"]]; symbols=load_symbols(data,elf,symtab); entries=[]
        for n in range(relsec["size"]//relsec["entsize"]):
            off=relsec["offset"]+n*relsec["entsize"]
            if is_rela: r_offset,r_info,addend=struct.unpack_from(endian+"IIi",data,off)
            else: r_offset,r_info=struct.unpack_from(endian+"II",data,off); addend=None
            symbol_index=r_info>>8; reloc_type=r_info&0xff
            if symbol_index>=len(symbols): raise ELFAuditError(f"relocation symbol index {symbol_index} out of range")
            target_offset=r_offset if elf["type"]==1 else r_offset-int(target["address"],16)
            if target_offset<0 or target_offset>=target["size"]: raise ELFAuditError(f"relocation offset 0x{r_offset:x} outside target section {target['name']}")
            symbol=symbols[symbol_index]
            entries.append({"offset":f"0x{r_offset:x}","target_section_offset":target_offset,"symbol_index":symbol_index,"symbol":symbol["name"],"symbol_value":f"0x{symbol['value']:x}","type":reloc_type,"type_name":RELOC_NAMES.get(reloc_type,f"R_ARM_UNKNOWN_{reloc_type}"),"addend":addend,"symbol_section_index":symbol["section_index"]})
        reports.append({"name":relsec["name"],"type":"RELA" if is_rela else "REL","symbol_table_index":symtab["index"],"target_section":target["name"],"target_section_index":target["index"],"symbol_table":symtab["name"],"entry_count":len(entries),"entries":entries}); total+=len(entries)
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),"elf_class":elf["elf_class"],"machine":elf["machine_name"],"endianness":elf["endianness"],"relocation_sections":len(reports),"relocation_count":total,"sections":reports,"evidence_limits":["Relocations are reported, not applied or resolved into final addresses.","REL addends are encoded in target bytes and are not decoded by this report.","Unknown relocation types are preserved numerically without guessed semantics.","Only ELF32 EM_ARM SHT_REL and SHT_RELA with conventional symbol tables are supported."]}
def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument("input",type=Path); parser.add_argument("--json",type=Path); args=parser.parse_args()
    try:
        report=audit_relocations(args.input.read_bytes()); report["source"]=args.input.name; rendered=json.dumps(report,indent=2)
        if args.json: args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError) as exc: parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
