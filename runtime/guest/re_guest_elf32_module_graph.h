#ifndef RE_GUEST_ELF32_MODULE_GRAPH_H
#define RE_GUEST_ELF32_MODULE_GRAPH_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_ELF32_MAX_MODULES 32u
#define RE_GUEST_ELF32_MAX_MODULE_NEEDED 32u
#define RE_GUEST_ELF32_SONAME_CAP 256u
typedef struct {
    const char *soname;
    const char *const *needed;
    size_t needed_count;
} REGUESTELF32MODULEDESC;
typedef struct {
    size_t module_index;
    size_t needed_ordinal;
} REGUESTELF32UNRESOLVED;
typedef struct {
    size_t order[RE_GUEST_ELF32_MAX_MODULES];
    size_t order_count;
    REGUESTELF32UNRESOLVED unresolved[RE_GUEST_ELF32_MAX_MODULES * RE_GUEST_ELF32_MAX_MODULE_NEEDED];
    size_t unresolved_count;
    uint8_t reachable[RE_GUEST_ELF32_MAX_MODULES];
    uint8_t has_cycle;
    uint8_t complete;
} REGUESTELF32MODULEGRAPH;
typedef enum {
    RE_GUEST_ELF32_MODULE_GRAPH_OK = 0,
    RE_GUEST_ELF32_MODULE_GRAPH_E_ARGUMENT = -720,
    RE_GUEST_ELF32_MODULE_GRAPH_E_CAPACITY = -721,
    RE_GUEST_ELF32_MODULE_GRAPH_E_NAME = -722,
    RE_GUEST_ELF32_MODULE_GRAPH_E_DUPLICATE = -723,
    RE_GUEST_ELF32_MODULE_GRAPH_E_ROOT = -724,
    RE_GUEST_ELF32_MODULE_GRAPH_E_UNRESOLVED = -725,
    RE_GUEST_ELF32_MODULE_GRAPH_E_CYCLE = -726
} REGUESTELF32MODULEGRAPHStatus;
/* Resolve only caller-supplied modules. Output order is dependency-first.
 * Repeated DT_NEEDED entries are preserved by the input but traversed once.
 * No files are opened, mapped, relocated, or executed.
 */
REGUESTELF32MODULEGRAPHStatus re_guest_elf32_build_module_graph(
    const REGUESTELF32MODULEDESC *modules, size_t module_count,
    const char *root_soname, REGUESTELF32MODULEGRAPH *graph,
    size_t *failed_index);
#ifdef __cplusplus
}
#endif
#endif
