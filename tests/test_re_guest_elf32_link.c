#include "re_guest_elf32_link.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void p16(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void p32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static void ph(uint8_t*p,uint32_t t,uint32_t o,uint32_t a,uint32_t fs,uint32_t ms,uint32_t f,uint32_t al){
 p32(p,t);p32(p+4,o);p32(p+8,a);p32(p+12,a);p32(p+16,fs);p32(p+20,ms);p32(p+24,f);p32(p+28,al);
}
static void dyn(uint8_t*p,uint32_t t,uint32_t v){p32(p,t);p32(p+4,v);}
static void fixture(uint8_t*b){
 memset(b,0,768);b[0]=0x7f;b[1]='E';b[2]='L';b[3]='F';b[4]=1;b[5]=1;b[6]=1;
 p16(b+16,RE_GUEST_ELF32_ET_EXEC);p16(b+18,RE_GUEST_ELF32_EM_ARM);p32(b+20,1);
 p32(b+24,0x1000);p32(b+28,52);p16(b+40,52);p16(b+42,32);p16(b+44,2);
 ph(b+52,RE_GUEST_ELF32_PT_LOAD,0,0x1000,768,768,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X,0x1000);
 ph(b+84,RE_GUEST_ELF32_PT_DYNAMIC,0x100,0x1100,72,72,RE_GUEST_ELF32_PF_R,4);
 dyn(b+0x100,4,0x1200); /* DT_HASH */
 dyn(b+0x108,5,0x1180); /* DT_STRTAB */
 dyn(b+0x110,10,16);    /* DT_STRSZ */
 dyn(b+0x118,6,0x11c0); /* DT_SYMTAB */
 dyn(b+0x120,11,16);    /* DT_SYMENT */
 dyn(b+0x128,17,0x1220);/* DT_REL */
 dyn(b+0x130,18,8);     /* DT_RELSZ */
 dyn(b+0x138,19,8);     /* DT_RELENT */
 dyn(b+0x140,0,0);
 memcpy(b+0x180,"\0foo\0bar\0",9);
 /* Two named symbols after the mandatory null symbol. */
 p32(b+0x1c0+16,1);p32(b+0x1c0+20,0x1000);p32(b+0x1c0+24,4);
 b[0x1c0+16+12]=0x10;p16(b+0x1c0+16+14,1);
 p32(b+0x1c0+32,5);p32(b+0x1c0+36,0x2000);p32(b+0x1c0+40,8);
 b[0x1c0+32+12]=0x10;p16(b+0x1c0+32+14,1);
 p32(b+0x200,1);p32(b+0x204,3);p32(b+0x208,1);p32(b+0x20c,0);p32(b+0x210,0);
 p32(b+0x220,0x1300);p32(b+0x224,(1u<<8)|2u);
}
int main(void){
 uint8_t b[768];REGUESTELF32IMAGE im;REGUESTELF32DYNAMIC d;
 REGUESTELF32LINKINFO info;REGUESTELF32LINKSYMBOL syms[4];
 REGUESTELF32LINKRELOCATION rels[4];size_t failed=999;
 fixture(b);
 assert(re_guest_elf32_parse_image(b,sizeof b,&im,&failed)==RE_GUEST_ELF32_OK);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&im,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_OK);
 assert(re_guest_elf32_parse_link_metadata(b,sizeof b,&im,&d,&info,syms,4,rels,4,&failed)==RE_GUEST_ELF32_LINK_OK);
 assert(info.has_sysv_hash&&info.has_symbols&&info.symbol_count==3);
 assert(syms[0].name[0]=='\0'); /* ELF null symbol has an empty name. */
 assert(strcmp(syms[1].name,"foo")==0&&syms[1].value==0x1000&&syms[1].binding==1);
 assert(strcmp(syms[2].name,"bar")==0&&syms[2].size==8);
 assert(info.has_rel&&info.relocation_count==1&&rels[0].offset==0x1300);
 assert(rels[0].symbol_index==1&&rels[0].type==2&&!rels[0].has_addend);
 assert(re_guest_elf32_parse_link_metadata(b,sizeof b,&im,&d,&info,syms,2,rels,4,&failed)==RE_GUEST_ELF32_LINK_E_CAPACITY);
 fixture(b);p32(b+0x11c,0xfffffff0u);
 assert(re_guest_elf32_parse_image(b,sizeof b,&im,&failed)==RE_GUEST_ELF32_OK);
 assert(re_guest_elf32_parse_dynamic(b,sizeof b,&im,&d,&failed)==RE_GUEST_ELF32_DYNAMIC_OK);
 assert(re_guest_elf32_parse_link_metadata(b,sizeof b,&im,&d,&info,syms,4,rels,4,&failed)==RE_GUEST_ELF32_LINK_E_RANGE);
 puts("Guest ELF32 link metadata tests: PASS");return 0;
}
