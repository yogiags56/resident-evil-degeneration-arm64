#include "re_guest_cpu.h"
#include "re_guest_memory_bus.h"

static int map_bus_status(const REGUESTMEM *m, REGUESTMEMStatus s) {
 if(m && m->bus_fault)*m->bus_fault=(int)s;
 if(s==RE_GUEST_MEM_OK)return 1;
 if(s==RE_GUEST_MEM_E_PERMISSION)return -2;
 if(s==RE_GUEST_MEM_E_RANGE)return -1;
 return 0;
}
static int range(const REGUESTMEM*m,uint32_t a,size_t n,size_t*o){
 if(!m||!m->bytes||a<m->guest_base)return 0;
 *o=(size_t)(a-m->guest_base);
 return (uint64_t)*o+n<=m->size;
}
static int rd8(const REGUESTMEM*m,uint32_t a,uint8_t*v){
 size_t o;if(m&&m->bus)return map_bus_status(m,re_guest_memory_read8(m->bus,a,v));
 if(!range(m,a,1,&o))return 0;*v=m->bytes[o];return 1;
}
static int wr8(const REGUESTMEM*m,uint32_t a,uint8_t v){
 size_t o;if(m&&m->bus)return map_bus_status(m,re_guest_memory_write8(m->bus,a,v));
 if(!range(m,a,1,&o))return 0;m->bytes[o]=v;return 1;
}
static int read16(const REGUESTMEM*m,uint32_t a,uint16_t*v){
 size_t o;if(a&1)return -1;
 if(m&&m->bus)return map_bus_status(m,re_guest_memory_read16(m->bus,a,v));
 if(!range(m,a,2,&o))return 0;*v=(uint16_t)(m->bytes[o]|((uint16_t)m->bytes[o+1]<<8));return 1;
}
static int write16(const REGUESTMEM*m,uint32_t a,uint16_t v){
 size_t o;if(a&1)return -1;
 if(m&&m->bus)return map_bus_status(m,re_guest_memory_write16(m->bus,a,v));
 if(!range(m,a,2,&o))return 0;m->bytes[o]=(uint8_t)v;m->bytes[o+1]=(uint8_t)(v>>8);return 1;
}
static int read32(const REGUESTMEM*m,uint32_t a,uint32_t*v){
 size_t o;if(a&3)return -1;
 if(m&&m->bus)return map_bus_status(m,re_guest_memory_read32(m->bus,a,v));
 if(!range(m,a,4,&o))return 0;*v=(uint32_t)m->bytes[o]|((uint32_t)m->bytes[o+1]<<8)|((uint32_t)m->bytes[o+2]<<16)|((uint32_t)m->bytes[o+3]<<24);return 1;
}
static int write32(const REGUESTMEM*m,uint32_t a,uint32_t v){
 size_t o;if(a&3)return -1;
 if(m&&m->bus)return map_bus_status(m,re_guest_memory_write32(m->bus,a,v));
 if(!range(m,a,4,&o))return 0;m->bytes[o]=(uint8_t)v;m->bytes[o+1]=(uint8_t)(v>>8);m->bytes[o+2]=(uint8_t)(v>>16);m->bytes[o+3]=(uint8_t)(v>>24);return 1;
}
static int fetch16(const REGUESTMEM*m,uint32_t a,uint16_t*v){
 if(a&1u)return -1;
 if(m&&m->bus)return map_bus_status(m,re_guest_memory_fetch16(m->bus,a,v));
 return read16(m,a,v);
}
static int fetch32(const REGUESTMEM*m,uint32_t a,uint32_t*v){
 if(a&3u)return -1;
 if(m&&m->bus)return map_bus_status(m,re_guest_memory_fetch32(m->bus,a,v));
 return read32(m,a,v);
}
static uint32_t ror(uint32_t x,unsigned n){n&=31;return n?(x>>n)|(x<<(32-n)):x;}
static int cond(uint32_t f,unsigned c){int n=!!(f&RE_CPSR_N),z=!!(f&RE_CPSR_Z),v=!!(f&RE_CPSR_V),cy=!!(f&RE_CPSR_C);switch(c){case 0:return z;case 1:return !z;case 2:return cy;case 3:return !cy;case 4:return n;case 5:return !n;case 6:return v;case 7:return !v;case 8:return cy&&!z;case 9:return !cy||z;case 10:return n==v;case 11:return n!=v;case 12:return !z&&n==v;case 13:return z||n!=v;case 14:return 1;default:return 0;}}
static void set_nz(REGUESTCPU*c,uint32_t x){c->cpsr=(c->cpsr&~(RE_CPSR_N|RE_CPSR_Z))|(x&RE_CPSR_N)|(x?0:RE_CPSR_Z);}
static void set_add(REGUESTCPU*c,uint32_t a,uint32_t b,uint32_t r){set_nz(c,r);c->cpsr=(c->cpsr&~(RE_CPSR_C|RE_CPSR_V))|((uint64_t)a+b>0xffffffffu?RE_CPSR_C:0)|((~(a^b)&(a^r)&0x80000000u)?RE_CPSR_V:0);}
static void set_sub(REGUESTCPU*c,uint32_t a,uint32_t b,uint32_t r){set_nz(c,r);c->cpsr=(c->cpsr&~(RE_CPSR_C|RE_CPSR_V))|(a>=b?RE_CPSR_C:0)|(((a^b)&(a^r)&0x80000000u)?RE_CPSR_V:0);}
static uint32_t rv(REGUESTCPU*c,unsigned r,uint32_t pc){return r==15?pc+8:c->r[r];}
static uint32_t shift_imm(uint32_t v,unsigned type,unsigned n,uint32_t*carry,uint32_t oldc){
 if(type==0){if(!n)return v;*carry=(v>>(32-n))&1;return v<<n;}
 if(type==1){if(!n){*carry=v>>31;return 0;}*carry=(v>>(n-1))&1;return v>>n;}
 if(type==2){if(!n){*carry=v>>31;return (uint32_t)((int32_t)v>>31);}*carry=(v>>(n-1))&1;return (uint32_t)((int32_t)v>>n);}
 if(!n){*carry=v&1;return (oldc<<31)|(v>>1);}*carry=(v>>(n-1))&1;return ror(v,n);
}
static REGUESTCPUStatus arm(REGUESTCPU*c,const REGUESTMEM*m,uint32_t pc){
 uint32_t x,a,b,r,addr,carry=(c->cpsr&RE_CPSR_C)!=0;unsigned op,rn,rd,rm,rs,sh,rot,count;int q=fetch32(m,pc,&x);
 if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;
 c->r[15]=pc+4;
 /* BLX (register), including ARM/Thumb interworking. */
 if((x&0x0ffffff0u)==0x012fff30u){a=rv(c,x&15,pc);c->r[14]=pc+4;c->thumb=a&1;c->r[15]=a&(c->thumb?~1u:~3u);return RE_CPU_OK;}
 if(!cond(c->cpsr,x>>28))return RE_CPU_OK;
 if((x&0x0ffffff0u)==0x012fff10u){a=rv(c,x&15,pc);c->thumb=a&1;c->r[15]=a&(c->thumb?~1u:~3u);return RE_CPU_OK;}
 if((x&0x0e000000u)==0x0a000000u){int32_t off=(int32_t)(x&0xffffffu);if(off&0x800000)off|=(int32_t)0xff000000;if(x&(1u<<24))c->r[14]=pc+4;c->r[15]=pc+8+(uint32_t)(off*4);return RE_CPU_OK;}
 /* Single data transfer: immediate offset, pre-index, byte/word, no writeback. */
 if((x&0x0c000000u)==0x04000000u && !(x&(1u<<25)) && (x&(1u<<24)) && !(x&(1u<<21))){
  rn=(x>>16)&15;rd=(x>>12)&15;a=rv(c,rn,pc);addr=(x&(1u<<23))?a+(x&0xfff):a-(x&0xfff);
  if(x&(1u<<20)){if(x&(1u<<22)){uint8_t v;if(!rd8(m,addr,&v))return RE_CPU_E_MEMORY;c->r[rd]=v;}else{q=read32(m,addr,&r);if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;
 c->r[rd]=r;}if(rd==15)c->r[15]&=~3u;}
  else {if(x&(1u<<22)){if(!wr8(m,addr,(uint8_t)c->r[rd]))return RE_CPU_E_MEMORY;}else{q=write32(m,addr,c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;}}
  return RE_CPU_OK;
 }
 /* LDM/STM including PUSH (STMDB sp!) and POP (LDMIA sp!). */
 if((x&0x0e000000u)==0x08000000u){
  int pre=!!(x&(1u<<24)),up=!!(x&(1u<<23)),wb=!!(x&(1u<<21)),load=!!(x&(1u<<20));
  rn=(x>>16)&15;uint16_t list=(uint16_t)x;count=0;for(unsigned i=0;i<16;i++)if(list&(1u<<i))count++;
  if(!count)return RE_CPU_E_UNIMPLEMENTED;
  a=rv(c,rn,pc);addr=up?(a+(pre?4u:0u)):(a-(count*4u)+(pre?0u:4u));
  for(unsigned i=0;i<16;i++)if(list&(1u<<i)){
   if(load){q=read32(m,addr,&r);if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;
   if(i==15){c->r[15]=r&~3u;}else c->r[i]=r;}
   else {q=write32(m,addr,i==15?pc+12:c->r[i]);if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;}
   addr+=4;
  }
  if(wb)c->r[rn]=up?a+count*4u:a-count*4u;
  return RE_CPU_OK;
 }
 /* Data processing, immediate and register Operand2 (including register shifts). */
 if((x&0x0c000000u)==0){
  int immediate=!!(x&(1u<<25));op=(x>>21)&15;rn=(x>>16)&15;rd=(x>>12)&15;a=rv(c,rn,pc);
  if(immediate){rot=((x>>8)&15)*2;b=ror(x&255,rot);if(rot)carry=b>>31;}
  else{
   rm=x&15;b=rv(c,rm,pc);sh=(x>>5)&3;
   if(x&(1u<<4)){
    if(x&(1u<<7))return RE_CPU_E_UNIMPLEMENTED;
    rs=(x>>8)&15;unsigned n=rv(c,rs,pc)&255;
    if(!n){}else if(sh==0){if(n<32){carry=(b>>(32-n))&1;b<<=n;}else{carry=n==32?b&1:0;b=0;}}
    else if(sh==1){if(n<32){carry=(b>>(n-1))&1;b>>=n;}else{carry=n==32?b>>31:0;b=0;}}
    else if(sh==2){if(n<32){carry=(b>>(n-1))&1;b=(uint32_t)((int32_t)b>>n);}else{carry=b>>31;b=(uint32_t)((int32_t)b>>31);}}
    else {n&=31;if(n){b=ror(b,n);carry=b>>31;}}
   }else{b=shift_imm(b,sh,(x>>7)&31,&carry,(c->cpsr&RE_CPSR_C)!=0);}
  }
  int set=!!(x&(1u<<20));
  switch(op){
   case 0:r=a&b;break;case 1:r=a^b;break;case 2:r=a-b;break;case 3:r=b-a;break;
   case 4:r=a+b;break;case 5:r=a+b+((c->cpsr&RE_CPSR_C)!=0);break;
   case 6:r=a-b-((c->cpsr&RE_CPSR_C)?0u:1u);break;case 7:r=b-a-((c->cpsr&RE_CPSR_C)?0u:1u);break;
   case 8:r=a&b;set=1;break;case 9:r=a^b;set=1;break;case 10:r=a-b;set=1;break;case 11:r=a+b;set=1;break;
   case 12:r=a|b;break;case 13:r=b;break;case 14:r=a&~b;break;case 15:r=~b;break;default:return RE_CPU_E_UNIMPLEMENTED;
  }
  if(set){if(op==2||op==6||op==10)set_sub(c,a,b,r);else if(op==3||op==7)set_sub(c,b,a,r);else if(op==4||op==5||op==11)set_add(c,a,b,r);else{set_nz(c,r);c->cpsr=(c->cpsr&~RE_CPSR_C)|(carry?RE_CPSR_C:0);}}
  if(op==8||op==9||op==10||op==11)return RE_CPU_OK;
  if(rd==15){c->r[15]=r&~3u;}else c->r[rd]=r;
  return RE_CPU_OK;
 }
 return RE_CPU_E_UNIMPLEMENTED;
}
/* CPSR stores IT[1:0] in bits 26:25 and IT[7:2] in bits 15:10. */
#define RE_CPSR_IT_MASK 0x0600fc00u
static void it_sync(REGUESTCPU*c){
 uint32_t v=((uint32_t)(c->itstate&3u)<<25)|((uint32_t)(c->itstate&0xfcu)<<8);
 c->cpsr=(c->cpsr&~RE_CPSR_IT_MASK)|v;
}
static void it_set(REGUESTCPU*c,uint8_t v){c->itstate=v;it_sync(c);}
static void it_advance(REGUESTCPU*c){
 uint8_t v=c->itstate;
 if((v&7u)==0u)v=0;
 else v=(uint8_t)((v&0xe0u)|((v<<1)&0x1fu));
 it_set(c,v);
}

static REGUESTCPUStatus thumb(REGUESTCPU*c,const REGUESTMEM*m,uint32_t pc){
 uint16_t x;uint32_t a,b,r,addr;unsigned rd,rs,rm,imm,op;int q=fetch16(m,pc,&x);
 if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;
 c->r[15]=pc+2;
 /* Thumb-2 conditional wide B.cond (T3). */
 if((x&0xf800u)==0xf000u){
  uint16_t y; q=fetch16(m,pc+2,&y); if(q<0)return RE_CPU_E_ALIGNMENT; if(!q)return RE_CPU_E_MEMORY;
  if((y&0xd000u)==0x8000u){
   unsigned cc=(x>>6)&15u;
   if(cc>=14u)return RE_CPU_E_UNIMPLEMENTED;
   unsigned sbit=(x>>10)&1u,j1=(y>>13)&1u,j2=(y>>11)&1u;
   uint32_t u=(sbit<<20)|(j2<<19)|(j1<<18)|((uint32_t)(x&0x003fu)<<12)|((uint32_t)(y&0x07ffu)<<1);
   int32_t off=(int32_t)(u<<11)>>11;
   if(cond(c->cpsr,cc))c->r[15]=pc+4u+(uint32_t)off;
   else c->r[15]=pc+4u;
   return RE_CPU_OK;
  }
  /* Thumb BLX immediate (T2), switch to ARM state and word-align target. */
  if((y&0xd001u)==0xc000u){
   unsigned sbit=(x>>10)&1u,j1=(y>>13)&1u,j2=(y>>11)&1u;
   unsigned i1=(~(j1^sbit))&1u,i2=(~(j2^sbit))&1u;
   uint32_t u=(sbit<<24)|(i1<<23)|(i2<<22)|((uint32_t)(x&0x03ffu)<<12)|((uint32_t)(y&0x07feu)<<1);
   int32_t off=(int32_t)(u<<7)>>7;
   c->r[14]=(pc+4u)|1u;
   c->thumb=0;
   c->r[15]=(((pc+4u)&~3u)+(uint32_t)off)&~3u;
   return RE_CPU_OK;
  }
 }
 /* Thumb-2 wide B.W and BL immediate branches (T4 encodings). */
 if((x&0xf800u)==0xf000u){
  uint16_t y; q=read16(m,pc+2,&y); if(q<0)return RE_CPU_E_ALIGNMENT; if(!q)return RE_CPU_E_MEMORY;
  unsigned sbit=(x>>10)&1u, j1=(y>>13)&1u, j2=(y>>11)&1u;
  unsigned is1=(~(j1^sbit))&1u, is2=(~(j2^sbit))&1u;
  if((y&0xd000u)==0x9000u || (y&0xd000u)==0xd000u){
   uint32_t u=(sbit<<24)|(is1<<23)|(is2<<22)|((uint32_t)(x&0x03ffu)<<12)|((uint32_t)(y&0x07ffu)<<1);
   int32_t off=(int32_t)(u<<7)>>7;
   if((y&0xd000u)==0xd000u)c->r[14]=(pc+4u)|1u;
   c->r[15]=pc+4u+(uint32_t)off; return RE_CPU_OK;
  }
 }
 /* Thumb-2 immediate wide loads/stores: F8C/F8D word, F880/F890 byte. */
 if((x&0xfff0u)==0xf8c0u || (x&0xfff0u)==0xf8d0u ||
    (x&0xfff0u)==0xf880u || (x&0xfff0u)==0xf890u){
  uint16_t y; q=read16(m,pc+2,&y); if(q<0)return RE_CPU_E_ALIGNMENT; if(!q)return RE_CPU_E_MEMORY;
  unsigned rn=x&15u; rd=(y>>12)&15u; imm=y&0x0fffu;
  addr=(rn==15u?((pc+4u)&~3u):c->r[rn])+imm;
  if((x&0xfff0u)==0xf8d0u){q=read32(m,addr,&r);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;if(rd==15u)c->r[15]=r&~1u;else c->r[rd]=r;}
  else if((x&0xfff0u)==0xf8c0u){q=write32(m,addr,c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;}
  else if((x&0xfff0u)==0xf890u){uint8_t v;if(!rd8(m,addr,&v))return RE_CPU_E_MEMORY;c->r[rd]=v;}
  else {if(!wr8(m,addr,(uint8_t)c->r[rd]))return RE_CPU_E_MEMORY;}
  c->r[15]=pc+4u; return RE_CPU_OK;
 }
 /* IT instruction: condition in bits 7:4, mask in bits 3:0. */
 if((x&0xff00u)==0xbf00u && (x&0x000fu)!=0u){
  unsigned cc=(x>>4)&15u;
  if(cc>=14u)return RE_CPU_E_UNIMPLEMENTED;
  if(c->itstate)return RE_CPU_E_UNIMPLEMENTED;
  it_set(c,(uint8_t)((cc<<4)|(x&15u)));
  return RE_CPU_OK;
 }
 /* Thumb-1 register-offset memory group: STR/STRH/STRB/LDRSB/LDR/LDRH/LDRB/LDRSH. */
 if((x&0xf000u)==0x5000u){
  unsigned code=(x>>9)&7u; rm=(x>>6)&7u; rs=(x>>3)&7u; rd=x&7u;
  addr=c->r[rs]+c->r[rm];
  if(code==0u){q=write32(m,addr,c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;}
  else if(code==1u){q=write16(m,addr,(uint16_t)c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;}
  else if(code==2u){if(!wr8(m,addr,(uint8_t)c->r[rd]))return RE_CPU_E_MEMORY;}
  else if(code==3u){uint8_t v;if(!rd8(m,addr,&v))return RE_CPU_E_MEMORY;c->r[rd]=(uint32_t)(int32_t)(int8_t)v;}
  else if(code==4u){q=read32(m,addr,&r);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;c->r[rd]=r;}
  else if(code==5u){uint16_t v;q=read16(m,addr,&v);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;c->r[rd]=v;}
  else if(code==6u){uint8_t v;if(!rd8(m,addr,&v))return RE_CPU_E_MEMORY;c->r[rd]=v;}
  else {uint16_t v;q=read16(m,addr,&v);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;c->r[rd]=(uint32_t)(int32_t)(int16_t)v;}
  return RE_CPU_OK;
 }
 /* Thumb-1 immediate halfword transfer: 1000 L imm5 Rn Rd. */
 if((x&0xf000u)==0x8000u){
  rd=x&7u;rs=(x>>3)&7u;imm=((x>>6)&31u)*2u;addr=c->r[rs]+imm;
  if(x&0x0800u){uint16_t v;q=read16(m,addr,&v);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;c->r[rd]=v;}
  else {q=write16(m,addr,(uint16_t)c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;}
  return RE_CPU_OK;
 }
 /* Thumb-2 register-offset wide transfers (imm2 shift, no writeback). */
 if((x&0xfff0u)==0xf800u || (x&0xfff0u)==0xf810u ||
    (x&0xfff0u)==0xf820u || (x&0xfff0u)==0xf830u ||
    (x&0xfff0u)==0xf840u || (x&0xfff0u)==0xf850u ||
    (x&0xfff0u)==0xf910u || (x&0xfff0u)==0xf930u){
  uint16_t y; q=read16(m,pc+2,&y); if(q<0)return RE_CPU_E_ALIGNMENT; if(!q)return RE_CPU_E_MEMORY;
  unsigned rn=x&15u, rm=y&15u, scale=(y>>4)&3u;
  rd=(y>>12)&15u; addr=(rn==15u?((pc+4u)&~3u):c->r[rn])+(c->r[rm]<<scale);
  if((x&0xfff0u)==0xf850u){q=read32(m,addr,&r);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;c->r[rd]=r;}
  else if((x&0xfff0u)==0xf840u){q=write32(m,addr,c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;}
  else if((x&0xfff0u)==0xf810u){uint8_t v;if(!rd8(m,addr,&v))return RE_CPU_E_MEMORY;c->r[rd]=v;}
  else if((x&0xfff0u)==0xf800u){if(!wr8(m,addr,(uint8_t)c->r[rd]))return RE_CPU_E_MEMORY;}
  else if((x&0xfff0u)==0xf830u){uint16_t v;q=read16(m,addr,&v);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;c->r[rd]=v;}
  else if((x&0xfff0u)==0xf820u){q=write16(m,addr,(uint16_t)c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;}
  else if((x&0xfff0u)==0xf910u){uint8_t v;if(!rd8(m,addr,&v))return RE_CPU_E_MEMORY;c->r[rd]=(uint32_t)(int32_t)(int8_t)v;}
  else {uint16_t v;q=read16(m,addr,&v);if(q<0)return RE_CPU_E_ALIGNMENT;if(!q)return RE_CPU_E_MEMORY;c->r[rd]=(uint32_t)(int32_t)(int16_t)v;}
  c->r[15]=pc+4u;return RE_CPU_OK;
 }
 /* Thumb-2 MOVW/MOVT (wide immediate construction). */
 if((x&0xfbf0u)==0xf240u || (x&0xfbf0u)==0xf2c0u){
  uint16_t y; q=read16(m,pc+2,&y); if(q<0)return RE_CPU_E_ALIGNMENT; if(!q)return RE_CPU_E_MEMORY;
  unsigned imm16=((x&15u)<<12)|(((x>>10)&1u)<<11)|(((y>>12)&7u)<<8)|(y&255u);
  rd=(y>>8)&15u;
  if((x&0xfbf0u)==0xf240u)c->r[rd]=(c->r[rd]&0xffff0000u)|imm16;
  else c->r[rd]=(c->r[rd]&0x0000ffffu)|(imm16<<16);
  c->r[15]=pc+4; return RE_CPU_OK;
 }
 if((x&0xf800)==0x2000){rd=(x>>8)&7;imm=x&255;c->r[rd]=imm;set_nz(c,imm);return RE_CPU_OK;}
 if((x&0xf800)==0x2800){rd=(x>>8)&7;imm=x&255;r=c->r[rd]-imm;set_sub(c,c->r[rd],imm,r);return RE_CPU_OK;}
 if((x&0xf800)==0x3000||(x&0xf800)==0x3800){rd=(x>>8)&7;imm=x&255;a=c->r[rd];r=(x&0x0800)?a-imm:a+imm;c->r[rd]=r;if(x&0x0800)set_sub(c,a,imm,r);else set_add(c,a,imm,r);return RE_CPU_OK;}
 if((x&0xf800)==0x0000||(x&0xf800)==0x0800||(x&0xf800)==0x1000){rd=x&7;rs=(x>>3)&7;imm=(x>>6)&31;a=c->r[rs];op=(x>>11)&3;r=op==0?a<<imm:op==1?(imm?a>>imm:0):(imm?(uint32_t)((int32_t)a>>imm):(uint32_t)((int32_t)a>>31));c->r[rd]=r;set_nz(c,r);return RE_CPU_OK;}
 if((x&0xfc00)==0x4000){op=(x>>6)&15;rd=x&7;rs=(x>>3)&7;a=c->r[rd];b=c->r[rs];switch(op){case 0:r=a&b;break;case 1:r=a^b;break;case 2:r=a<<(b&255);break;case 3:r=(uint32_t)((int32_t)a>>(b&255));break;case 8:r=a&b;set_nz(c,r);return RE_CPU_OK;case 10:r=a-b;set_sub(c,a,b,r);return RE_CPU_OK;case 12:r=a|b;break;case 14:r=a&~b;break;case 15:r=~b;break;default:return RE_CPU_E_UNIMPLEMENTED;}c->r[rd]=r;set_nz(c,r);return RE_CPU_OK;}
 if((x&0xf800)==0xe000){int32_t off=x&0x7ff;if(off&0x400)off|=~0x7ff;c->r[15]=pc+4+(uint32_t)(off*2);return RE_CPU_OK;}
 if((x&0xf000)==0xd000&&((x>>8)&15)<14){unsigned cc=(x>>8)&15;int32_t off=(int8_t)(x&255);if(cond(c->cpsr,cc))c->r[15]=pc+4+(uint32_t)(off*2);return RE_CPU_OK;}
 if((x&0xff87)==0x4700){rs=(x>>3)&15;addr=rs==15?pc+4:c->r[rs];c->thumb=addr&1;c->r[15]=addr&(c->thumb?~1u:~3u);return RE_CPU_OK;}
 if((x&0xf800)==0x6000||(x&0xf800)==0x6800){rd=x&7;rs=(x>>3)&7;imm=((x>>6)&31)*4;addr=c->r[rs]+imm;if(x&0x0800){q=read32(m,addr,&r);if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;
 c->r[rd]=r;}else{q=write32(m,addr,c->r[rd]);if(q<0)return RE_CPU_E_ALIGNMENT;
 if(!q)return RE_CPU_E_MEMORY;}return RE_CPU_OK;}
 return RE_CPU_E_UNIMPLEMENTED;
}
void re_guest_cpu_reset(REGUESTCPU*c,uint32_t entry,int thumb){if(!c)return;for(size_t i=0;i<sizeof(*c);i++)((uint8_t*)c)[i]=0;c->thumb=!!thumb;c->r[15]=entry&(thumb?~1u:~3u);it_sync(c);}
static int thumb_control_flow(const REGUESTMEM*m,uint32_t pc,uint16_t first){
 /* 16-bit conditional/unconditional branches and BX/BLX-register. */
 if((first&0xf800u)==0xe000u)return 1;
 if((first&0xf000u)==0xd000u && ((first>>8)&15u)<14u)return 1;
 if((first&0xff87u)==0x4700u)return 1;
 /* POP including PC. */
 if((first&0xfe00u)==0xbc00u && (first&0x0100u))return 1;
 /* Thumb-2 B.cond, B, BL and BLX immediate. */
 if((first&0xf800u)==0xf000u){
  uint16_t second;
  if(read16(m,pc+2u,&second)!=1)return 0;
  if((second&0xd000u)==0x8000u || (second&0xd000u)==0x9000u ||
     (second&0xd000u)==0xd000u || (second&0xd001u)==0xc000u)return 1;
 }
 /* Wide load into PC is a control-flow transfer. */
 if((first&0xfff0u)==0xf8d0u || (first&0xfff0u)==0xf850u){
  uint16_t second;
  if(fetch16(m,pc+2u,&second)==1 && ((second>>12)&15u)==15u)return 1;
 }
 return 0;
}
REGUESTCPUStatus re_guest_cpu_step(REGUESTCPU*c,const REGUESTMEM*m){
 if(!c||!m||!m->bytes)return RE_CPU_E_ARGUMENT;
 if(c->halted)return RE_CPU_HALTED;
 uint32_t pc=c->r[15]; REGUESTCPUStatus s;
 if(c->thumb && c->itstate){
  uint16_t first; int q=read16(m,pc,&first);
  if(q<0)return RE_CPU_E_ALIGNMENT;
  if(!q)return RE_CPU_E_MEMORY;
  unsigned width=((first&0xf800u)>=0xe800u)?4u:2u;
  if((first&0xff00u)==0xbf00u && (first&15u))return RE_CPU_E_UNIMPLEMENTED;
  if(!cond(c->cpsr,(c->itstate>>4)&15u)){
   c->r[15]=pc+width;it_advance(c);s=RE_CPU_OK;
  }else{
   int flow=thumb_control_flow(m,pc,first);
   s=thumb(c,m,pc);
   if(s==RE_CPU_OK){
    if(flow || c->r[15]!=(pc+width))it_set(c,0);
    else it_advance(c);
   }
  }
 }else s=c->thumb?thumb(c,m,pc):arm(c,m,pc);
 if(s==RE_CPU_OK)c->steps++;
 return s;
}
REGUESTCPUStatus re_guest_cpu_run(REGUESTCPU*c,const REGUESTMEM*m,uint64_t limit,uint64_t*done){uint64_t n=0;if(done)*done=0;if(!c||!m||!limit)return RE_CPU_E_ARGUMENT;while(n<limit){REGUESTCPUStatus s=re_guest_cpu_step(c,m);if(s!=RE_CPU_OK){if(done)*done=n;return s;}n++;}if(done)*done=n;return RE_CPU_OK;}
