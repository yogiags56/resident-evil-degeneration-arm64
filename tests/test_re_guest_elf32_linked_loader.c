#include "re_guest_elf32_linked_loader.h"
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
static void make_elf(uint8_t*b){
 memset(b,0,0x244);b[0]=0x7f;b[1]='E';b[2]='L';b[3]='F';b[4]=1;b[5]=1;b[6]=1;
 p16(b+16,RE_GUEST_ELF32_ET_DYN);p16(b+18,RE_GUEST_ELF32_EM_ARM);p32(b+20,1);
 p32(b+24,0x1000);p32(b+28,52);p16(b+40,52);p16(b+42,32);p16(b+44,3);
 ph(b+52,RE_GUEST_ELF32_PT_LOAD,0,0x1000,0x240,0x240,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X,0x1000);
 ph(b+84,RE_GUEST_ELF32_PT_LOAD,0x240,0x2000,4,8,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W,4);
 ph(b+116,RE_GUEST_ELF32_PT_DYNAMIC,0x100,0x1100,72,72,RE_GUEST_ELF32_PF_R,4);
 dyn(b+0x100,4,0x1220);dyn(b+0x108,5,0x1180);dyn(b+0x110,10,16);
 dyn(b+0x118,6,0x11c0);dyn(b+0x120,11,16);
 dyn(b+0x128,17,0x1238);dyn(b+0x130,18,8);dyn(b+0x138,19,8);
 dyn(b+0x140,0,0);
 memcpy(b+0x180,"\0foo\0",5);
 p32(b+0x1c0+16,1);b[0x1c0+16+12]=0x10;p16(b+0x1c0+16+14,0);
 p32(b+0x220,1);p32(b+0x224,2);p32(b+0x228,1);
 p32(b+0x238,0x2000);p32(b+0x23c,(1u<<8)|RE_ARM_R_ABS32);
 p32(b+0x240,4);
}
int main(void){
 uint8_t elf[0x244],code[0x240]={0},data[8]={0};
 REGUESTELF32LOADBUFFER buffers[2]={{code,sizeof(code)},{data,sizeof(data)}};
 REGUESTELF32LINKSYMBOL symbols[4];
 REGUESTELF32LINKRELOCATION decoded[4];
 REGUESTRELOCATION prepared[4];
 REGUESTELF32LINKWORKSPACE ws={symbols,4,decoded,4,prepared,4};
 REGUESTSYMBOL providers[1]={{"foo",0x105000,RE_GUEST_STB_GLOBAL,
     RE_GUEST_STV_DEFAULT,1,NULL,NULL}};
 REGUESTELF32LINKEDLOADRESULT result;REGUESTMEMORY memory;
 size_t failed=99;int inner=0;void *ptr=NULL;
 make_elf(elf);re_guest_memory_init(&memory);
 assert(re_guest_elf32_load_linked_image(&memory,elf,sizeof(elf),0x100000,
     buffers,2,providers,1,&ws,&result,&failed,&inner)==RE_GUEST_ELF32_LINKED_LOAD_OK);
 assert(result.load.entry_valid&&result.load.entry_point==0x101000);
 assert(result.link.has_symbols&&result.link.symbol_count==2);
 assert(result.link.relocation_count==1&&result.dynamic_dependency_count==0);
 assert(data[0]==0x04&&data[1]==0x50&&data[2]==0x10&&data[3]==0x00);
 assert(data[4]==0&&data[5]==0&&data[6]==0&&data[7]==0);
 assert(re_guest_memory_region_count(&memory)==2);
 assert(re_guest_memory_resolve(&memory,0x101000,4,RE_GUEST_MEM_EXEC,&ptr)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_resolve(&memory,0x101000,4,RE_GUEST_MEM_WRITE,&ptr)==RE_GUEST_MEM_E_PERMISSION);
 assert(re_guest_memory_resolve(&memory,0x102000,4,RE_GUEST_MEM_WRITE,&ptr)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_unmap(&memory,0x101000)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_unmap(&memory,0x102000)==RE_GUEST_MEM_OK);

 memset(code,0,sizeof(code));memset(data,0,sizeof(data));make_elf(elf);re_guest_memory_init(&memory);
 assert(re_guest_elf32_load_linked_image(&memory,elf,sizeof(elf),0x100000,
     buffers,2,NULL,0,&ws,&result,&failed,&inner)==RE_GUEST_ELF32_LINKED_LOAD_E_RELOCATION_PLAN);
 assert(inner==RE_GUEST_ELF32_RELOC_PLAN_E_UNRESOLVED);
 assert(re_guest_memory_region_count(&memory)==0);
 puts("Guest ELF32 linked-load coordinator tests: PASS");return 0;
}
