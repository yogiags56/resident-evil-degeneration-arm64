#!/usr/bin/env python3
"""Inventory ARM/Thumb instructions using ELF mapping symbols and Capstone."""
from __future__ import annotations
import argparse, collections, hashlib, importlib, json, re, struct
from pathlib import Path
from guest_arm_elf_audit import audit_elf, ELFAuditError
SHF_EXECINSTR=0x4
SHT_SYMTAB=2
SHT_DYNSYM=11
MAP_RE=re.compile(r"^\$([atd])(?:\..*)?$")
def read_cstring(blob, offset):
    if offset<0 or offset>=len(blob): raise ELFAuditError("symbol name offset is out of range")
    end=blob.find(b"\0",offset)
    if end<0: raise ELFAuditError("unterminated symbol name")
    try: return blob[offset:end].decode("utf-8")
    except UnicodeDecodeError as exc: raise ELFAuditError("invalid symbol name encoding") from exc
def parse_mapping_symbols(data, elf):
    if elf["elf_class"]!=32: raise ELFAuditError("mapping-symbol parser currently supports ELF32 only")
    endian="<" if elf["endianness"]=="little" else ">"
    sections=elf["sections"]; result=collections.defaultdict(list)
    for symtab in sections:
        if symtab["type"] not in (SHT_SYMTAB,SHT_DYNSYM): continue
        link=symtab["link"]
        if link<0 or link>=len(sections): raise ELFAuditError("symbol table string-table link is out of range")
        strings=sections[link]
        if strings["type"]!=3: raise ELFAuditError("symbol table link does not reference a string table")
        entsize=symtab["entsize"]
        if entsize<16 or entsize==0 or symtab["size"]%entsize: raise ELFAuditError("invalid ELF32 symbol entry size")
        if symtab["offset"]+symtab["size"]>len(data) or strings["offset"]+strings["size"]>len(data):
            raise ELFAuditError("symbol or string table exceeds file bounds")
        strtab=data[strings["offset"]:strings["offset"]+strings["size"]]
        for n in range(symtab["size"]//entsize):
            off=symtab["offset"]+n*entsize
            name_off,value,size,info,other,shndx=struct.unpack_from(endian+"IIIBBH",data,off)
            if not name_off: continue
            name=read_cstring(strtab,name_off); match=MAP_RE.fullmatch(name)
            if not match or (info>>4)!=0: continue
            if shndx==0 or shndx>=len(sections): raise ELFAuditError(f"mapping symbol {name} has invalid section index")
            section=sections[shndx]
            if not (section["flags"]&SHF_EXECINSTR): continue
            offset=value if elf["type"]==1 else value-int(section["address"],16)
            if offset<0 or offset>section["size"]: raise ELFAuditError(f"mapping symbol {name} lies outside section {section['name']}")
            result[shndx].append({"name":name,"kind":match.group(1),"offset":offset,"value":value})
    for idx,symbols in result.items():
        symbols.sort(key=lambda x:(x["offset"],x["kind"],x["name"]))
        compact=[]
        for symbol in symbols:
            if compact and compact[-1]["offset"]==symbol["offset"]:
                if compact[-1]["kind"]!=symbol["kind"]: raise ELFAuditError(f"conflicting mapping symbols at section offset {symbol['offset']}")
                continue
            compact.append(symbol)
        result[idx]=compact
    return dict(result)
def mapping_ranges(symbols, section_size):
    ranges=[]; cursor=0
    for i,symbol in enumerate(symbols):
        start=symbol["offset"]
        if start>cursor: ranges.append({"kind":"unknown","start":cursor,"end":start})
        end=symbols[i+1]["offset"] if i+1<len(symbols) else section_size
        if end>start: ranges.append({"kind":symbol["kind"],"start":start,"end":end,"mapping_symbol":symbol["name"]})
        cursor=end
    if not symbols and section_size: ranges.append({"kind":"unknown","start":0,"end":section_size})
    return ranges
def decode_section(code,address,mode,capstone_module,max_samples=200):
    cs_mode=capstone_module.CS_MODE_ARM if mode=="arm" else capstone_module.CS_MODE_THUMB
    engine=capstone_module.Cs(capstone_module.CS_ARCH_ARM,cs_mode); engine.detail=False
    cursor=0; decoded=0; counts=collections.Counter(); samples=[]; gaps=[]; unit=4 if mode=="arm" else 2
    while cursor<len(code):
        insn=next(engine.disasm(code[cursor:],address+cursor,count=1),None)
        if insn is None or insn.size<=0 or insn.size>len(code)-cursor or insn.address!=address+cursor:
            start=cursor; cursor=min(len(code),cursor+unit); gaps.append((start,cursor)); continue
        counts[insn.mnemonic]+=1; decoded+=insn.size
        if len(samples)<max_samples: samples.append({"address":f"0x{insn.address:x}","size":insn.size,"mnemonic":insn.mnemonic,"operands":insn.op_str})
        cursor+=insn.size
    merged=[]
    for start,end in gaps:
        if merged and merged[-1][1]==start: merged[-1][1]=end
        else: merged.append([start,end])
    return {"size":len(code),"decoded_bytes":decoded,"undecoded_bytes":len(code)-decoded,
            "decoded_instructions":sum(counts.values()),"mnemonics":dict(sorted(counts.items())),
            "undecoded_ranges":[{"offset":start,"address":f"0x{address+start:x}","size":end-start,"bytes":code[start:end].hex()} for start,end in merged],
            "sample_instructions":samples}
def inventory(data, mode=None, capstone_module=None):
    elf=audit_elf(data)
    if elf["elf_class"]!=32 or elf["machine"]!=40: raise ELFAuditError("instruction inventory currently requires ELF32 with e_machine=EM_ARM")
    mappings=parse_mapping_symbols(data,elf)
    if not mappings and mode not in ("arm","thumb"): raise ELFAuditError("no mapping symbols found; supply explicit fallback --mode arm or --mode thumb")
    if capstone_module is None:
        try: capstone_module=importlib.import_module("capstone")
        except ImportError as exc: raise RuntimeError("Capstone is required; install a reviewed/pinned package (see Stage 119V documentation)") from exc
    output=[]; total_bytes=decoded=decoder_gaps=data_bytes=unknown_bytes=instruction_count=0; all_counts=collections.Counter()
    for section in elf["sections"]:
        if not (section["flags"]&SHF_EXECINSTR) or section["size"]==0: continue
        off,size=section["offset"],section["size"]
        if off+size>len(data): raise ELFAuditError(f"executable section {section['name']} exceeds input bounds")
        code=data[off:off+size]; idx=section["index"]; symbols=mappings.get(idx,[])
        ranges=mapping_ranges(symbols,size) if symbols else ([{"kind":mode,"start":0,"end":size,"mapping_symbol":None}] if mode else [{"kind":"unknown","start":0,"end":size}])
        range_reports=[]; sec_decoded=sec_gaps=sec_data=sec_unknown=sec_insns=0; sec_counts=collections.Counter(); sec_samples=[]; base=int(section["address"],16)
        for item in ranges:
            start,end=item["start"],item["end"]; segment=code[start:end]; kind=item["kind"]
            record={"kind":kind,"offset":start,"address":f"0x{base+start:x}","size":end-start}
            if kind in ("a","t","arm","thumb"):
                decoder_mode={"a":"arm","t":"thumb","arm":"arm","thumb":"thumb"}[kind]
                result=decode_section(segment,base+start,decoder_mode,capstone_module); record.update(result)
                sec_decoded+=result["decoded_bytes"]; sec_gaps+=result["undecoded_bytes"]; sec_insns+=result["decoded_instructions"]
                sec_counts.update(result["mnemonics"]); sec_samples.extend(result["sample_instructions"][:max(0,200-len(sec_samples))])
            elif kind=="d": record["classification"]="data"; sec_data+=len(segment)
            else: record["classification"]="unmapped"; sec_unknown+=len(segment)
            if item.get("mapping_symbol"): record["mapping_symbol"]=item["mapping_symbol"]
            range_reports.append(record)
        total_bytes+=size; decoded+=sec_decoded; decoder_gaps+=sec_gaps; data_bytes+=sec_data; unknown_bytes+=sec_unknown; instruction_count+=sec_insns; all_counts.update(sec_counts)
        output.append({"name":section["name"],"address":section["address"],"file_offset":off,"size":size,"mapping_symbols":symbols,"ranges":range_reports,
                       "decoded_bytes":sec_decoded,"decoder_undecoded_bytes":sec_gaps,"data_bytes":sec_data,"unmapped_bytes":sec_unknown,
                       "decoded_instructions":sec_insns,"mnemonics":dict(sorted(sec_counts.items())),"sample_instructions":sec_samples})
    return {"source_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),"elf_class":elf["elf_class"],"machine":elf["machine_name"],
            "endianness":elf["endianness"],"fallback_mode":mode,"mode_source":"ELF mapping symbols" if mappings else "explicit fallback",
            "mapping_symbol_sections":len(mappings),"decoder":{"name":"Capstone","version":getattr(capstone_module,"__version__","unknown")},
            "summary":{"executable_sections":len(output),"raw_executable_bytes":total_bytes,"decoded_bytes":decoded,"decoder_undecoded_bytes":decoder_gaps,
                       "mapping_data_bytes":data_bytes,"unmapped_bytes":unknown_bytes,"decoded_instructions":instruction_count,"mnemonics":dict(sorted(all_counts.items()))},
            "sections":output,"evidence_limits":["Only local STB_LOCAL $a/$t/$d mapping symbols are used.",
            "A section without mapping symbols requires explicit fallback mode; mixed code/data is not guessed.",
            "Undecoded bytes are decoder failures, not proof of invalid code.","No proprietary input binary is stored or executed."]}
def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument("input",type=Path)
    parser.add_argument("--mode",choices=("arm","thumb"),help="explicit fallback mode only for sections with no mapping symbols")
    parser.add_argument("--json",type=Path); args=parser.parse_args()
    try:
        report=inventory(args.input.read_bytes(),args.mode); report["source"]=args.input.name; rendered=json.dumps(report,indent=2)
        if args.json: args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,ELFAuditError,RuntimeError) as exc: parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
