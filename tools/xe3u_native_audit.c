#include "re_guest_xe3u_decode.h"
#include <stdio.h>
#include <stdlib.h>

#define XE3U_DECODE_CAPACITY (64u*1024u*1024u)
#define XE3U_SYMBOL_CAPACITY 512u
#define XE3U_INTERNAL_CAPACITY 65536u
#define XE3U_EXTERNAL_CAPACITY 4096u

static void *read_file(const char *path,size_t *size) {
    FILE *file=fopen(path,"rb");
    long length;
    uint8_t *bytes;
    if(!file)return NULL;
    if(fseek(file,0,SEEK_END)!=0||(length=ftell(file))<0||
       fseek(file,0,SEEK_SET)!=0) {
        fclose(file);return NULL;
    }
    bytes=(uint8_t *)malloc((size_t)length? (size_t)length:1u);
    if(!bytes){fclose(file);return NULL;}
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length) {
        free(bytes);fclose(file);return NULL;
    }
    fclose(file);*size=(size_t)length;return bytes;
}
int main(int argc,char **argv) {
    uint8_t *source=NULL,*decoded=NULL;
    REGUESTXE3UHEADER header;
    REGUESTXE3USYMBOL *symbols=NULL;
    uint32_t *internal=NULL;
    REGUESTXE3UEXTERNALRELOCATION *external=NULL;
    REGUESTXE3UFIXUPINFO fixups;
    size_t source_size=0,decoded_size=0,failed_field=0;
    size_t failed_section=0,failed_record=0;
    REGUESTXE3UDECODEStatus ds;
    REGUESTXE3UStatus fs;
    int result=1;
    if(argc!=2) {
        fprintf(stderr,"usage: %s biohazard.s3e\n",argv[0]);return 2;
    }
    source=(uint8_t *)read_file(argv[1],&source_size);
    decoded=(uint8_t *)malloc(XE3U_DECODE_CAPACITY);
    symbols=(REGUESTXE3USYMBOL *)calloc(XE3U_SYMBOL_CAPACITY,sizeof(*symbols));
    internal=(uint32_t *)calloc(XE3U_INTERNAL_CAPACITY,sizeof(*internal));
    external=(REGUESTXE3UEXTERNALRELOCATION *)calloc(
        XE3U_EXTERNAL_CAPACITY,sizeof(*external));
    if(!source||!decoded||!symbols||!internal||!external) {
        fprintf(stderr,"allocation or input read failed\n");goto cleanup;
    }
    ds=re_guest_xe3u_decode_lzma_alone(source,source_size,decoded,
        XE3U_DECODE_CAPACITY,64u*1024u*1024u,&decoded_size,&header,&failed_field);
    if(ds!=RE_GUEST_XE3U_DECODE_OK) {
        fprintf(stderr,"XE3U decode failed: status=%d field=%zu\n",
            (int)ds,failed_field);goto cleanup;
    }
    fs=re_guest_xe3u_parse_fixups(decoded,decoded_size,&header,
        symbols,XE3U_SYMBOL_CAPACITY,internal,XE3U_INTERNAL_CAPACITY,
        external,XE3U_EXTERNAL_CAPACITY,&fixups,&failed_section,&failed_record);
    if(fs!=RE_GUEST_XE3U_OK) {
        fprintf(stderr,"XE3U fixup parse failed: status=%d section=%zu record=%zu\n",
            (int)fs,failed_section,failed_record);goto cleanup;
    }
    printf("XE3U decoded_size=%zu version=0x%08x flags=0x%04x arch=%u\n",
        decoded_size,header.version,header.flags,header.architecture);
    printf("fixup=0x%08x+0x%08x code=0x%08x file=0x%08x memory=0x%08x\n",
        header.fixup_offset,header.fixup_size,header.code_offset,
        header.code_file_size,header.code_memory_size);
    printf("entry_offset=0x%08x base=0x%08x data_offset=0x%08x extra=0x%08x+0x%08x\n",
        header.entry_offset,header.base_address,header.data_offset,
        header.extra_offset,header.extra_size);
    printf("fixup_sections=%zu symbols=%zu internal=%zu external=%zu\n",
        fixups.section_count,fixups.symbol_count,
        fixups.internal_relocation_count,fixups.external_relocation_count);
    for(size_t i=0;i<fixups.section_count;i++)
        printf("section[%zu] type=%u offset=0x%08x size=0x%08x count=%u\n",
            i,fixups.sections[i].type,fixups.sections[i].offset,
            fixups.sections[i].size,fixups.sections[i].count);
    result=0;
cleanup:
    free(source);free(decoded);free(symbols);free(internal);free(external);
    return result;
}
