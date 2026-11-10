#include "re_guest_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put32(unsigned char*p,unsigned v){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}
static void put16(unsigned char*p,unsigned v){p[0]=v;p[1]=v>>8;}
int main(void){
 unsigned char mem[128]={0};REGUESTMEM m={mem,sizeof(mem),0x50000000};REGUESTCPU c;
 /* A32: MOV r0,#7; ADD r1,r0,#5; CMP r1,#12; B +0 */
 put32(mem,0xe3a00007);put32(mem+4,0xe2801005);put32(mem+8,0xe351000c);put32(mem+12,0xea000000);
 re_guest_cpu_reset(&c,0x50000000,0);
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK && c.r[0]==7);
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK && c.r[1]==12);
 /* CMP immediate is outside this subset's immediate decoder: expect explicit unsupported */
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK && (c.cpsr&RE_CPSR_Z));
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK && c.r[15]==0x50000014);
 /* Thumb: MOVS r0,#9; ADDS r0,#3; unconditional branch */
 memset(mem,0,sizeof(mem));put16(mem,0x2009);put16(mem+2,0x3003);put16(mem+4,0xe000);
 re_guest_cpu_reset(&c,0x50000000,1);
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK && c.r[0]==9);
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK && c.r[0]==12);
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK && c.r[15]==0x50000008);
 /* ARM import-thunk transfer: LDR pc,[r12,#4]! updates r12 and branches. */
 memset(mem,0,sizeof(mem));
 put32(mem,0xe5bcf004);put32(mem+0x40,0x50000060);put32(mem+0x60,0xe3a00001);
 re_guest_cpu_reset(&c,0x50000000,0);c.r[12]=0x5000003c;
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK);
 assert(c.r[12]==0x50000040&&c.r[15]==0x50000060&&!c.thumb);
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_OK&&c.r[0]==1);
 /* Failed loads do not commit the base-register writeback. */
 memset(mem,0,sizeof(mem));put32(mem,0xe5bcf004);
 re_guest_cpu_reset(&c,0x50000000,0);c.r[12]=0x5000007c;
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_E_MEMORY&&c.r[12]==0x5000007c);
 re_guest_cpu_reset(&c,0x50000000,0);c.r[12]=0x5000003d;
 assert(re_guest_cpu_step(&c,&m)==RE_CPU_E_ALIGNMENT&&c.r[12]==0x5000003d);
 /* Memory fault and alignment fault are distinguished */
 re_guest_cpu_reset(&c,0x5000007c,0);c.r[15]=0x5000007e;assert(re_guest_cpu_step(&c,&m)==RE_CPU_E_ALIGNMENT);
 re_guest_cpu_reset(&c,0x4fffffff,0);assert(re_guest_cpu_step(&c,&m)==RE_CPU_E_MEMORY);
 puts("Stage60 guest CPU tests: PASS");return 0;
}
