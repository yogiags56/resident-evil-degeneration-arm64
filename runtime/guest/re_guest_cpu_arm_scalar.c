#include "re_guest_cpu_arm_scalar.h"

static uint32_t transfer_address(uint32_t base,uint32_t offset,int add) {
    return add ? base + offset : base - offset;
}

static REGUESTCPUStatus load_value(
    const REGUESTCPUSCALAR *memory,
    uint32_t address,
    unsigned kind,
    uint32_t *value
) {
    uint8_t byte_value = 0;
    uint16_t half_value = 0;
    REGUESTCPUStatus status;

    if (!value) return RE_CPU_E_ARGUMENT;

    switch (kind) {
    case 0: /* unsigned byte */
        status = re_guest_cpu_scalar_read8(memory,address,&byte_value);
        if (status == RE_CPU_OK) *value = byte_value;
        return status;
    case 1: /* unsigned word */
        return re_guest_cpu_scalar_read32(memory,address,value);
    case 2: /* unsigned halfword */
        status = re_guest_cpu_scalar_read16(memory,address,&half_value);
        if (status == RE_CPU_OK) *value = half_value;
        return status;
    case 3: /* signed byte */
        status = re_guest_cpu_scalar_read8(memory,address,&byte_value);
        if (status == RE_CPU_OK)
            *value = (uint32_t)(int32_t)(int8_t)byte_value;
        return status;
    case 4: /* signed halfword */
        status = re_guest_cpu_scalar_read16(memory,address,&half_value);
        if (status == RE_CPU_OK)
            *value = (uint32_t)(int32_t)(int16_t)half_value;
        return status;
    default:
        return RE_CPU_E_UNIMPLEMENTED;
    }
}

REGUESTCPUStatus re_guest_cpu_arm_scalar_transfer(
    REGUESTCPU *cpu,
    const REGUESTCPUSCALAR *memory,
    uint32_t instruction
) {
    unsigned rn,rd,kind;
    uint32_t base,offset,address,result=0;
    int load,add,byte;
    REGUESTCPUStatus status;

    if (!cpu || !memory) return RE_CPU_E_ARGUMENT;
    if ((instruction >> 28) != 0xeu) return RE_CPU_E_UNIMPLEMENTED;

    /*
     * Ordinary A32 single data transfer, immediate, pre-index, no writeback.
     * Excludes register-offset and post-index forms.
     */
    if ((instruction & 0x0f200000u) == 0x05000000u) {
        rn=(instruction>>16)&15u;
        rd=(instruction>>12)&15u;
        if (rn==15u || rd==15u) return RE_CPU_E_UNIMPLEMENTED;

        load=!!(instruction&(1u<<20));
        byte=!!(instruction&(1u<<22));
        add=!!(instruction&(1u<<23));
        base=cpu->r[rn];
        offset=instruction&0x0fffu;
        address=transfer_address(base,offset,add);

        if (load) {
            status=load_value(memory,address,byte?0u:1u,&result);
            if (status!=RE_CPU_OK) return status;
            cpu->r[rd]=result;
            return RE_CPU_OK;
        }
        if (byte)
            return re_guest_cpu_scalar_write8(memory,address,(uint8_t)cpu->r[rd]);
        return re_guest_cpu_scalar_write32(memory,address,cpu->r[rd]);
    }

    /*
     * A32 extra load/store immediate encoding:
     * bits[27:25]=000, bit[7]=1, bit[4]=1, I=1, P=1, W=0.
     * SH=01: LDRH/STRH; SH=10: LDRSB; SH=11: LDRSH.
     */
    if ((instruction & 0x0e000090u) != 0x00000090u ||
        !(instruction&(1u<<22)) ||
        !(instruction&(1u<<24)) ||
        (instruction&(1u<<21)))
        return RE_CPU_E_UNIMPLEMENTED;

    rn=(instruction>>16)&15u;
    rd=(instruction>>12)&15u;
    if (rn==15u || rd==15u) return RE_CPU_E_UNIMPLEMENTED;

    {
        unsigned sh=(instruction>>5)&3u;
        if (sh==0u) return RE_CPU_E_UNIMPLEMENTED;
        if (sh==1u) kind=2u;
        else if (sh==2u) kind=3u;
        else kind=4u;
        load=!!(instruction&(1u<<20));
        if (!load && sh!=1u) return RE_CPU_E_UNIMPLEMENTED;
    }

    base=cpu->r[rn];
    offset=(((instruction>>8)&15u)<<4)|(instruction&15u);
    add=!!(instruction&(1u<<23));
    address=transfer_address(base,offset,add);

    if (load) {
        status=load_value(memory,address,kind,&result);
        if (status!=RE_CPU_OK) return status;
        cpu->r[rd]=result;
        return RE_CPU_OK;
    }

    return re_guest_cpu_scalar_write16(
        memory,address,(uint16_t)cpu->r[rd]);
}
