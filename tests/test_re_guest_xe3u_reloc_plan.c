#include "re_guest_xe3u_reloc_plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put32(uint8_t *p,uint32_t v) {
 p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
int main(void) {
 uint8_t code[0x100]={0};
 REGUESTXE3UHEADER h={0};
 uint32_t offsets[3]={0x80,0x10,0x24};
 uint32_t scratch[8];
 REGUESTXE3URELOCATION out[3],before[3];
 size_t failed=99;
 h.base_address=0x4a000000u;h.code_file_size=sizeof(code);
 h.code_memory_size=0x200u;
 put32(code+0x80,h.base_address+0x181u); /* Thumb pointer into BSS tail */
 put32(code+0x10,h.base_address+0x20u);  /* ARM pointer into code */
 put32(code+0x24,h.base_address+0x101u); /* Thumb pointer into initialized data */
 memset(out,0xa5,sizeof(out));
 assert(re_guest_xe3u_prepare_internal_relocations(code,sizeof(code),&h,
   offsets,3,0x50000000u,scratch,8,out,3,&failed)==RE_GUEST_XE3U_RELOC_PLAN_OK);
 assert(failed==3);
 assert(out[0].patch_offset==0x80&&out[0].original_value==0x4a000181u);
 assert(out[0].relocated_value==0x50000181u&&out[0].thumb_state==1);
 assert(out[1].relocated_value==0x50000020u&&out[1].thumb_state==0);
 assert(out[2].relocated_value==0x50000101u&&out[2].thumb_state==1);
 memcpy(before,out,sizeof(out));

 /* Negative load bias preserves Thumb state. */
 assert(re_guest_xe3u_prepare_internal_relocations(code,sizeof(code),&h,
   offsets,3,0x40000000u,scratch,8,out,3,&failed)==RE_GUEST_XE3U_RELOC_PLAN_OK);
 assert(out[0].relocated_value==0x40000181u);
 assert(out[1].relocated_value==0x40000020u);
 assert(out[2].relocated_value==0x40000101u);

 /* Every failure preserves the previously published plan. */
 memcpy(out,before,sizeof(out));
 offsets[1]=offsets[0];
 assert(re_guest_xe3u_prepare_internal_relocations(code,sizeof(code),&h,
   offsets,3,0x50000000u,scratch,8,out,3,&failed)==RE_GUEST_XE3U_RELOC_PLAN_E_DUPLICATE);
 assert(failed==1&&memcmp(out,before,sizeof(out))==0);
 offsets[1]=0x10;offsets[2]=0x101;
 assert(re_guest_xe3u_prepare_internal_relocations(code,sizeof(code),&h,
   offsets,3,0x50000000u,scratch,8,out,3,&failed)==RE_GUEST_XE3U_RELOC_PLAN_E_PATCH_RANGE);
 assert(failed==2&&memcmp(out,before,sizeof(out))==0);
 offsets[2]=0x24;put32(code+0x24,0x60000001u);
 assert(re_guest_xe3u_prepare_internal_relocations(code,sizeof(code),&h,
   offsets,3,0x50000000u,scratch,8,out,3,&failed)==RE_GUEST_XE3U_RELOC_PLAN_E_TARGET_RANGE);
 assert(failed==2&&memcmp(out,before,sizeof(out))==0);
 put32(code+0x24,h.base_address+0x101u);
 assert(re_guest_xe3u_prepare_internal_relocations(code,sizeof(code),&h,
   offsets,3,0x50000000u,scratch,5,out,3,&failed)==RE_GUEST_XE3U_RELOC_PLAN_E_CAPACITY);
 assert(memcmp(out,before,sizeof(out))==0);
 assert(re_guest_xe3u_prepare_internal_relocations(code,sizeof(code),&h,
   offsets,3,0x50000004u,scratch,8,out,3,&failed)==RE_GUEST_XE3U_RELOC_PLAN_E_ALIGNMENT);
 assert(memcmp(out,before,sizeof(out))==0);
 puts("Guest XE3U internal relocation planning tests: PASS");
 return 0;
}
