#!/usr/bin/env python3
"""Map and inventory ELF32 ARM dynamic REL/RELA tables from PT_DYNAMIC."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
from guest_arm_relocation_abi import SPECS
from guest_arm_pt_dynamic_audit import parse_program_headers, map_vaddr, PT_LOAD, PT_DYNAMIC
DT_NULL=0; DT_HASH=4; DT_STRTAB=5; DT_SYMTAB=6; DT_RELA=7; DT_RELASZ=8
DT_RELAENT=9; DT_STRSZ=10; DT_SYMENT=11; DT_REL=17; DT_RELSZ=18
DT_RELENT=19; DT_PLTREL=20; DT_JMPREL=23; DT_PLTRELSZ=2
DT_GNU_HASH=0x6ffffef5
DT_VERSYM=0x6ffffff0; DT_VERDEF=0x6ffffffc; DT_VERDEFNUM=0x6ffffffd
DT_VERNEED=0x6ffffffe; DT_VERNEEDNUM=0x6fffffff
TAGS={0:"DT_NULL",2:"DT_PLTRELSZ",4:"DT_HASH",5:"DT_STRTAB",6:"DT_SYMTAB",
7:"DT_RELA",8:"DT_RELASZ",9:"DT_RELAENT",10:"DT_STRSZ",11:"DT_SYMENT",
17:"DT_REL",18:"DT_RELSZ",19:"DT_RELENT",20:"DT_PLTREL",23:"DT_JMPREL",35:"DT_RELRSZ",36:"DT_RELR",37:"DT_RELRENT",DT_GNU_HASH:"DT_GNU_HASH",DT_VERSYM:"DT_VERSYM",DT_VERDEF:"DT_VERDEF",DT_VERDEFNUM:"DT_VERDEFNUM",DT_VERNEED:"DT_VERNEED",DT_VERNEEDNUM:"DT_VERNEEDNUM"}
RELOCS={0:"R_ARM_NONE",1:"R_ARM_PC24",2:"R_ARM_ABS32",3:"R_ARM_REL32",
10:"R_ARM_THM_CALL",21:"R_ARM_GLOB_DAT",22:"R_ARM_JUMP_SLOT",23:"R_ARM_RELATIVE",
28:"R_ARM_CALL",29:"R_ARM_JUMP24",30:"R_ARM_THM_JUMP24",38:"R_ARM_TARGET1",
42:"R_ARM_PREL31",43:"R_ARM_MOVW_ABS_NC",44:"R_ARM_MOVT_ABS",
47:"R_ARM_THM_MOVW_ABS_NC",48:"R_ARM_THM_MOVT_ABS"}
RELOCS.update({code:spec["name"] for code,spec in SPECS.items()})

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
MAX_HASH_DIMENSION=1_000_000

def _gnu_hash(data, address, loads, endian):
    _, header = _mapped(data, address, 16, loads, "DT_GNU_HASH header")
    nbucket, symoffset, bloom_size, bloom_shift = struct.unpack(endian+"IIII", header)
    if not nbucket or not bloom_size:
        raise ELFAuditError("DT_GNU_HASH bucket and bloom dimensions must be nonzero")
    if nbucket > MAX_HASH_DIMENSION or bloom_size > MAX_HASH_DIMENSION:
        raise ELFAuditError("DT_GNU_HASH dimensions exceed safety limit")
    if bloom_size & (bloom_size-1):
        raise ELFAuditError("DT_GNU_HASH bloom_size must be a power of two")
    if symoffset > MAX_HASH_DIMENSION:
        raise ELFAuditError("DT_GNU_HASH symoffset exceeds safety limit")
    if bloom_shift >= 32:
        raise ELFAuditError("DT_GNU_HASH bloom_shift exceeds ELF32 word width")
    bloom_addr=address+16
    bloom_size_bytes=bloom_size*4
    _, bloom_raw=_mapped(data,bloom_addr,bloom_size_bytes,loads,"DT_GNU_HASH bloom filter")
    bloom=struct.unpack(endian+("I"*bloom_size),bloom_raw)
    bucket_addr=bloom_addr+bloom_size_bytes
    _, bucket_raw=_mapped(data,bucket_addr,nbucket*4,loads,"DT_GNU_HASH buckets")
    buckets=struct.unpack(endian+("I"*nbucket),bucket_raw)
    chain_addr=bucket_addr+nbucket*4
    # Find the unique PT_LOAD file-backed segment containing the first chain word.
    candidates=[]
    for seg in loads:
        if chain_addr>=seg["vaddr"] and chain_addr<seg["vaddr"]+seg["filesz"]:
            candidates.append(seg)
    if len(candidates)!=1:
        raise ELFAuditError("DT_GNU_HASH chain does not map to a unique PT_LOAD file range")
    available=candidates[0]["vaddr"]+candidates[0]["filesz"]-chain_addr
    chain_words=min(available//4,MAX_HASH_DIMENSION)
    if chain_words<=0:
        raise ELFAuditError("DT_GNU_HASH chain has no file-backed words")
    _, chain_raw=_mapped(data,chain_addr,chain_words*4,loads,"DT_GNU_HASH chain bounds")
    chains=struct.unpack(endian+("I"*chain_words),chain_raw)
    max_symbol=symoffset-1
    chain_records={}
    for bucket in buckets:
        if bucket==0:
            continue
        if bucket<symoffset:
            raise ELFAuditError("DT_GNU_HASH bucket precedes symoffset")
        index=bucket-symoffset
        if index>=chain_words:
            raise ELFAuditError("DT_GNU_HASH bucket points outside bounded chain")
        symbol=bucket
        while True:
            chain_index=symbol-symoffset
            if chain_index>=chain_words:
                raise ELFAuditError("DT_GNU_HASH chain has no terminator within PT_LOAD bounds")
            value=chains[chain_index]
            chain_records[symbol]=value
            max_symbol=max(max_symbol,symbol)
            if value&1:
                break
            symbol+=1
            if symbol-symoffset>=MAX_HASH_DIMENSION:
                raise ELFAuditError("DT_GNU_HASH chain exceeds safety limit")
    symbol_count=max(symoffset,max_symbol+1)
    return {
        "symbol_count":symbol_count,
        "bucket_count":nbucket,
        "symoffset":symoffset,
        "bloom_size":bloom_size,
        "bloom_shift":bloom_shift,
        "bloom_words":list(bloom),
        "buckets":list(buckets),
        "chain_records":chain_records,
    }

def _gnu_name_hash(name):
    value=5381
    for byte in name.encode("utf-8"):
        value=((value<<5)+value+byte)&0xffffffff
    return value

def _parse_symbols(data,tags,loads,endian,max_index):
    required=(DT_SYMTAB,DT_SYMENT,DT_STRTAB,DT_STRSZ)
    if any(x not in tags for x in required):
        return None,{"available":False,"reason":"DT_SYMTAB/DT_SYMENT/DT_STRTAB/DT_STRSZ metadata incomplete"}
    if tags[DT_SYMENT]!=16:
        raise ELFAuditError("DT_SYMENT must be 16 for ELF32")
    sysv_count=None
    if DT_HASH in tags:
        _,hashhead=_mapped(data,tags[DT_HASH],8,loads,"DT_HASH header")
        nbucket,nchain=struct.unpack(endian+"II",hashhead)
        if nchain>MAX_HASH_DIMENSION or nbucket>MAX_HASH_DIMENSION:
            raise ELFAuditError("DT_HASH dimensions exceed safety limit")
        hash_size=8+4*(nbucket+nchain)
        _mapped(data,tags[DT_HASH],hash_size,loads,"DT_HASH table")
        sysv_count=nchain
    gnu_info=None
    if DT_GNU_HASH in tags:
        gnu_info=_gnu_hash(data,tags[DT_GNU_HASH],loads,endian)
    if sysv_count is None and gnu_info is None:
        return None,{"available":False,"reason":"neither bounded DT_HASH nor DT_GNU_HASH is available"}
    if sysv_count is not None and gnu_info is not None:
        if gnu_info["symbol_count"]>sysv_count:
            raise ELFAuditError("DT_GNU_HASH symbol bound exceeds SysV DT_HASH nchain")
        symbol_count=sysv_count
        comparison="gnu_bound_within_sysv_nchain" if gnu_info["symbol_count"]==sysv_count else "gnu_bound_lower_than_sysv_nchain"
    elif sysv_count is not None:
        symbol_count=sysv_count
        comparison="sysv_hash_only"
    else:
        symbol_count=gnu_info["symbol_count"]
        comparison="gnu_hash_only"
    if max_index>=symbol_count:
        raise ELFAuditError(f"relocation symbol index {max_index} exceeds bounded dynamic symbol count {symbol_count}")
    sym_size=symbol_count*16
    _,symraw=_mapped(data,tags[DT_SYMTAB],sym_size,loads,"DT_SYMTAB")
    _,strraw=_mapped(data,tags[DT_STRTAB],tags[DT_STRSZ],loads,"DT_STRTAB")
    symbols=[]
    for index in range(symbol_count):
        name,value,size,info,other,shndx=struct.unpack_from(endian+"IIIBBH",symraw,index*16)
        if name>=len(strraw):
            raise ELFAuditError("dynamic symbol name offset exceeds DT_STRSZ")
        if name:
            end=strraw.find(b"\0",name)
            if end<0:
                raise ELFAuditError("unterminated dynamic symbol name")
            try: symbol_name=strraw[name:end].decode("utf-8")
            except UnicodeDecodeError as exc: raise ELFAuditError("invalid dynamic symbol name encoding") from exc
        else:
            symbol_name=""
        symbols.append({"index":index,"name":symbol_name,"value":f"0x{value:x}",
          "size":size,"binding":info>>4,"type":info&15,"visibility":other&3,
          "section_index":shndx,"undefined":shndx==0})
    if gnu_info is not None:
        bloom=list(gnu_info["bloom_words"])
        for index,chain_hash in gnu_info["chain_records"].items():
            if index>=len(symbols):
                raise ELFAuditError("DT_GNU_HASH chain references a symbol outside bounded DT_SYMTAB")
            name=symbols[index]["name"]
            h=_gnu_name_hash(name)
            if (chain_hash & 0xfffffffe)!=(h & 0xfffffffe):
                raise ELFAuditError(f"DT_GNU_HASH chain hash disagrees with symbol {index}")
            word_index=(h//32)&(gnu_info["bloom_size"]-1)
            bit1=h%32
            bit2=(h>>gnu_info["bloom_shift"])%32
            mask=(1<<bit1)|(1<<bit2)
            if bloom[word_index]&mask!=mask:
                raise ELFAuditError(f"DT_GNU_HASH bloom filter omits symbol {index}")
    info={"available":True,"symbol_count":symbol_count,"hash_source":comparison}
    if sysv_count is not None:
        info["sysv_symbol_count"]=sysv_count
    if gnu_info is not None:
        info["gnu_hash"]={k:v for k,v in gnu_info.items() if k!="chain_records"}
    return symbols,info

MAX_VERSION_RECORDS=65535
def _version_string(strraw,offset,label):
    if offset>=len(strraw):
        raise ELFAuditError(f"{label}: string offset exceeds DT_STRSZ")
    end=strraw.find(b"\0",offset)
    if end<0:
        raise ELFAuditError(f"{label}: unterminated version string")
    try:
        return strraw[offset:end].decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ELFAuditError(f"{label}: invalid version string encoding") from exc

def _version_next(address,delta,minimum,label):
    if delta<minimum or delta%4:
        raise ELFAuditError(f"{label}: invalid next/auxiliary offset")
    result=address+delta
    if result>0xffffffff:
        raise ELFAuditError(f"{label}: virtual address overflow")
    return result

def _parse_symbol_versions(data,tags,loads,endian,symbols,symbol_info):
    pairs=((DT_VERDEF,DT_VERDEFNUM,"DT_VERDEF"),(DT_VERNEED,DT_VERNEEDNUM,"DT_VERNEED"))
    for address_tag,count_tag,label in pairs:
        if (address_tag in tags)!=(count_tag in tags):
            raise ELFAuditError(f"{label} and its count tag must appear together")
        if count_tag in tags and (tags[count_tag]==0 or tags[count_tag]>MAX_VERSION_RECORDS):
            raise ELFAuditError(f"{label} count is empty or exceeds safety limit")
    version_records={}
    definitions=[]
    if DT_VERDEF in tags:
        address=tags[DT_VERDEF]
        for record_index in range(tags[DT_VERDEFNUM]):
            _,raw=_mapped(data,address,20,loads,"Elf32_Verdef")
            version,flags,index,count,name_hash,aux_delta,next_delta=struct.unpack(endian+"HHHHIII",raw)
            if version!=1 or count==0 or count>MAX_VERSION_RECORDS:
                raise ELFAuditError("invalid Elf32_Verdef version or auxiliary count")
            if index<2 or index&0x8000:
                raise ELFAuditError("invalid reserved Elf32_Verdef version index")
            aux_address=_version_next(address,aux_delta,20,"Elf32_Verdef auxiliary")
            names=[]
            for aux_index in range(count):
                _,aux=_mapped(data,aux_address,8,loads,"Elf32_Verdaux")
                name_offset,aux_next=struct.unpack(endian+"II",aux)
                if DT_STRTAB not in tags or DT_STRSZ not in tags:
                    raise ELFAuditError("version records require DT_STRTAB and DT_STRSZ")
                _,strraw=_mapped(data,tags[DT_STRTAB],tags[DT_STRSZ],loads,"DT_STRTAB")
                names.append(_version_string(strraw,name_offset,"Elf32_Verdaux"))
                if aux_index+1<count:
                    aux_address=_version_next(aux_address,aux_next,8,"Elf32_Verdaux")
                elif aux_next!=0:
                    raise ELFAuditError("final Elf32_Verdaux has nonzero next offset")
            if index in version_records:
                raise ELFAuditError("duplicate ELF symbol version index")
            record={"index":index,"kind":"definition","name":names[0],"parent_names":names[1:],
              "flags":flags,"hash":f"0x{name_hash:08x}"}
            version_records[index]=record; definitions.append(record)
            if record_index+1<tags[DT_VERDEFNUM]:
                address=_version_next(address,next_delta,20,"Elf32_Verdef")
            elif next_delta!=0:
                raise ELFAuditError("final Elf32_Verdef has nonzero next offset")
    requirements=[]
    if DT_VERNEED in tags:
        address=tags[DT_VERNEED]
        for record_index in range(tags[DT_VERNEEDNUM]):
            _,raw=_mapped(data,address,16,loads,"Elf32_Verneed")
            version,count,file_offset,aux_delta,next_delta=struct.unpack(endian+"HHIII",raw)
            if version!=1 or count==0 or count>MAX_VERSION_RECORDS:
                raise ELFAuditError("invalid Elf32_Verneed version or auxiliary count")
            if DT_STRTAB not in tags or DT_STRSZ not in tags:
                raise ELFAuditError("version records require DT_STRTAB and DT_STRSZ")
            _,strraw=_mapped(data,tags[DT_STRTAB],tags[DT_STRSZ],loads,"DT_STRTAB")
            library=_version_string(strraw,file_offset,"Elf32_Verneed file")
            aux_address=_version_next(address,aux_delta,16,"Elf32_Verneed auxiliary")
            auxiliaries=[]
            for aux_index in range(count):
                _,aux=_mapped(data,aux_address,16,loads,"Elf32_Vernaux")
                name_hash,flags,other,name_offset,aux_next=struct.unpack(endian+"IHHII",aux)
                index=other&0x7fff
                if index<2:
                    raise ELFAuditError("invalid reserved Elf32_Vernaux version index")
                name=_version_string(strraw,name_offset,"Elf32_Vernaux")
                if index in version_records:
                    raise ELFAuditError("duplicate ELF symbol version index")
                record={"index":index,"kind":"requirement","name":name,"library":library,
                  "flags":flags,"hash":f"0x{name_hash:08x}"}
                version_records[index]=record; auxiliaries.append(record)
                if aux_index+1<count:
                    aux_address=_version_next(aux_address,aux_next,16,"Elf32_Vernaux")
                elif aux_next!=0:
                    raise ELFAuditError("final Elf32_Vernaux has nonzero next offset")
            requirements.append({"library":library,"versions":auxiliaries})
            if record_index+1<tags[DT_VERNEEDNUM]:
                address=_version_next(address,next_delta,16,"Elf32_Verneed")
            elif next_delta!=0:
                raise ELFAuditError("final Elf32_Verneed has nonzero next offset")
    if DT_VERSYM in tags:
        if symbols is None or not symbol_info.get("available"):
            raise ELFAuditError("DT_VERSYM cannot be bounded without a resolved dynamic symbol count")
        _,raw=_mapped(data,tags[DT_VERSYM],len(symbols)*2,loads,"DT_VERSYM")
        for index,symbol in enumerate(symbols):
            raw_value=struct.unpack_from(endian+"H",raw,index*2)[0]
            version_index=raw_value&0x7fff
            hidden=bool(raw_value&0x8000)
            if version_index==0:
                version={"index":0,"kind":"local","name":None}
            elif version_index==1:
                version={"index":1,"kind":"global","name":None}
            else:
                if version_index not in version_records:
                    raise ELFAuditError(f"DT_VERSYM index {version_index} has no DT_VERDEF/DT_VERNEED record")
                version=dict(version_records[version_index])
            version["hidden"]=hidden
            symbol["version"]=version
    elif symbols is not None:
        for symbol in symbols:
            symbol["version"]=None
    return {"available":bool(DT_VERSYM in tags or definitions or requirements),
      "versym_entries":len(symbols) if DT_VERSYM in tags and symbols is not None else 0,
      "definitions":definitions,"requirements":requirements,
      "version_index_count":len(version_records)}

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
            symbol={"index":sym_index,"name":sym["name"],"undefined":sym["undefined"],"version":sym.get("version")}
        else: symbol={"index":sym_index,"name":None,"undefined":None}
        entries.append({"index":index,"target_address":f"0x{target:08x}",
          "symbol_index":sym_index,"symbol":symbol["name"],"symbol_undefined":symbol["undefined"],"symbol_version":symbol.get("version"),
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
    symbols,symbol_info=_parse_symbols(data,tags,loads,endian,max_index) if raw_specs or DT_VERSYM in tags else (None,{"available":False,"reason":"no dynamic relocation tables or version symbol table"})
    version_info=_parse_symbol_versions(data,tags,loads,endian,symbols,symbol_info)
    tables=[]
    for label,address,size,ent,kind,_ in raw_specs:
        tables.append({"source_tag":label,**_reloc_table(data,address,size,ent,kind,loads,endian,label,symbols)})
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
      "elf_class":32,"machine":"ARM","endianness":ph["endianness"],
      "section_headers_present":ph["section_headers_present"],"dynamic_entries":entries,
      "dynamic_tags":{TAGS.get(k,f"DT_UNKNOWN_{k}"):v for k,v in tags.items()},
      "symbol_resolution":symbol_info,"symbol_versioning":version_info,"relocation_tables":tables,
      "relocation_count":sum(x["entry_count"] for x in tables),
      "evidence_limits":["Only ELF32 EM_ARM dynamic REL/RELA tables described by PT_DYNAMIC are supported.",
      "Symbol names require bounded SysV DT_HASH or DT_GNU_HASH metadata and safe DT_SYMTAB/DT_STRTAB ranges.",
      "Symbol versions are decoded only from bounded DT_VERSYM/DT_VERDEF/DT_VERNEED metadata; no provider compatibility is inferred.",
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
