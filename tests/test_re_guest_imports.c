#include "re_guest_imports.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static REGUESTIMPORTStatus add_handler(const S3EAAPCS32Frame *f,
    const REGUESTMEM *m, void *u, REGUESTIMPORTResult *r) {
    uint32_t fifth=0;
    (void)m; (void)u;
    if(s3e_aapcs32_arg_word(f,4,&fifth)!=S3E_AAPCS32_OK) return RE_IMPORT_E_HANDLER;
    r->r0=f->r[0]+f->r[1]+fifth;
    r->r1=0xAABBCCDDu;
    return RE_IMPORT_OK;
}
static REGUESTIMPORTStatus fail_handler(const S3EAAPCS32Frame *f,
    const REGUESTMEM *m, void *u, REGUESTIMPORTResult *r) {
    (void)f;(void)m;(void)u;(void)r;
    return RE_IMPORT_E_HANDLER;
}
static uint32_t rd32(const uint8_t*p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
int main(void) {
    uint8_t membytes[128]={0};
    REGUESTMEM mem={membytes,sizeof(membytes),0x50000000u};
    REGUESTIMPORTTable table;
    REGUESTCPU cpu;
    uint32_t p0=0,p1=0;
    /* fifth AAPCS word lives at entry SP. */
    membytes[64]=7; membytes[65]=0; membytes[66]=0; membytes[67]=0;
    assert(re_guest_import_table_init(&table,0x51000000u)==RE_IMPORT_OK);
    assert(re_guest_import_bind(&table,11,add_handler,0,&p0)==RE_IMPORT_OK);
    assert(re_guest_import_bind(&table,12,fail_handler,0,&p1)==RE_IMPORT_OK);
    assert((p0&1u)==1u && (p1&1u)==1u && p0!=p1);
    assert(re_guest_import_bind(&table,11,add_handler,0,&p1)==RE_IMPORT_E_DUPLICATE);
    assert(re_guest_import_patch_slot(&table,11,&mem,0x50000010u)==RE_IMPORT_OK);
    assert(rd32(membytes+16)==p0);
    memset(&cpu,0,sizeof(cpu));
    cpu.thumb=1; cpu.r[15]=p0&~1u; cpu.r[14]=0x50000021u;
    cpu.r[13]=0x50000040u; cpu.r[0]=100; cpu.r[1]=200;
    assert(re_guest_import_dispatch(&table,&cpu,&mem)==RE_IMPORT_OK);
    assert(cpu.r[0]==307 && cpu.r[1]==0xAABBCCDDu);
    assert(cpu.r[15]==0x50000020u && cpu.thumb==1);
    cpu.r[15]=p1&~1u; cpu.thumb=1;
    assert(re_guest_import_dispatch(&table,&cpu,&mem)==RE_IMPORT_E_HANDLER);
    cpu.r[15]=0x51000080u; cpu.thumb=1;
    assert(re_guest_import_dispatch(&table,&cpu,&mem)==RE_IMPORT_E_UNBOUND);
    assert(re_guest_import_patch_slot(&table,11,&mem,0x5000007fu)==RE_IMPORT_E_MEMORY);
    assert(re_guest_import_patch_slot(&table,99,&mem,0x50000010u)==RE_IMPORT_E_UNBOUND);
    puts("Guest import trampoline tests: PASS (11 assertions)");
    return 0;
}
