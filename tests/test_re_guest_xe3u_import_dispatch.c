#include "re_guest_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static void put32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static REGUESTIMPORTStatus plus_ten(const S3EAAPCS32Frame *f,const REGUESTMEM *m,
 void *user,REGUESTIMPORTResult *r){(void)m;(void)user;r->r0=f->r[0]+10u;r->r1=f->r[1];return RE_IMPORT_OK;}
int main(void){
 const size_t size=0x13b710u;const uint32_t base=0x50000000u;
 uint8_t *bytes=(uint8_t *)calloc(size,1);REGUESTMEM mem={bytes,size,base};
 REGUESTIMPORTTable imports;REGUESTRuntime rt;uint32_t thumb_ptr=0;
 assert(bytes);
 /* Original three ARM instructions at the real import-stub address 0x84. */
 put32(bytes+0x84,0xe28fc601u);put32(bytes+0x88,0xe28cca3bu);
 put32(bytes+0x8c,0xe5bcf67cu);
 assert(re_guest_import_table_init(&imports,0x60000000u)==RE_IMPORT_OK);
 assert(re_guest_import_bind(&imports,42u,plus_ten,NULL,&thumb_ptr)==RE_IMPORT_OK);
 assert((thumb_ptr&1u)==1u);
 assert(re_guest_import_patch_slot(&imports,42u,&mem,base+0x13b708u)==RE_IMPORT_OK);
 re_guest_runtime_init(&rt,&mem,&imports,0x61000000u,base+0x84u,0);
 rt.cpu.r[13]=base+0x1000u;rt.cpu.r[14]=base+0x300u;rt.cpu.r[0]=32u;rt.cpu.r[1]=7u;
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
 assert(rt.cpu.thumb&&rt.cpu.r[15]==(thumb_ptr&~1u));
 assert(rt.cpu.r[12]==base+0x13b708u);
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
 assert(rt.cpu.r[0]==42u&&rt.cpu.r[1]==7u);
 assert(!rt.cpu.thumb&&rt.cpu.r[15]==base+0x300u);
 assert(rt.dispatched_imports==1u&&rt.cpu_instructions==3u);
 /* Unknown import cells can point to the runtime's unresolved trap, not 0x70. */
 put32(bytes+0x13b708u,0x61000001u);
 re_guest_runtime_init(&rt,&mem,&imports,0x61000000u,base+0x84u,0);
 rt.cpu.r[13]=base+0x1000u;rt.cpu.r[14]=base+0x300u;
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
 assert(rt.cpu.thumb&&rt.cpu.r[15]==0x61000000u);
 assert(re_guest_runtime_step(&rt)==RE_RUNTIME_E_UNRESOLVED_IMPORT);
 free(bytes);
 puts("Guest XE3U direct Thumb import dispatch tests: PASS");
 return 0;
}
