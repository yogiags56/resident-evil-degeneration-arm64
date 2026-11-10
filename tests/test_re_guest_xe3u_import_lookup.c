#include "re_guest_xe3u_import_lookup.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put32(uint8_t *p,uint32_t v) {
 p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
int main(void) {
 const size_t code_size=0x13b714u;
 uint8_t *code=(uint8_t *)calloc(code_size,1);
 REGUESTXE3UHEADER h={0};
 REGUESTXE3USYMBOL symbols[1]={{"sampleApi"}};
 REGUESTXE3UEXTERNALRELOCATION ext[2]={{0x13b708u,0,4u},{0,0,0}};
 REGUESTXE3UIMPORTCELL result,before;
 size_t failed=99;
 assert(code);
 h.base_address=0x4a000000u;h.code_file_size=(uint32_t)code_size;
 h.code_memory_size=(uint32_t)code_size+0x1000u;
 put32(code+0x13b708u,h.base_address+0x70u);
 put32(code+0x84u,0xe28fc601u);
 put32(code+0x88u,0xe28cca3bu);
 put32(code+0x8cu,0xe5bcf67cu);
 memset(&result,0xa5,sizeof(result));
 assert(re_guest_xe3u_lookup_import_cell(code,code_size,&h,symbols,1,
   ext,1,0x50000000u,0x50000000u+0x13b708u,&result,&failed)==
   RE_GUEST_XE3U_IMPORT_LOOKUP_OK);
 assert(failed==0&&result.cell_offset==0x13b708u);
 assert(result.stub_offset==0x84u&&result.symbol_index==0u&&result.section_type==4u);
 before=result;
 assert(re_guest_xe3u_lookup_import_cell(code,code_size,&h,symbols,1,
   ext,1,0x50000000u,0x50000000u+0x13b70cu,&result,&failed)==
   RE_GUEST_XE3U_IMPORT_LOOKUP_E_NOT_FOUND);
 assert(memcmp(&result,&before,sizeof(result))==0);
 ext[1]=ext[0];
 assert(re_guest_xe3u_lookup_import_cell(code,code_size,&h,symbols,1,
   ext,2,0x50000000u,0x50000000u+0x13b708u,&result,&failed)==
   RE_GUEST_XE3U_IMPORT_LOOKUP_E_DUPLICATE);
 ext[1]=(REGUESTXE3UEXTERNALRELOCATION){0x13b708u,0,3u};
 assert(re_guest_xe3u_lookup_import_cell(code,code_size,&h,symbols,1,
   ext,1,0x50000000u,0x50000000u+0x13b708u,&result,&failed)==
   RE_GUEST_XE3U_IMPORT_LOOKUP_E_UNSUPPORTED);
 ext[0]=(REGUESTXE3UEXTERNALRELOCATION){0xc7c4cu,0,4u};
 assert(re_guest_xe3u_lookup_import_cell(code,code_size,&h,symbols,1,
   ext,1,0x50000000u,0x50000000u+0xc7c4cu,&result,&failed)==
   RE_GUEST_XE3U_IMPORT_LOOKUP_E_STUB);
 ext[0]=(REGUESTXE3UEXTERNALRELOCATION){0x13b708u,0,4u};
 put32(code+0x13b708u,0);
 assert(re_guest_xe3u_lookup_import_cell(code,code_size,&h,symbols,1,
   ext,1,0x50000000u,0x50000000u+0x13b708u,&result,&failed)==
   RE_GUEST_XE3U_IMPORT_LOOKUP_E_STUB);
 assert(memcmp(&result,&before,sizeof(result))==0);
 free(code);
 puts("Guest XE3U import cell lookup tests: PASS");
 return 0;
}
