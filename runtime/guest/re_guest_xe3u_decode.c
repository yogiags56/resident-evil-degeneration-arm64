#include "re_guest_xe3u_decode.h"
#include "re_dtrz_lzma.h"
#include <string.h>
REGUESTXE3UDECODEStatus re_guest_xe3u_decode_lzma_alone(
    const uint8_t *source,size_t source_size,uint8_t *destination,
    size_t capacity,uint64_t memory_limit,size_t *written,
    REGUESTXE3UHEADER *header,size_t *failed_field) {
    re_dtrz_lzma_context context;
    REGUESTXE3UHEADER pending;
    size_t decoded=0,failed=0;
    re_dtrz_status ds;
    REGUESTXE3UStatus hs;
    if(written)*written=0;
    if(failed_field)*failed_field=0;
    if(!source||source_size<13u||!destination||!capacity||!written||!header)
        return RE_GUEST_XE3U_DECODE_E_ARGUMENT;
    re_dtrz_lzma_context_init(&context,memory_limit);
    ds=re_dtrz_lzma_alone_decode(&context,RE_DTRZ_METHOD_LZMA_ALONE,
        source,source_size,destination,capacity,&decoded);
    if(ds==RE_DTRZ_E_MEMORY)return RE_GUEST_XE3U_DECODE_E_MEMORY;
    if(ds==RE_DTRZ_E_BOUNDS)return RE_GUEST_XE3U_DECODE_E_CAPACITY;
    if(ds!=RE_DTRZ_OK)return RE_GUEST_XE3U_DECODE_E_LZMA;
    hs=re_guest_xe3u_parse_header(destination,decoded,&pending,&failed);
    if(hs!=RE_GUEST_XE3U_OK) {
        if(failed_field)*failed_field=failed;
        return RE_GUEST_XE3U_DECODE_E_HEADER;
    }
    *header=pending;
    *written=decoded;
    return RE_GUEST_XE3U_DECODE_OK;
}
