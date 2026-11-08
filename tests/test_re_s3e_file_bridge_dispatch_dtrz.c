#include "re_guest_imports.h"
#include "re_s3e_file_bridge.h"
#include "re_dtrz_file_backend.h"
#include "re_dtrz_vfs.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
/* One stored entry: hello.txt -> "hello". Mirrors the observed DTRZ layout. */
static uint8_t *make_archive(size_t *size) {
    const size_t payload=9u+11u+10u+7u+12u+4u+32u, total=payload+9u;
    uint8_t *b=(uint8_t *)calloc(total,1u); size_t p=9u;
    assert(b);
    memcpy(b,"DTRZ",4); put16(b+4,2); put16(b+6,2); b[8]=0;
    memcpy(b+p,"readme.txt",10); p+=11;
    memcpy(b+p,"asset.bin",9); p+=10;
    memcpy(b+p,"folder",6); p+=7;
    put16(b+p,0); put16(b+p+2,0); put16(b+p+4,0xffff); p+=6;
    put16(b+p,1); put16(b+p+2,1); put16(b+p+4,0xffff); p+=6;
    put16(b+p,1); put16(b+p+2,2); p+=4;
    put32(b+p,(uint32_t)payload); put32(b+p+4,5); put32(b+p+8,5);
    put32(b+p+12,RE_DTRZ_METHOD_STORED); p+=16;
    put32(b+p,(uint32_t)(payload+5u)); put32(b+p+4,4); put32(b+p+8,4);
    put32(b+p+12,RE_DTRZ_METHOD_STORED); p+=16;
    assert(p==payload); memcpy(b+p,"hello",5); memcpy(b+p+5,"PACK",4);
    *size=total; return b;
}
static void dispatch(REGUESTIMPORTTable *t, REGUESTCPU *cpu, REGUESTMEM *m,
                     uint32_t ptr, uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3,
                     uint32_t *assertions) {
    cpu->thumb=1; cpu->r[15]=ptr&~1u; cpu->r[14]=0x50000101u;
    cpu->r[13]=m->guest_base+0x1f0u;
    cpu->r[0]=a0; cpu->r[1]=a1; cpu->r[2]=a2; cpu->r[3]=a3;
    assert(re_guest_import_dispatch(t,cpu,m)==RE_IMPORT_OK); (*assertions)++;
    assert(cpu->r[15]==0x50000100u && cpu->thumb==1); (*assertions)++;
}
int main(void) {
    uint8_t guest[512]={0}; size_t archive_size=0; uint8_t *archive_bytes=make_archive(&archive_size);
    REGUESTMEM memory={guest,sizeof(guest),0x50000000u};
    re_dtrz_archive archive; re_dtrz_vfs vfs; re_dtrz_file_backend dtrz_backend;
    REFileBackend callbacks; REFileAdapter adapter; RES3EFileBridge bridge;
    REGUESTIMPORTTable table; REGUESTCPU cpu={0}; uint32_t ptr[4], assertions=0;
    const uint32_t symbols[4]={0xF101u,0xF102u,0xF103u,0xF104u};
    memcpy(guest+0x00,"readme.txt",11); memcpy(guest+0x20,"rb",3);
    assert(re_dtrz_archive_parse(&archive,archive_bytes,archive_size)==RE_DTRZ_OK); assertions++;
    assert(re_dtrz_vfs_init(&vfs,&archive,NULL,NULL)==RE_DTRZ_OK); assertions++;
    assert(re_dtrz_file_backend_init(&dtrz_backend,&vfs,&callbacks)==RE_FILE_OK); assertions++;
    assert(re_file_adapter_init(&adapter,&callbacks,&dtrz_backend)==RE_FILE_OK); assertions++;
    assert(re_s3e_file_bridge_init(&bridge,&adapter)==RE_S3E_BRIDGE_OK); assertions++;
    assert(re_guest_import_table_init(&table,0x51000000u)==RE_IMPORT_OK); assertions++;
    assert(re_guest_import_bind(&table,symbols[0],re_s3e_file_bridge_open_handler,&bridge,&ptr[0])==RE_IMPORT_OK); assertions++;
    assert(re_guest_import_bind(&table,symbols[1],re_s3e_file_bridge_read_handler,&bridge,&ptr[1])==RE_IMPORT_OK); assertions++;
    assert(re_guest_import_bind(&table,symbols[2],re_s3e_file_bridge_get_char_handler,&bridge,&ptr[2])==RE_IMPORT_OK); assertions++;
    assert(re_guest_import_bind(&table,symbols[3],re_s3e_file_bridge_close_handler,&bridge,&ptr[3])==RE_IMPORT_OK); assertions++;

    /* Open via guest registers; path/mode strings are guest-memory addresses. */
    dispatch(&table,&cpu,&memory,ptr[0],memory.guest_base, memory.guest_base+0x20,0,0,&assertions);
    uint32_t handle=cpu.r[0]; assert(handle!=0); assertions++;
    assert(re_dtrz_file_backend_active_streams(&dtrz_backend)==1); assertions++;

    /* fread-like guest call: size=2,count=2 => 4 bytes, return 2 items. */
    dispatch(&table,&cpu,&memory,ptr[1],memory.guest_base+0x80,2,2,handle,&assertions);
    assert(cpu.r[0]==2); assertions++;
    assert(memcmp(guest+0x80,"hell",4)==0); assertions++;

    dispatch(&table,&cpu,&memory,ptr[2],handle,0,0,0,&assertions);
    assert(cpu.r[0]==(uint32_t)'o'); assertions++;
    dispatch(&table,&cpu,&memory,ptr[2],handle,0,0,0,&assertions);
    assert(cpu.r[0]==UINT32_MAX); assertions++;

    dispatch(&table,&cpu,&memory,ptr[3],handle,0,0,0,&assertions);
    assert(cpu.r[0]==0); assertions++;
    assert(re_dtrz_file_backend_active_streams(&dtrz_backend)==0); assertions++;
    /* A stale token is rejected by the actual handler/dispatcher path. */
    cpu.thumb=1; cpu.r[15]=ptr[2]&~1u; cpu.r[14]=0x50000101u;
    cpu.r[13]=memory.guest_base+0x1f0u; cpu.r[0]=handle;
    assert(re_guest_import_dispatch(&table,&cpu,&memory)==RE_IMPORT_E_HANDLER); assertions++;
    assert(re_s3e_file_bridge_production_imports_authorized()==0); assertions++;

    re_file_adapter_close_all(&adapter); assert(re_dtrz_file_backend_active_streams(&dtrz_backend)==0); assertions++;
    re_dtrz_archive_destroy(&archive); free(archive_bytes);
    printf("Stage 99 dispatcher + DTRZ end-to-end: PASS (%u assertions)\n",assertions);
    return 0;
}
