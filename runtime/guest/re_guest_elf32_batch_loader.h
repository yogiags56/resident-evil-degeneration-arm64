#ifndef RE_GUEST_ELF32_BATCH_LOADER_H
#define RE_GUEST_ELF32_BATCH_LOADER_H
#include "re_guest_elf32_loader.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_ELF32_MAX_BATCH_MODULES 32u
typedef struct {
    const REGUESTELF32LOADREQUEST *request;
} REGUESTELF32BATCHITEM;
typedef struct {
    REGUESTELF32LOADRESULT modules[RE_GUEST_ELF32_MAX_BATCH_MODULES];
    size_t module_count;
    size_t total_segments;
} REGUESTELF32BATCHRESULT;
typedef enum {
    RE_GUEST_ELF32_BATCH_OK=0,
    RE_GUEST_ELF32_BATCH_E_ARGUMENT=-740,
    RE_GUEST_ELF32_BATCH_E_CAPACITY=-741,
    RE_GUEST_ELF32_BATCH_E_IMAGE=-742,
    RE_GUEST_ELF32_BATCH_E_LAYOUT=-743,
    RE_GUEST_ELF32_BATCH_E_LOAD=-744,
    RE_GUEST_ELF32_BATCH_E_ROLLBACK=-745,
    RE_GUEST_ELF32_BATCH_E_MEMORY_STATE=-746
} REGUESTELF32BATCHStatus;
/* Caller supplies modules in dependency-first order and fully prepared
 * relocations/provider addresses. The whole guest mapping table is restored
 * if any member fails. Borrowed segment buffers may have changed on failure
 * and must be discarded/reinitialized before retry.
 */
REGUESTELF32BATCHStatus re_guest_elf32_load_batch(
    REGUESTMEMORY *memory,const REGUESTELF32BATCHITEM *items,size_t count,
    REGUESTELF32BATCHRESULT *result,size_t *failed_module,
    size_t *failed_segment,size_t *failed_relocation,int *inner_status);
#ifdef __cplusplus
}
#endif
#endif
