#include "re_guest_cpu_arm_scalar.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Independent byte-array reference model for the supported A32 subset. */
static uint32_t rng_state=0x6d2b79f5u;
static uint32_t rnd(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static uint32_t ref_read(const uint8_t *m,unsigned a,unsigned width) {
    uint32_t v=0;
    for(unsigned i=0;i<width;i++)
        v |= (uint32_t)m[a+i] << (8u*i);
    return v;
}
static void ref_write(uint8_t *m,unsigned a,unsigned width,uint32_t v) {
    for(unsigned i=0;i<width;i++)
        m[a+i]=(uint8_t)(v>>(8u*i));
}
static uint32_t sign_extend(uint32_t v,unsigned width) {
    if(width==1u) return (uint32_t)(int32_t)(int8_t)v;
    if(width==2u) return (uint32_t)(int32_t)(int16_t)v;
    return v;
}

static uint32_t encode_single(unsigned load,unsigned byte,unsigned rn,
 unsigned rd,unsigned add,unsigned offset) {
    return 0xe4000000u | (1u<<24) | (add<<23) |
        (byte<<22) | (load<<20) | (rn<<16) | (rd<<12) | offset;
}
static uint32_t encode_extra(unsigned load,unsigned sh,unsigned rn,
 unsigned rd,unsigned add,unsigned offset) {
    return 0xe0000090u | (1u<<24) | (add<<23) | (1u<<22) |
        (load<<20) | (rn<<16) | (rd<<12) |
        (((offset>>4)&15u)<<8) | (sh<<5) | (1u<<4) |
        (offset&15u);
}

int main(void) {
    REGUESTMEMORY map;
    REGUESTALIGNMENTPOLICY policy;
    REGUESTCPUSCALAR scalar;
    uint8_t actual[256], expected[256];

    re_guest_memory_init(&map);
    re_guest_alignment_policy_init(&policy);
    assert(re_guest_memory_map_ex(&map,0x8000u,sizeof(actual),actual,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,
        RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
    re_guest_cpu_scalar_init(&scalar,&map,&policy);

    for(unsigned iteration=0;iteration<512u;iteration++) {
        REGUESTCPU cpu,reference;
        unsigned kind=iteration%8u;
        unsigned width=(kind==0u||kind==1u)?4u:
            (kind==4u||kind==5u||kind==7u)?2u:1u;
        unsigned load=!(kind==1u||kind==3u||kind==5u);
        unsigned byte=(kind==2u||kind==3u);
        unsigned sh=(kind==4u||kind==5u)?1u:
            kind==6u?2u:3u;
        unsigned rn=1u+(rnd()%6u);
        unsigned rd=7u+(rnd()%7u);
        unsigned add=rnd()&1u;
        unsigned offset=(rnd()%24u)*width;
        unsigned base_index=128u;
        unsigned address=add?base_index+offset:base_index-offset;
        uint32_t instruction;

        for(unsigned i=0;i<sizeof(actual);i++)
            actual[i]=(uint8_t)rnd();
        memcpy(expected,actual,sizeof(actual));

        re_guest_cpu_reset(&cpu,0x1000u,0);
        for(unsigned r=0;r<16u;r++)
            cpu.r[r]=rnd();
        cpu.r[rn]=0x8000u+base_index;
        reference=cpu;

        if(kind<=3u)
            instruction=encode_single(load,byte,rn,rd,add,offset);
        else
            instruction=encode_extra(load,sh,rn,rd,add,offset);

        if(load) {
            uint32_t value=ref_read(expected,address,width);
            if(kind==6u||kind==7u)
                value=sign_extend(value,width);
            reference.r[rd]=value;
        } else {
            ref_write(expected,address,width,reference.r[rd]);
        }

        REGUESTCPUStatus status=re_guest_cpu_arm_scalar_transfer(
            &cpu,&scalar,instruction);
        assert(status==RE_CPU_OK);
        assert(memcmp(cpu.r,reference.r,sizeof(cpu.r))==0);
        assert(memcmp(actual,expected,sizeof(actual))==0);
    }

    puts("A32 scalar differential reference tests: PASS (512 cases)");
    return 0;
}
