#ifndef RE_GUEST_ARM_BRANCH_RELOC_H
#define RE_GUEST_ARM_BRANCH_RELOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RE_GUEST_ARM_BRANCH_RELOC_OK = 0,
    RE_GUEST_ARM_BRANCH_RELOC_E_ARGUMENT = -700,
    RE_GUEST_ARM_BRANCH_RELOC_E_TYPE = -701,
    RE_GUEST_ARM_BRANCH_RELOC_E_OPCODE = -702,
    RE_GUEST_ARM_BRANCH_RELOC_E_INTERWORK = -703,
    RE_GUEST_ARM_BRANCH_RELOC_E_ALIGNMENT = -704,
    RE_GUEST_ARM_BRANCH_RELOC_E_RANGE = -705
} REGUESTARMBRANCHRELOCStatus;

/*
 * Encode an ARM-state A32 B/BL relocation.
 * place is the guest address of the instruction; symbol_value must identify
 * an ARM-state target (bit 0 clear). For REL, addend is ignored and decoded
 * from imm24; for RELA, addend is the explicit signed byte addend.
 * This helper deliberately rejects Thumb interworking and BLX encodings.
 */
REGUESTARMBRANCHRELOCStatus re_guest_arm_branch_reloc_encode(
    uint32_t relocation_type,
    uint32_t instruction,
    uint32_t place,
    uint32_t symbol_value,
    int32_t explicit_addend,
    uint8_t has_explicit_addend,
    uint32_t *encoded_instruction);

#ifdef __cplusplus
}
#endif
#endif
