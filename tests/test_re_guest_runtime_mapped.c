#include "re_guest_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static REGUESTIMPORTStatus mapped_add(const S3EAAPCS32Frame *frame,
    const REGUESTMEMBUS *bus, void *user, REGUESTIMPORTResult *out) {
    uint32_t fifth=0, marker=0;
    (void)user;
    if (s3e_aapcs32_arg_word(frame,4,&fifth)!=S3E_AAPCS32_OK)
        return RE_IMPORT_E_HANDLER;
    if (re_guest_memory_read32(bus,0x6004,&marker)!=RE_GUEST_MEM_OK ||
        marker!=0xA1B2C3D4u)
        return RE_IMPORT_E_HANDLER;
    out->r0=frame->r[0]+frame->r[1]+fifth;
    out->r1=marker;
    return RE_IMPORT_OK;
}
int main(void) {
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    uint8_t code[2]={0x09,0x22};
    uint8_t stack[32]={0};
    uint8_t slots[8]={0};
    uint32_t trampoline=0, pointer=0, slot_value=0;
    uint64_t done=0;
    stack[0]=7;
    stack[4]=0xD4;stack[5]=0xC3;stack[6]=0xB2;stack[7]=0xA1;
    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory,0x5000,sizeof(code),code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory,0x6000,sizeof(stack),stack,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory,0x6100,sizeof(slots),slots,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus,&memory);
    assert(re_guest_import_table_init(&imports,0x7000)==RE_IMPORT_OK);
    assert(re_guest_import_bind_mapped(&imports,42,mapped_add,NULL,
        &trampoline)==RE_IMPORT_OK);
    assert(re_guest_import_patch_slot_mapped(&imports,42,&bus,0x6100)==RE_IMPORT_OK);
    assert(re_guest_memory_read32(&bus,0x6100,&slot_value)==RE_GUEST_MEM_OK);
    assert(slot_value==trampoline);
    assert(re_guest_import_patch_slot_mapped(&imports,42,&bus,0x5000)==RE_IMPORT_E_MEMORY);
    re_guest_runtime_init_mapped(&runtime,&bus,&imports,0x7F000000u,0x5000,1);
    assert(re_guest_runtime_step(&runtime)==RE_RUNTIME_OK);
    assert(runtime.cpu.r[2]==9&&runtime.cpu_instructions==1);
    runtime.cpu.r[15]=trampoline&~1u;
    runtime.cpu.thumb=1;
    runtime.cpu.r[14]=0x5001u;
    runtime.cpu.r[13]=0x6000u;
    runtime.cpu.r[0]=100;runtime.cpu.r[1]=200;
    assert(re_guest_runtime_step(&runtime)==RE_RUNTIME_OK);
    assert(runtime.cpu.r[0]==307&&runtime.cpu.r[1]==0xA1B2C3D4u);
    assert(runtime.cpu.r[15]==0x5000u&&runtime.cpu.thumb==1);
    assert(runtime.dispatched_imports==1&&runtime.cpu_instructions==1);
    /* A missing mapped instruction region stops without advancing CPU state. */
    runtime.cpu.r[15]=0x8000u;runtime.cpu.thumb=1;
    assert(re_guest_runtime_step(&runtime)==RE_RUNTIME_E_CPU);
    assert(runtime.last_cpu_status==RE_CPU_E_MEMORY);
    assert(runtime.fault_valid&&runtime.fault_pc==0x8000u);
    puts("Mapped guest runtime integration tests: PASS");
    return 0;
}
