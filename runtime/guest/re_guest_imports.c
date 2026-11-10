#include "re_guest_imports.h"

static int guest_range(const REGUESTMEM *m, uint32_t a, size_t n, size_t *off) {
    if (!m || !m->bytes || a < m->guest_base) return 0;
    *off = (size_t)(a - m->guest_base);
    return *off <= m->size && n <= m->size - *off;
}
static void write_le32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
REGUESTIMPORTStatus re_guest_import_table_init(REGUESTIMPORTTable *t,
                                                uint32_t base) {
    if (!t || (base & 3u)) return RE_IMPORT_E_ARGUMENT;
    for (size_t i=0;i<sizeof(*t);i++) ((uint8_t *)t)[i]=0;
    t->trampoline_base = base;
    return RE_IMPORT_OK;
}
REGUESTIMPORTStatus re_guest_import_bind(REGUESTIMPORTTable *t,
                                          uint32_t id,
                                          REGUESTIMPORTHandler h,
                                          void *user,
                                          uint32_t *ptr) {
    if (!t || !h || !ptr) return RE_IMPORT_E_ARGUMENT;
    if (t->count >= RE_IMPORT_MAX_BINDINGS) return RE_IMPORT_E_CAPACITY;
    for (size_t i=0;i<t->count;i++)
        if (t->entries[i].symbol_index == id) return RE_IMPORT_E_DUPLICATE;
    if (t->trampoline_base > UINT32_MAX - (uint32_t)(t->count*4u) - 4u)
        return RE_IMPORT_E_CAPACITY;
    uint32_t addr=t->trampoline_base+(uint32_t)(t->count*4u);
    t->entries[t->count].symbol_index=id;
    t->entries[t->count].trampoline_address=addr;
    t->entries[t->count].handler=h;
    t->entries[t->count].mapped_handler=NULL;
    t->entries[t->count].user=user;
    t->count++;
    *ptr=addr|1u; /* All external fixups in this executable are Thumb-tagged. */
    return RE_IMPORT_OK;
}
REGUESTIMPORTStatus re_guest_import_bind_mapped(REGUESTIMPORTTable *t,
    uint32_t id, REGUESTIMPORTMappedHandler h, void *user, uint32_t *ptr) {
    if (!t || !h || !ptr) return RE_IMPORT_E_ARGUMENT;
    if (t->count >= RE_IMPORT_MAX_BINDINGS) return RE_IMPORT_E_CAPACITY;
    for (size_t i=0;i<t->count;i++)
        if (t->entries[i].symbol_index == id) return RE_IMPORT_E_DUPLICATE;
    if (t->trampoline_base > UINT32_MAX - (uint32_t)(t->count*4u) - 4u)
        return RE_IMPORT_E_CAPACITY;
    uint32_t addr=t->trampoline_base+(uint32_t)(t->count*4u);
    t->entries[t->count].symbol_index=id;
    t->entries[t->count].trampoline_address=addr;
    t->entries[t->count].handler=NULL;
    t->entries[t->count].mapped_handler=h;
    t->entries[t->count].user=user;
    t->count++;
    *ptr=addr|1u;
    return RE_IMPORT_OK;
}
static const REGUESTIMPORTBinding *find_id(const REGUESTIMPORTTable *t,uint32_t id) {
    for(size_t i=0;i<t->count;i++) if(t->entries[i].symbol_index==id) return &t->entries[i];
    return 0;
}
REGUESTIMPORTStatus re_guest_import_patch_slot(const REGUESTIMPORTTable *t,
                                                uint32_t id,
                                                REGUESTMEM *m,
                                                uint32_t slot) {
    const REGUESTIMPORTBinding *b;
    size_t off;
    if(!t||!m) return RE_IMPORT_E_ARGUMENT;
    b=find_id(t,id);
    if(!b) return RE_IMPORT_E_UNBOUND;
    if(slot&3u) return RE_IMPORT_E_MEMORY;
    if(!guest_range(m,slot,4,&off)) return RE_IMPORT_E_MEMORY;
    write_le32(m->bytes+off,b->trampoline_address|1u);
    return RE_IMPORT_OK;
}

int re_guest_import_is_trampoline_pc(const REGUESTIMPORTTable *t,
                                     uint32_t pc, int thumb_state) {
    if (!t || !thumb_state) return 0;
    for (size_t i=0;i<t->count;i++)
        if (t->entries[i].trampoline_address==pc) return 1;
    return 0;
}

