#ifndef RE_GUEST_XE3U_DECODE_H
#define RE_GUEST_XE3U_DECODE_H
#include "re_guest_xe3u_image.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    RE_GUEST_XE3U_DECODE_OK=0,
    RE_GUEST_XE3U_DECODE_E_ARGUMENT=-830,
    RE_GUEST_XE3U_DECODE_E_LZMA=-831,
    RE_GUEST_XE3U_DECODE_E_MEMORY=-832,
    RE_GUEST_XE3U_DECODE_E_CAPACITY=-833,
    RE_GUEST_XE3U_DECODE_E_HEADER=-834
} REGUESTXE3UDECODEStatus;
/* Decode an LZMA-alone S3E stream into caller-owned storage, then validate
 * the XE3U header. Requires the optional liblzma component at link time.
 * Header and written outputs are published only after both steps succeed.
 */
REGUESTXE3UDECODEStatus re_guest_xe3u_decode_lzma_alone(
    const uint8_t *source,size_t source_size,uint8_t *destination,
    size_t destination_capacity,uint64_t decoder_memory_limit,
    size_t *written,REGUESTXE3UHEADER *header,size_t *failed_header_field);
#ifdef __cplusplus
}
#endif
#endif
