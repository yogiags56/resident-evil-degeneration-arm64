#include "re_guest_xe3u_import_plan.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static REGUESTIMPORTStatus handler(const S3EAAPCS32Frame*f,const REGUESTMEMBUS*bus,void*u,REGUESTIMPORTResult*r){
 (void)bus;(void)u;r->r0=f->r[0]+1u;r->r1=0;return RE_IMPORT_OK;
}
static void init_image(uint8_t*code,size_t size,uint32_t base,REGUESTXE3UHEADER*h){
 memset(h,0,sizeof(*h));h->base_address=base;h->code_file_size=(uint32_t)size;
 h->code_memory_size=(uint32_t)size+0x1000u;
 put32(code+0x84,0xe28fc601u);put32(code+0x88,0xe28cca3bu);
 put32(code+0x8c,0xe5bcf67cu);
 put32(code+0x100,0);put32(code+0x13b708u,base+0x70u);
}
int main(void){
 const size_t size=0x13b710u;const uint32_t base=0x50000000u;
 uint8_t*code=(uint8_t*)calloc(size,1);REGUESTXE3UHEADER h;
 REGUESTXE3UEXTERNALRELOCATION ext[2]={{0x100u,6u,4u},{0x13b708u,5u,4u}};
 REGUESTIMPORTTable imports;uint32_t ptr=0,scratch[4];size_t unbound=0,failed=0;
 REGUESTXE3UEXTERNALPATCH plan[2],before[2];
 assert(code);init_image(code,size,base,&h);
 assert(re_guest_import_table_init(&imports,0x60000000u)==RE_IMPORT_OK);
 assert(re_guest_import_bind_mapped(&imports,5u,handler,NULL,&ptr)==RE_IMPORT_OK);
 assert(re_guest_xe3u_prepare_external_import_plan(code,size,&h,7,ext,2,
   &imports,base,0x61000000u,1,scratch,4,plan,2,&unbound,&failed)==RE_GUEST_XE3U_IMPORT_PLAN_OK);
 assert(unbound==1&&failed==2);
 assert(plan[0].cell_offset==0x100u&&plan[0].original_value==0);
 assert(plan[0].trampoline_value==0x61000001u&&!plan[0].is_arm_stub);
 assert(plan[1].cell_offset==0x13b708u&&plan[1].original_value==base+0x70u);
 assert(plan[1].trampoline_value==ptr&&plan[1].is_arm_stub);
 memcpy(before,plan,sizeof(plan));
 assert(re_guest_xe3u_prepare_external_import_plan(code,size,&h,7,ext,2,
   &imports,base,0,0,scratch,4,plan,2,&unbound,&failed)==RE_GUEST_XE3U_IMPORT_PLAN_E_UNBOUND);
 assert(memcmp(plan,before,sizeof(plan))==0);

 /* Successful all-cell apply to one writable mapped image. */
 {
  uint8_t*mapped=(uint8_t*)malloc(size);REGUESTMEMORY memory;REGUESTMEMBUS bus;uint32_t v;
  assert(mapped);memcpy(mapped,code,size);re_guest_memory_init(&memory);
  assert(re_guest_memory_map_ex(&memory,base,size,mapped,
    RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC,
    RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
  re_guest_memory_bus_init(&bus,&memory);
  assert(re_guest_xe3u_apply_external_import_plan(&bus,&h,base,plan,2,&failed)==RE_GUEST_XE3U_IMPORT_PLAN_OK);
  assert(re_guest_memory_read32(&bus,base+0x100u,&v)==RE_GUEST_MEM_OK&&v==0x61000001u);
  assert(re_guest_memory_read32(&bus,base+0x13b708u,&v)==RE_GUEST_MEM_OK&&v==ptr);
  free(mapped);
 }
 /* If a later cell is read-only, earlier writes are rolled back. */
 {
  uint8_t*a=(uint8_t*)malloc(0x200u),*b=(uint8_t*)malloc(0x710u);
  REGUESTMEMORY memory;REGUESTMEMBUS bus;uint32_t v;
  assert(a&&b);memcpy(a,code,0x200u);memcpy(b,code+0x13b000u,0x710u);
  re_guest_memory_init(&memory);
  assert(re_guest_memory_map_ex(&memory,base,0x200u,a,
    RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC,
    RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
  assert(re_guest_memory_map_ex(&memory,base+0x13b000u,0x710u,b,
    RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC,
    RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
  re_guest_memory_bus_init(&bus,&memory);
  assert(re_guest_xe3u_apply_external_import_plan(&bus,&h,base,plan,2,&failed)==RE_GUEST_XE3U_IMPORT_PLAN_E_MEMORY);
  assert(failed==1);
  assert(re_guest_memory_read32(&bus,base+0x100u,&v)==RE_GUEST_MEM_OK&&v==0);
  assert(re_guest_memory_read32(&bus,base+0x13b708u,&v)==RE_GUEST_MEM_OK&&v==base+0x70u);
  free(a);free(b);
 }
 /* Stale original bytes fail before any write. */
 {
  uint8_t*mapped=(uint8_t*)malloc(size);REGUESTMEMORY memory;REGUESTMEMBUS bus;uint32_t v;
  assert(mapped);memcpy(mapped,code,size);put32(mapped+0x100u,0x12345678u);
  re_guest_memory_init(&memory);
  assert(re_guest_memory_map_ex(&memory,base,size,mapped,
    RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC,
    RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
  re_guest_memory_bus_init(&bus,&memory);
  assert(re_guest_xe3u_apply_external_import_plan(&bus,&h,base,plan,2,&failed)==RE_GUEST_XE3U_IMPORT_PLAN_E_STALE);
  assert(failed==0);
  assert(re_guest_memory_read32(&bus,base+0x13b708u,&v)==RE_GUEST_MEM_OK&&v==base+0x70u);
  free(mapped);
 }
 free(code);
 puts("Guest XE3U external import planning/apply tests: PASS");
 return 0;
}
