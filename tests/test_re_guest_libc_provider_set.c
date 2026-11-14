#include "re_guest_libc_provider_set.h"
#include "re_guest_heap.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void *stub_open(void *u,const char *p,const char *m){(void)u;(void)p;(void)m;return NULL;}
static int64_t stub_read(void *u,void *s,void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int64_t stub_write(void *u,void *s,const void *d,uint64_t n){(void)u;(void)s;(void)d;(void)n;return -1;}
static int stub_close(void *u,void *s){(void)u;(void)s;return 0;}

int main(void) {
    static const char *const names[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT] = {
        "malloc","free","realloc","valloc",
        "memcpy","memmove","memset","strlen","strcmp","strncmp","strcpy",
        "strncpy","strncat","strchr","strstr","strcasecmp","strncasecmp",
        "atoi","strtok",
        "snprintf","sprintf","printf",
        "fopen","fclose","fread","fwrite","fseek","ftell","fgets",
        "fprintf","feof","ferror","fflush","sscanf","fscanf","ungetc"
    };
    REGUESTELF32LINKSYMBOL symbols[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL exports[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTLIBCAllocProvider alloc;
    REGUESTLIBCStringProvider string;
    REGUESTLIBCFormatProvider format;
    REGUESTLIBCStdioProvider stdio;
    REGUESTLIBCStdioBackend backend = {
        stub_open,stub_read,stub_write,NULL,NULL,stub_close,NULL
    };
    REGUESTHEAP heap;
    REGUESTLIBCProviderSet set = {&alloc,&string,&format,&stdio};
    REGUESTIMPORTTable imports, before;
    size_t count=0u, failed=0u;
    REGUESTLIBCProviderSetStatus bind_status;

    memset(symbols,0,sizeof(symbols));
    memset(exports,0xa5,sizeof(exports));
    for(size_t i=0;i<RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT;i++) {
        strcpy(symbols[i].name,names[i]);
        symbols[i].binding=RE_GUEST_STB_GLOBAL;
        symbols[i].visibility=RE_GUEST_STV_DEFAULT;
        symbols[i].type=2u;
        symbols[i].section_index=0u;
    }
    assert(re_guest_heap_init(&heap,0x50000000u,0x10000u)==RE_GUEST_HEAP_OK);
    assert(re_guest_libc_alloc_provider_init(&alloc,&heap)==RE_GUEST_LIBC_ALLOC_OK);
    assert(re_guest_libc_string_provider_init(&string)==RE_GUEST_LIBC_STRING_OK);
    assert(re_guest_libc_format_provider_init(&format)==RE_GUEST_LIBC_FORMAT_OK);
    assert(re_guest_libc_stdio_provider_init(&stdio,&backend,NULL)==RE_GUEST_LIBC_STDIO_OK);
    assert(re_guest_import_table_init(&imports,0x30000000u)==RE_IMPORT_OK);

    assert(re_guest_libc_provider_set_bind_elf_symbols(&set,&imports,symbols,
        RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,exports,
        RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&count,&failed)==
        RE_GUEST_LIBC_PROVIDER_SET_OK);
    assert(count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(imports.count==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(failed==RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT);
    assert(strcmp(exports[0].name,"malloc")==0);
    assert(strcmp(exports[4].name,"memcpy")==0);
    assert(strcmp(exports[19].name,"snprintf")==0);
    assert(strcmp(exports[22].name,"fopen")==0);
    assert(strcmp(exports[35].name,"ungetc")==0);

    /* Retry from an empty table so the deliberately missing stdio symbol
       fails after earlier provider families have already bound. */
    assert(re_guest_import_table_init(&imports,0x30000000u)==RE_IMPORT_OK);
    /* Missing one exact symbol must roll back every import binding and
       preserve the caller's output array. */
    before=imports;
    memset(exports,0x5a,sizeof(exports));
    symbols[35].name[0]='x';
    count=99u;
    bind_status=re_guest_libc_provider_set_bind_elf_symbols(&set,&imports,symbols,
        RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,exports,
        RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,&count,&failed);
    if(bind_status!=RE_GUEST_LIBC_PROVIDER_SET_E_STDIO)
        fprintf(stderr,"unexpected provider-set status=%d failed=%zu imports=%zu\\n",
            (int)bind_status,failed,imports.count);
    assert(bind_status==RE_GUEST_LIBC_PROVIDER_SET_E_STDIO);
    assert(imports.count==before.count);
    assert(memcmp(&imports,&before,sizeof(imports))==0);
    assert(count==0u);
    for(size_t i=0;i<sizeof(exports);i++)
        assert(((const unsigned char *)exports)[i]==0x5a);

    re_guest_heap_shutdown(&heap);
    puts("Transactional libc provider set tests: PASS");
    return 0;
}
