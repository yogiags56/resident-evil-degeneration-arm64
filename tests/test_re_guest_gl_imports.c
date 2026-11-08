#include "re_guest_gl_imports.h"
#include "re_guest_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct { uint32_t calls_i,calls_x,t,p; int32_t v; } MockGL;
static void mock_i(uint32_t t,uint32_t p,int32_t v) {
    extern MockGL *g_mock; g_mock->calls_i++;g_mock->t=t;g_mock->p=p;g_mock->v=v;
}
static void mock_x(uint32_t t,uint32_t p,int32_t v) {
    extern MockGL *g_mock; g_mock->calls_x++;g_mock->t=t;g_mock->p=p;g_mock->v=v;
}
MockGL *g_mock;
static uint32_t rd32(const uint8_t*p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
int main(void) {
    static uint8_t bytes[0x200000];
    REGUESTMEM mem={bytes,sizeof(bytes),0x50000000u};
    REGUESTIMPORTTable table;
    REGUESTGLBackend backend;
    REGUESTRuntime rt;
    MockGL mock={0}; uint32_t pi=0,px=0;
    g_mock=&mock; backend.tex_parameter_i=mock_i;backend.tex_parameter_x=mock_x;backend.user=0;
    assert(re_guest_import_table_init(&table,0x51000000u)==RE_IMPORT_OK);
    assert(re_guest_gl_bind_verified(&table,&mem,&backend,mem.guest_base,&pi,&px)==RE_IMPORT_OK);
    assert(table.count==2 && pi!=px);
    assert(rd32(bytes+0xC7C4C)==pi && rd32(bytes+0x13B960)==pi);
    assert(rd32(bytes+0xC7C50)==px && rd32(bytes+0x13B8C0)==px);
    re_guest_runtime_init(&rt,&mem,&table,0x4A000070u,0x50000000u,1);
    rt.cpu.r[15]=pi&~1u;rt.cpu.r[14]=0x50000101u;rt.cpu.r[13]=0x50000100u;rt.cpu.r[0]=0x0DE1;rt.cpu.r[1]=0x2801;rt.cpu.r[2]=0x2601;
    assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
    assert(mock.calls_i==1 && mock.t==0x0DE1 && mock.p==0x2801 && mock.v==0x2601);
    assert(rt.cpu.r[15]==0x50000100u && rt.dispatched_imports==1);
    rt.cpu.r[15]=px&~1u;rt.cpu.r[14]=0x50000121u;rt.cpu.r[13]=0x50000100u;rt.cpu.r[0]=0x0DE1;rt.cpu.r[1]=0x2800;rt.cpu.r[2]=0x00010000;
    assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
    assert(mock.calls_x==1 && mock.v==0x00010000);
    assert(rt.cpu.r[15]==0x50000120u && rt.dispatched_imports==2);
    puts("Verified GL import adapter tests: PASS (12 assertions)");
    return 0;
}
