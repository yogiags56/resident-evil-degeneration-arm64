#include "re_guest_elf32_batch_loader.h"
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
 dyn(b+0x128,17,0x1238);dyn(b+0x130,18,8);dyn(b+0x138,19,8);dyn(b+0x140,0,0);
 memcpy(b+0x180,"\0foo\0",5);
 p32(b+0x1c0+16,1);b[0x1c0+16+12]=0x10;p16(b+0x1c0+16+14,0);
 p32(b+0x220,1);p32(b+0x224,2);p32(b+0x228,1);
 p32(b+0x238,0x2000);p32(b+0x23c,(1u<<8)|RE_ARM_R_ABS32);
 p32(b+0x240,4);
}
int main(void){
 uint8_t elf_a[0x244],elf_b[0x244],code_a[0x240],data_a[8],code_b[0x240],data_b[8];
 REGUESTELF32LOADBUFFER ba[2]={{code_a,sizeof(code_a)},{data_a,sizeof(data_a)}};
 REGUESTELF32LOADBUFFER bb[2]={{code_b,sizeof(code_b)},{data_b,sizeof(data_b)}};
 REGUESTRELOCATION bad={0xDEAD0000u,RE_ARM_R_ABS32,1,0x1234,0,0,0,1};
 REGUESTELF32LOADREQUEST qa={elf_a,sizeof(elf_a),0x100000,ba,2,NULL,0};
 REGUESTELF32LOADREQUEST qb={elf_b,sizeof(elf_b),0x200000,bb,2,NULL,0};
 REGUESTELF32BATCHITEM items[2]={{&qa},{&qb}};
 REGUESTELF32BATCHRESULT result;REGUESTMEMORY memory;
 size_t fm=99,fs=99,fr=99;int inner=0;
 make_elf(elf_a);make_elf(elf_b);re_guest_memory_init(&memory);
 assert(re_guest_elf32_load_batch(&memory,items,2,&result,&fm,&fs,&fr,&inner)==RE_GUEST_ELF32_BATCH_OK);
 assert(result.module_count==2&&result.total_segments==4&&fm==2);
 assert(re_guest_memory_region_count(&memory)==4);
 assert(result.modules[0].entry_point==0x101000&&result.modules[1].entry_point==0x201000);

 /* A failure in the second image restores the complete pre-batch map table. */
 re_guest_memory_init(&memory);qb.relocations=&bad;qb.relocation_count=1;
 memset(&result,0xA5,sizeof(result));
 assert(re_guest_elf32_load_batch(&memory,items,2,&result,&fm,&fs,&fr,&inner)==RE_GUEST_ELF32_BATCH_E_LOAD);
 assert(fm==1&&inner==RE_GUEST_ELF32_LOAD_E_RELOCATION);
 assert(re_guest_memory_region_count(&memory)==0);
 assert(result.module_count==0);
 puts("Guest ELF32 transactional batch loader tests: PASS");
 return 0;
}
