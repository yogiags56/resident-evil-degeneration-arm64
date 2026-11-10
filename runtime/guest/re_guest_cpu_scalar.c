#include "re_guest_cpu_scalar.h"

static REGUESTCPUStatus map_memory_status(REGUESTMEMStatus s) {
 switch(s) {
 case RE_GUEST_MEM_OK: return RE_CPU_OK;
 case RE_GUEST_MEM_E_PERMISSION: return RE_CPU_E_PERMISSION;
 case RE_GUEST_MEM_E_ARGUMENT: return RE_CPU_E_ARGUMENT;
 case RE_GUEST_MEM_E_RANGE: return RE_CPU_E_ALIGNMENT;
 case RE_GUEST_MEM_E_NOT_FOUND:
 case RE_GUEST_MEM_E_CAPACITY:
 case RE_GUEST_MEM_E_OVERLAP:
 default: return RE_CPU_E_MEMORY;
 }
}

static REGUESTCPUStatus check(const REGUESTCPUSCALAR *s,uint32_t address,
 size_t width,REGUESTACCESSCLASS cls,uint32_t permission,void **host) {
 REGUESTALIGNMENTSTATUS a;
 REGUESTMEMStatus m;
 if(host) *host=NULL;
 if(!s||!s->memory||!s->alignment||!host) return RE_CPU_E_ARGUMENT;
 a=re_guest_alignment_check(s->memory,s->alignment,address,width,cls);
 if(a==RE_GUEST_ALIGN_E_ARGUMENT) return RE_CPU_E_ARGUMENT;
 if(a==RE_GUEST_ALIGN_E_FAULT) return RE_CPU_E_ALIGNMENT;
 if(a==RE_GUEST_ALIGN_E_MEMORY) return RE_CPU_E_MEMORY;
 if(a!=RE_GUEST_ALIGN_OK) return RE_CPU_E_UNIMPLEMENTED;
 m=re_guest_memory_resolve(s->memory,address,width,permission,host);
 return map_memory_status(m);
}

static uint32_t assemble_le(const uint8_t *p,size_t width) {
 uint32_t v=0;
 for(size_t i=0;i<width;i++) v|=(uint32_t)p[i]<<(8u*i);
 return v;
}

static void disassemble_le(uint8_t *p,size_t width,uint32_t value) {
 for(size_t i=0;i<width;i++) p[i]=(uint8_t)(value>>(8u*i));
}

void re_guest_cpu_scalar_init(REGUESTCPUSCALAR *s,
 const REGUESTMEMORY *m,const REGUESTALIGNMENTPOLICY *a) {
 if(s) { s->memory=m; s->alignment=a; }
}

static REGUESTCPUStatus read_scalar(const REGUESTCPUSCALAR *s,
 uint32_t address,size_t width,REGUESTACCESSCLASS cls,uint32_t *out) {
 void *host=NULL;
 REGUESTCPUStatus status;
 uint32_t value;
 if(!out) return RE_CPU_E_ARGUMENT;
 status=check(s,address,width,cls,RE_GUEST_MEM_READ,&host);
 if(status!=RE_CPU_OK) return status;
 value=assemble_le((const uint8_t *)host,width);
 *out=value;
 return RE_CPU_OK;
}

static REGUESTCPUStatus write_scalar(const REGUESTCPUSCALAR *s,
 uint32_t address,size_t width,REGUESTACCESSCLASS cls,uint32_t value) {
 void *host=NULL;
 REGUESTCPUStatus status=check(s,address,width,cls,RE_GUEST_MEM_WRITE,&host);
 if(status!=RE_CPU_OK) return status;
 disassemble_le((uint8_t *)host,width,value);
 return RE_CPU_OK;
}

REGUESTCPUStatus re_guest_cpu_scalar_read8(const REGUESTCPUSCALAR *s,
 uint32_t a,uint8_t *v) {
 uint32_t x=0; REGUESTCPUStatus st;
 if(!v) return RE_CPU_E_ARGUMENT;
 st=read_scalar(s,a,1u,RE_GUEST_ACCESS_BYTE,&x);
 if(st==RE_CPU_OK) *v=(uint8_t)x;
 return st;
}
REGUESTCPUStatus re_guest_cpu_scalar_read16(const REGUESTCPUSCALAR *s,
 uint32_t a,uint16_t *v) {
 uint32_t x=0; REGUESTCPUStatus st;
 if(!v) return RE_CPU_E_ARGUMENT;
 st=read_scalar(s,a,2u,RE_GUEST_ACCESS_HALFWORD,&x);
 if(st==RE_CPU_OK) *v=(uint16_t)x;
 return st;
}
REGUESTCPUStatus re_guest_cpu_scalar_read32(const REGUESTCPUSCALAR *s,
 uint32_t a,uint32_t *v) {
 if(!v) return RE_CPU_E_ARGUMENT;
 return read_scalar(s,a,4u,RE_GUEST_ACCESS_WORD,v);
}
REGUESTCPUStatus re_guest_cpu_scalar_write8(const REGUESTCPUSCALAR *s,
 uint32_t a,uint8_t v) {
 return write_scalar(s,a,1u,RE_GUEST_ACCESS_BYTE,v);
}
REGUESTCPUStatus re_guest_cpu_scalar_write16(const REGUESTCPUSCALAR *s,
 uint32_t a,uint16_t v) {
 return write_scalar(s,a,2u,RE_GUEST_ACCESS_HALFWORD,v);
}
REGUESTCPUStatus re_guest_cpu_scalar_write32(const REGUESTCPUSCALAR *s,
 uint32_t a,uint32_t v) {
 return write_scalar(s,a,4u,RE_GUEST_ACCESS_WORD,v);
}
