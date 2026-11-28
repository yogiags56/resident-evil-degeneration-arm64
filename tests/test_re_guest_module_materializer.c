#include "re_guest_module_materializer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { const char *data; size_t size; size_t offset; size_t max_chunk; int closed; int fail_read; } Fake;
static void *fake_open(void *user,const char *path) {
    Fake *f=(Fake *)user;
    if (!strcmp(path,"missing.bin")) return NULL;
    f->offset=0; f->closed=0; return f;
}
static int64_t fake_length(void *user,void *stream) {
    Fake *f=(Fake *)stream; (void)user; return (int64_t)f->size;
}
static int64_t fake_read(void *user,void *stream,void *dst,uint64_t count) {
    Fake *f=(Fake *)stream; size_t n; (void)user;
    if (f->fail_read) return -1;
    if (f->offset==f->size) return 0;
    n=f->size-f->offset;
    if (n>count)n=(size_t)count;
    if (n>f->max_chunk)n=f->max_chunk;
    memcpy(dst,f->data+f->offset,n); f->offset+=n; return (int64_t)n;
}
static void fake_close(void *user,void *stream) { Fake *f=(Fake *)stream;(void)user;f->closed++; }
int main(void) {
    const char payload[]="synthetic ELF payload";
    REGUESTMODULEMANIFEST m; REGUESTMATERIALIZEDSET out;
    REGUESTMODULESOURCE src={fake_open,fake_length,fake_read,fake_close};
    Fake fake={payload,sizeof(payload)-1u,0u,3u,0,0};
    memset(&m,0,sizeof(m)); memset(&out,0,sizeof(out));
    m.version=1u;m.module_count=1u;m.root_module_index=0u;
    strcpy(m.modules[0].soname,"game.so");
    strcpy(m.modules[0].asset_path,"payload/game.elf");
    m.modules[0].max_bytes=1024u;
    assert(re_guest_module_materialize(&m,&src,&fake,&out)==RE_GUEST_MATERIALIZE_OK);
    assert(out.module_count==1u && out.modules[0].size==sizeof(payload)-1u);
    assert(memcmp(out.modules[0].bytes,payload,sizeof(payload)-1u)==0);
    assert(fake.closed==1);
    re_guest_module_materialized_free(&out);
    assert(out.module_count==0u && out.modules[0].bytes==NULL);

    fake.size=sizeof(payload)-1u;fake.max_chunk=3u;fake.fail_read=1;
    assert(re_guest_module_materialize(&m,&src,&fake,&out)==RE_GUEST_MATERIALIZE_E_READ);
    assert(out.module_count==0u && out.modules[0].bytes==NULL);
    assert(fake.closed==1);

    fake.fail_read=0;strcpy(m.modules[0].asset_path,"missing.bin");
    assert(re_guest_module_materialize(&m,&src,&fake,&out)==RE_GUEST_MATERIALIZE_E_OPEN);
    assert(out.module_count==0u);

    puts("Guest module materializer tests: PASS");
    return 0;
}
