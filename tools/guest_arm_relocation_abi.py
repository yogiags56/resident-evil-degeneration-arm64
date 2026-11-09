#!/usr/bin/env python3
"""Evidence-backed ELF32 ARM relocation metadata; never applies relocations."""
from __future__ import annotations

# Width is the number of bytes occupied at r_offset, not the size of the
# encoded relocation record. Alignment follows the AAELF32 place requirements.
# "unknown" is intentional for COPY: its write size is the symbol st_size.
SPECS={
  0:  {"name":"R_ARM_NONE","width":0,"alignment":1,"class":"misc","kind":"static","semantics":"no operation"},
  1:  {"name":"R_ARM_PC24","width":4,"alignment":4,"class":"arm32","kind":"deprecated","semantics":"((S + A) | T) - P"},
  2:  {"name":"R_ARM_ABS32","width":4,"alignment":1,"class":"data","kind":"static","semantics":"(S + A) | T"},
  3:  {"name":"R_ARM_REL32","width":4,"alignment":1,"class":"data","kind":"static","semantics":"((S + A) | T) - P"},
  4:  {"name":"R_ARM_LDR_PC_G0","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"S + A - P"},
  5:  {"name":"R_ARM_ABS16","width":2,"alignment":1,"class":"data","kind":"static","semantics":"S + A"},
  6:  {"name":"R_ARM_ABS12","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"S + A"},
  7:  {"name":"R_ARM_THM_ABS5","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"S + A"},
  8:  {"name":"R_ARM_ABS8","width":1,"alignment":1,"class":"data","kind":"static","semantics":"S + A"},
  9:  {"name":"R_ARM_SBREL32","width":4,"alignment":1,"class":"data","kind":"static","semantics":"((S + A) | T) - B(S)"},
  10: {"name":"R_ARM_THM_CALL","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"((S + A) | T) - P"},
  11: {"name":"R_ARM_THM_PC8","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"S + A - Pa"},
  17: {"name":"R_ARM_TLS_DTPMOD32","width":4,"alignment":4,"class":"data","kind":"dynamic","semantics":"module index of defining/current module"},
  18: {"name":"R_ARM_TLS_DTPOFF32","width":4,"alignment":4,"class":"data","kind":"dynamic","semantics":"TLS symbol offset"},
  19: {"name":"R_ARM_TLS_TPOFF32","width":4,"alignment":4,"class":"data","kind":"dynamic","semantics":"TLS symbol offset from TP"},
  20: {"name":"R_ARM_COPY","width":None,"alignment":1,"class":"variable_data","kind":"dynamic","semantics":"copy symbol st_size bytes","width_source":"symbol_st_size"},
  21: {"name":"R_ARM_GLOB_DAT","width":4,"alignment":4,"class":"data","kind":"dynamic","semantics":"symbol address"},
  22: {"name":"R_ARM_JUMP_SLOT","width":4,"alignment":4,"class":"data","kind":"dynamic","semantics":"symbol address"},
  23: {"name":"R_ARM_RELATIVE","width":4,"alignment":4,"class":"data","kind":"dynamic","semantics":"load-bias delta plus implicit addend"},
  24: {"name":"R_ARM_GOTOFF32","width":4,"alignment":1,"class":"data","kind":"static","semantics":"((S + A) | T) - GOT_ORG"},
  25: {"name":"R_ARM_BASE_PREL","width":4,"alignment":1,"class":"data","kind":"static","semantics":"B(S) + A - P"},
  26: {"name":"R_ARM_GOT_BREL","width":4,"alignment":1,"class":"data","kind":"static","semantics":"GOT(S) + A - GOT_ORG"},
  27: {"name":"R_ARM_PLT32","width":4,"alignment":4,"class":"arm32","kind":"deprecated","semantics":"((S + A) | T) - P"},
  28: {"name":"R_ARM_CALL","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"((S + A) | T) - P"},
  29: {"name":"R_ARM_JUMP24","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"((S + A) | T) - P"},
  30: {"name":"R_ARM_THM_JUMP24","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"((S + A) | T) - P"},
  31: {"name":"R_ARM_BASE_ABS","width":4,"alignment":1,"class":"data","kind":"static","semantics":"B(S) + A"},
  38: {"name":"R_ARM_TARGET1","width":4,"alignment":1,"class":"data","kind":"platform_specific","semantics":"platform-defined; commonly ABS32"},
  42: {"name":"R_ARM_PREL31","width":4,"alignment":1,"class":"data","kind":"static","semantics":"S + A - P; 31-bit signed field"},
  43: {"name":"R_ARM_MOVW_ABS_NC","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"low 16 bits of S + A"},
  44: {"name":"R_ARM_MOVT_ABS","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"high 16 bits of S + A"},
  45: {"name":"R_ARM_MOVW_PREL_NC","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"low 16 bits of S + A - P"},
  46: {"name":"R_ARM_MOVT_PREL","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"high 16 bits of S + A - P"},
  47: {"name":"R_ARM_THM_MOVW_ABS_NC","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"low 16 bits of S + A"},
  48: {"name":"R_ARM_THM_MOVT_ABS","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"high 16 bits of S + A"},
  49: {"name":"R_ARM_THM_MOVW_PREL_NC","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"low 16 bits of S + A - P"},
  50: {"name":"R_ARM_THM_MOVT_PREL","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"high 16 bits of S + A - P"},
  51: {"name":"R_ARM_THM_JUMP19","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"conditional Thumb branch displacement"},
  52: {"name":"R_ARM_THM_JUMP6","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"CBZ/CBNZ displacement"},
  53: {"name":"R_ARM_THM_ALU_PREL_11_0","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"ADR.W displacement"},
  54: {"name":"R_ARM_THM_PC12","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"Thumb literal load displacement"},
  57: {"name":"R_ARM_ALU_PC_G0_NC","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM PC-relative ALU"},
  58: {"name":"R_ARM_ALU_PC_G0","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM PC-relative ALU"},
  59: {"name":"R_ARM_ALU_PC_G1_NC","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM PC-relative ALU"},
  60: {"name":"R_ARM_ALU_PC_G1","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM PC-relative ALU"},
  61: {"name":"R_ARM_ALU_PC_G2","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM PC-relative ALU"},
  62: {"name":"R_ARM_LDR_PC_G1","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM PC-relative load/store"},
  63: {"name":"R_ARM_LDR_PC_G2","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM PC-relative load/store"},
  64: {"name":"R_ARM_LDRS_PC_G0","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM signed/halfword load/store"},
  65: {"name":"R_ARM_LDRS_PC_G1","width":4,"alignment":4,"class":"arm32","kind":"static","semantics":"grouped ARM signed/halfword load/store"},
  87: {"name":"R_ARM_THM_MOVW_BREL_NC","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"low 16 bits of SB-relative value"},
  88: {"name":"R_ARM_THM_MOVT_BREL","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"high 16 bits of SB-relative value"},
  89: {"name":"R_ARM_THM_MOVW_BREL","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"checked low 16 bits of SB-relative value"},
  102:{"name":"R_ARM_THM_JUMP11","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"Thumb unconditional branch displacement"},
  103:{"name":"R_ARM_THM_JUMP8","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"Thumb conditional branch displacement"},
  131:{"name":"R_ARM_THM_GOT_BREL12","width":4,"alignment":2,"class":"thumb32","kind":"static","semantics":"Thumb GOT-relative load"},
  132:{"name":"R_ARM_THM_ALU_ABS_G0_NC","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"low byte of absolute value"},
  133:{"name":"R_ARM_THM_ALU_ABS_G1_NC","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"second byte of absolute value"},
  134:{"name":"R_ARM_THM_ALU_ABS_G2_NC","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"third byte of absolute value"},
  135:{"name":"R_ARM_THM_ALU_ABS_G3","width":2,"alignment":2,"class":"thumb16","kind":"static","semantics":"top byte of absolute value"},
}

def get_spec(relocation_type):
    spec=SPECS.get(relocation_type)
    return dict(spec) if spec is not None else None

def contract_for(relocation_type, symbol_index=None, target_domain="virtual", encoding=None):
    """Return conservative ABI contract metadata; never infer provider resolution."""
    spec=get_spec(relocation_type)
    if spec is None:
        return {"known":False,"symbol_policy":"unknown","applicability":"unknown",
                "target_policy":"unknown","contract_issues":[]}
    no_symbol={0,23}
    mandatory_symbol={17,18,19,20,21,22}
    if relocation_type in no_symbol:
        symbol_policy="must_be_zero_or_absent"
    elif relocation_type in mandatory_symbol:
        symbol_policy="must_be_nonzero"
    else:
        symbol_policy="may_be_zero"
    if relocation_type in {17,18,19,20,21,22,23}:
        applicability="dynamic"
    elif spec["kind"] in {"static","deprecated","platform_specific"}:
        applicability="static_or_link_time"
    else:
        applicability="unknown"
    target_policy="none" if spec["width"]==0 else (
        "instruction" if spec["class"] in {"arm32","thumb16","thumb32"} else
        "memory")
    issues=[]
    if symbol_index is not None:
        if symbol_policy=="must_be_zero_or_absent" and symbol_index!=0:
            issues.append("unexpected_symbol_index_for_symbol_free_relocation")
        elif symbol_policy=="must_be_nonzero" and symbol_index==0:
            issues.append("required_symbol_index_is_zero")
    if target_domain!="virtual" and spec["width"] not in (0,None):
        issues.append("non_virtual_target_domain_for_address_relocation")
    if encoding=="RELR" and relocation_type!=23:
        issues.append("unexpected_relocation_type_for_RELR")
    return {"known":True,"symbol_policy":symbol_policy,"applicability":applicability,
            "target_policy":target_policy,"contract_issues":issues}

def known_widths():
    return {code:spec["width"] for code,spec in SPECS.items()}
