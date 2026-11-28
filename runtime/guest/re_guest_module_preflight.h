#ifndef RE_GUEST_MODULE_PREFLIGHT_H
#define RE_GUEST_MODULE_PREFLIGHT_H
#include "re_guest_elf32_module_graph.h"
#include "re_guest_module_materializer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
 RE_GUEST_PREFLIGHT_OK=0,
 RE_GUEST_PREFLIGHT_E_ARGUMENT=-880,
 RE_GUEST_PREFLIGHT_E_MANIFEST=-881,
 RE_GUEST_PREFLIGHT_E_IDENTITY=-882,
 RE_GUEST_PREFLIGHT_E_ROOT_SONAME=-883,
 RE_GUEST_PREFLIGHT_E_GRAPH=-884,
 RE_GUEST_PREFLIGHT_E_MEMORY=-885
} REGUESTMODULEPREFLIGHTSTATUS;
typedef struct {
 size_t module_count;
 size_t root_module_index;
 REGUESTELF32MODULEGRAPH graph;
} REGUESTMODULEPREFLIGHT;
/* Validates every materialized ELF identity and closes the DT_NEEDED graph.
 * Does not map, relocate, initialize, or execute any guest module.
 * Output is committed only when every check succeeds.
 */
REGUESTMODULEPREFLIGHTSTATUS re_guest_module_preflight(
 const REGUESTMODULEMANIFEST *manifest,
 const REGUESTMATERIALIZEDSET *materialized,
 REGUESTMODULEPREFLIGHT *out, size_t *failed_module);
#ifdef __cplusplus
}
#endif
#endif
