#include "re_guest_arm_branch_reloc.h"

#include <limits.h>

#define RE_ARM_R_PC24 1u
#define RE_ARM_R_PLT32 27u
#define RE_ARM_R_CALL 28u
#define RE_ARM_R_JUMP24 29u
#define RE_ARM_BRANCH_OPCODE_MASK 0x0e000000u
#define RE_ARM_BRANCH_OPCODE 0x0a000000u
#define RE_ARM_BRANCH_IMMEDIATE_MASK 0x00ffffffu

static int is_supported_type(uint32_t type) {
    return type==RE_ARM_R_PC24 || type==RE_ARM_R_PLT32 ||
           type==RE_ARM_R_CALL || type==RE_ARM_R_JUMP24;
}

static int32_t decode_rel_addend(uint32_t instruction) {
    uint32_t imm=instruction & RE_ARM_BRANCH_IMMEDIATE_MASK;
    int32_t signed_imm=(imm & 0x00800000u)
        ? (int32_t)(imm | 0xff000000u) : (int32_t)imm;
    return signed_imm * 4;
}

REGUESTARMBRANCHRELOCStatus re_guest_arm_branch_reloc_encode(
    uint32_t type, uint32_t instruction, uint32_t place,
    uint32_t symbol_value, int32_t explicit_addend,
    uint8_t has_explicit_addend, uint32_t *encoded_instruction) {
    int64_t addend, target, pc, displacement;
    uint32_t link_bit;

    if(!encoded_instruction || has_explicit_addend>1u)
        return RE_GUEST_ARM_BRANCH_RELOC_E_ARGUMENT;
    if(!is_supported_type(type))
        return RE_GUEST_ARM_BRANCH_RELOC_E_TYPE;
    if((place & 3u)!=0u)
        return RE_GUEST_ARM_BRANCH_RELOC_E_ALIGNMENT;
    if((instruction & RE_ARM_BRANCH_OPCODE_MASK)!=RE_ARM_BRANCH_OPCODE ||
       (instruction >> 28)==0x0fu)
        return RE_GUEST_ARM_BRANCH_RELOC_E_OPCODE;
    if((symbol_value & 1u)!=0u)
        return RE_GUEST_ARM_BRANCH_RELOC_E_INTERWORK;

    link_bit=(instruction >> 24)&1u;
    if((type==RE_ARM_R_CALL && link_bit==0u) ||
       (type==RE_ARM_R_JUMP24 && link_bit!=0u))
        return RE_GUEST_ARM_BRANCH_RELOC_E_OPCODE;

    addend=has_explicit_addend ? (int64_t)explicit_addend
                               : (int64_t)decode_rel_addend(instruction);
    target=(int64_t)(uint64_t)symbol_value+addend;
    pc=(int64_t)(uint64_t)place+8;
    if(target<0 || target>(int64_t)UINT32_MAX ||
       pc>(int64_t)UINT32_MAX)
        return RE_GUEST_ARM_BRANCH_RELOC_E_RANGE;
    displacement=target-pc;
    if((displacement & 3)!=0)
        return RE_GUEST_ARM_BRANCH_RELOC_E_ALIGNMENT;
    if(displacement < -33554432LL || displacement > 33554428LL)
        return RE_GUEST_ARM_BRANCH_RELOC_E_RANGE;

    *encoded_instruction=(instruction & ~RE_ARM_BRANCH_IMMEDIATE_MASK) |
        ((uint32_t)(displacement >> 2) & RE_ARM_BRANCH_IMMEDIATE_MASK);
    return RE_GUEST_ARM_BRANCH_RELOC_OK;
}
