#ifndef RE_GUEST_ELF32_PIPELINE_H
#define RE_GUEST_ELF32_PIPELINE_H
#include "re_guest_elf32_batch_loader.h"
#include "re_guest_elf32_module_graph.h"
#include "re_guest_elf32_provider_catalog.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const char *module_name; /* Root fallback name; non-root modules require DT_SONAME. */
    const uint8_t *elf_bytes;
    size_t elf_size;
    uint32_t load_bias;
    const REGUESTELF32LOADBUFFER *buffers;
    size_t buffer_count;
    REGUESTELF32LINKSYMBOL *symbols;
    size_t symbol_capacity;
    REGUESTELF32LINKRELOCATION *decoded_relocations;
    size_t decoded_relocation_capacity;
    REGUESTRELOCATION *prepared_relocations;
    size_t prepared_relocation_capacity;
} REGUESTELF32PIPELINEMODULE;
typedef struct {
    REGUESTELF32DYNAMIC dynamic;
    REGUESTELF32IMAGE image;
    REGUESTELF32LINKINFO link;
} REGUESTELF32PIPELINEMETADATA;
typedef struct {
    REGUESTELF32PIPELINEMETADATA metadata[RE_GUEST_ELF32_MAX_MODULES];
    REGUESTELF32MODULEGRAPH graph;
    REGUESTELF32BATCHRESULT loaded;
    size_t module_count;
    size_t provider_count;
} REGUESTELF32PIPELINERESULT;
typedef enum {
    RE_GUEST_ELF32_PIPELINE_OK=0,
    RE_GUEST_ELF32_PIPELINE_E_ARGUMENT=-770,
    RE_GUEST_ELF32_PIPELINE_E_METADATA=-771,
    RE_GUEST_ELF32_PIPELINE_E_GRAPH=-772,
    RE_GUEST_ELF32_PIPELINE_E_PROVIDER=-773,
    RE_GUEST_ELF32_PIPELINE_E_RELOCATION=-774,
    RE_GUEST_ELF32_PIPELINE_E_BATCH=-775
} REGUESTELF32PIPELINEStatus;
/* Caller supplies the complete candidate module inventory and all scratch
 * buffers. The root's module_name is its inventory identity when DT_SONAME
 * is absent. Other modules must provide DT_SONAME. Only the root-reachable
 * dependency closure is linked and loaded. No filesystem search or host
 * dynamic linker is used. */
REGUESTELF32PIPELINEStatus re_guest_elf32_load_pipeline(
    REGUESTMEMORY *memory,const REGUESTELF32PIPELINEMODULE *modules,
    size_t module_count,const char *root_name,
    REGUESTSYMBOL *provider_scratch,size_t provider_capacity,
    REGUESTELF32PIPELINERESULT *result,size_t *failed_module,
    size_t *failed_index,int *inner_status);
#ifdef __cplusplus
}
#endif
#endif
