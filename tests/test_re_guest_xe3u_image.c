#include "re_guest_xe3u_image.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void p16(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void p32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static void section(uint8_t*p,uint32_t type,uint32_t size){p32(p,type);p32(p+4,size);}
static void fixture(uint8_t*b){
 memset(b,0,0x200);memcpy(b,"XE3U",4);
 p32(b+4,0x00041901);p16(b+8,2);p16(b+10,5);
 p32(b+12,0x48);p32(b+16,0x4e);
 p32(b+20,0x96);p32(b+24,0x20);p32(b+28,0x30);
 p32(b+40,0);p32(b+52,0x4a000000);
 p32(b+56,0xb6);p32(b+60,0);p32(b+64,8);p32(b+68,0xa0);
 section(b+0x48,0,34);p16(b+0x50,2);
 memcpy(b+0x52,"s3eFileOpen\0s3eFileRead\0",24);
 section(b+0x6a,1,20);p32(b+0x72,2);p32(b+0x76,0x0c);p32(b+0x7a,0x18);
 section(b+0x7e,4,24);p32(b+0x86,2);
 p16(b+0x8a,0);p16(b+0x8c,0x10);p16(b+0x8e,0);
 p16(b+0x90,1);p16(b+0x92,0x20);p16(b+0x94,1);
}
int main(void){
 uint8_t bytes[0x200];REGUESTXE3UHEADER h;REGUESTXE3UFIXUPINFO info,before_info;
 REGUESTXE3USYMBOL symbols[2],before_symbols[2];
 uint32_t internal[2],before_internal[2];
 REGUESTXE3UEXTERNALRELOCATION external[2],before_external[2];
 size_t failed_section=99,failed_record=99;
 fixture(bytes);
 assert(re_guest_xe3u_parse_header(bytes,sizeof(bytes),&h,NULL)==RE_GUEST_XE3U_OK);
 assert(h.version==0x00041901&&h.flags==2&&h.architecture==5);
 assert(h.fixup_offset==0x48&&h.fixup_size==0x4e);
 assert(h.code_offset==0x96&&h.code_file_size==0x20&&h.code_memory_size==0x30);
 assert(h.base_address==0x4a000000&&h.has_data_offset&&h.data_offset==0xa0);
 assert(re_guest_xe3u_parse_fixups(bytes,sizeof(bytes),&h,symbols,2,
   internal,2,external,2,&info,&failed_section,&failed_record)==RE_GUEST_XE3U_OK);
 assert(info.section_count==3&&info.symbol_count==2);
 assert(info.internal_relocation_count==2&&info.external_relocation_count==2);
 assert(!strcmp(symbols[0].name,"s3eFileOpen")&&!strcmp(symbols[1].name,"s3eFileRead"));
 assert(internal[0]==0x0c&&internal[1]==0x18);
 assert(external[0].section_type==4&&external[0].offset==0x10&&external[0].symbol_index==0);
 assert(external[1].section_type==4&&external[1].offset==0x10020&&external[1].symbol_index==1);
 memcpy(before_symbols,symbols,sizeof(symbols));memcpy(before_internal,internal,sizeof(internal));
 memcpy(before_external,external,sizeof(external));before_info=info;
 p16(bytes+0x94,7);
 assert(re_guest_xe3u_parse_fixups(bytes,sizeof(bytes),&h,symbols,2,
   internal,2,external,2,&info,&failed_section,&failed_record)==RE_GUEST_XE3U_E_SYMBOL);
 assert(failed_section==2&&failed_record==1);
 assert(!memcmp(symbols,before_symbols,sizeof(symbols))&&!memcmp(internal,before_internal,sizeof(internal)));
 assert(!memcmp(external,before_external,sizeof(external))&&!memcmp(&info,&before_info,sizeof(info)));
 fixture(bytes);assert(re_guest_xe3u_parse_header(bytes,sizeof(bytes),&h,NULL)==RE_GUEST_XE3U_OK);
 assert(re_guest_xe3u_parse_fixups(bytes,sizeof(bytes),&h,symbols,1,
   internal,2,external,2,&info,NULL,NULL)==RE_GUEST_XE3U_E_CAPACITY);
 fixture(bytes);p32(bytes+16,0xffffffffu);
 assert(re_guest_xe3u_parse_header(bytes,sizeof(bytes),&h,NULL)==RE_GUEST_XE3U_E_RANGE);
 puts("Guest XE3U header and fixup parser tests: PASS");
 return 0;
}
