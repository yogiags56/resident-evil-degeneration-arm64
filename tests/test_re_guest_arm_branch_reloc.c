#include "re_guest_arm_branch_reloc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define ARM_B  0xea000000u
#define ARM_BL 0xeb000000u

int main(void) {
    uint32_t out=0xdeadbeefu;

    /* A32 BL from 0x1000 to 0x1100: PC is instruction address + 8. */
    assert(re_guest_arm_branch_reloc_encode(28u,ARM_BL,0x1000u,0x1100u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_OK);
    assert(out==0xeb00003eu);

    /* A32 B negative displacement and REL addend decoding. */
    assert(re_guest_arm_branch_reloc_encode(29u,ARM_B,0x1000u,0x0f00u,
        0,0,&out)==RE_GUEST_ARM_BRANCH_RELOC_OK);
    assert((out&0x00ffffffu)==0x00ffffbeu);

    /* Explicit addend is signed bytes, not a pre-shifted imm24. */
    assert(re_guest_arm_branch_reloc_encode(28u,ARM_BL,0x1000u,0x1100u,
        4,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_OK);
    assert(out==0xeb00003fu);

    out=0xdeadbeefu;
    assert(re_guest_arm_branch_reloc_encode(28u,ARM_B,0x1000u,0x1100u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_OPCODE);
    assert(out==0xdeadbeefu);
    assert(re_guest_arm_branch_reloc_encode(29u,ARM_BL,0x1000u,0x1100u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_OPCODE);
    assert(re_guest_arm_branch_reloc_encode(28u,0xf0000000u,0x1000u,0x1100u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_OPCODE);
    assert(re_guest_arm_branch_reloc_encode(28u,ARM_BL,0x1002u,0x1100u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_ALIGNMENT);
    assert(re_guest_arm_branch_reloc_encode(28u,ARM_BL,0x1000u,0x1101u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_INTERWORK);
    assert(re_guest_arm_branch_reloc_encode(10u,ARM_BL,0x1000u,0x1100u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_TYPE);
    assert(re_guest_arm_branch_reloc_encode(28u,ARM_BL,0xfffffffcu,0x1000u,
        0,1,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_RANGE);
    assert(re_guest_arm_branch_reloc_encode(28u,ARM_BL,0x1000u,0x1100u,
        0,2,&out)==RE_GUEST_ARM_BRANCH_RELOC_E_ARGUMENT);
    puts("ARM A32 branch relocation tests: PASS");
    return 0;
}