REGUESTIMPORTStatus re_guest_import_patch_slot_mapped(
    const REGUESTIMPORTTable *t, uint32_t id, REGUESTMEMBUS *bus, uint32_t slot) {
    const REGUESTIMPORTBinding *b;
    if (!t || !bus || !bus->memory) return RE_IMPORT_E_ARGUMENT;
    b=find_id(t,id);
    if (!b) return RE_IMPORT_E_UNBOUND;
    if (slot&3u) return RE_IMPORT_E_MEMORY;
    return re_guest_memory_write32(bus,slot,b->trampoline_address|1u)==
        RE_GUEST_MEM_OK ? RE_IMPORT_OK : RE_IMPORT_E_MEMORY;
}

REGUESTIMPORTStatus re_guest_import_dispatch_mapped(
    REGUESTIMPORTTable *t, REGUESTCPU *cpu, const REGUESTMEMBUS *bus) {
    const REGUESTIMPORTBinding *b=NULL;
    S3EAAPCS32Frame frame;
    REGUESTIMPORTResult result={0,0};
    REGUESTCPU before;
    REGUESTIMPORTStatus hs;
    uint8_t stack[RE_IMPORT_MAPPED_STACK_BYTES];
    size_t available=0;
    uint32_t pc,lr;
    if (!t || !cpu || !bus || !bus->memory) return RE_IMPORT_E_ARGUMENT;
    pc=cpu->r[15];
    for(size_t i=0;i<t->count;i++)
        if(t->entries[i].trampoline_address==pc){b=&t->entries[i];break;}
    if(!b || !cpu->thumb) return RE_IMPORT_E_UNBOUND;
    if(!b->mapped_handler) return RE_IMPORT_E_HANDLER;
    if(cpu->r[13]&3u) return RE_IMPORT_E_MEMORY;
    frame.r[0]=cpu->r[0];frame.r[1]=cpu->r[1];
    frame.r[2]=cpu->r[2];frame.r[3]=cpu->r[3];frame.sp=cpu->r[13];
    while(available<sizeof(stack)) {
        uint64_t address=(uint64_t)frame.sp+available;
        uint8_t value;
        if(address>UINT32_MAX ||
           re_guest_memory_read8(bus,(uint32_t)address,&value)!=RE_GUEST_MEM_OK)
            break;
        stack[available++]=value;
    }
    frame.stack_words=stack;
    frame.stack_bytes=available;
    before=*cpu;
    hs=b->mapped_handler(&frame,bus,b->user,&result);
    if(hs!=RE_IMPORT_OK) {
        *cpu=before;
        return hs==RE_IMPORT_E_UNBOUND?hs:RE_IMPORT_E_HANDLER;
    }
    lr=cpu->r[14];
    cpu->r[0]=result.r0;cpu->r[1]=result.r1;
    cpu->thumb=(uint8_t)(lr&1u);
    cpu->r[15]=lr&(cpu->thumb?~1u:~3u);
    return RE_IMPORT_OK;
}

REGUESTIMPORTStatus re_guest_import_dispatch(REGUESTIMPORTTable *t,
                                              REGUESTCPU *cpu,
                                              const REGUESTMEM *m) {
    const REGUESTIMPORTBinding *b=0;
    S3EAAPCS32Frame f;
    REGUESTIMPORTResult result={0,0};
    REGUESTIMPORTStatus hs;
    uint32_t pc,lr;
    size_t stack_off=0;
    if(!t||!cpu||!m||!m->bytes) return RE_IMPORT_E_ARGUMENT;
    pc=cpu->r[15];
    for(size_t i=0;i<t->count;i++)
        if(t->entries[i].trampoline_address==pc) { b=&t->entries[i]; break; }
    if(!b || !cpu->thumb) return RE_IMPORT_E_UNBOUND;
    if((cpu->r[13]&3u)!=0u) return RE_IMPORT_E_MEMORY;
    f.r[0]=cpu->r[0]; f.r[1]=cpu->r[1]; f.r[2]=cpu->r[2]; f.r[3]=cpu->r[3];
    f.sp=cpu->r[13];
    if(!guest_range(m,f.sp,0,&stack_off)) return RE_IMPORT_E_MEMORY;
    f.stack_words=m->bytes+stack_off;
    f.stack_bytes=m->size-stack_off;
    hs=b->handler(&f,m,b->user,&result);
    if(hs!=RE_IMPORT_OK) return hs==RE_IMPORT_E_UNBOUND?hs:RE_IMPORT_E_HANDLER;
    cpu->r[0]=result.r0;
    cpu->r[1]=result.r1;
    lr=cpu->r[14];
    cpu->thumb=(uint8_t)(lr&1u);
    cpu->r[15]=lr&(cpu->thumb?~1u:~3u);
    return RE_IMPORT_OK;
}
