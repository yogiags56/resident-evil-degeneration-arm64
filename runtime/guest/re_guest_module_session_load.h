#ifndef RE_GUEST_MODULE_SESSION_LOAD_H
#define RE_GUEST_MODULE_SESSION_LOAD_H
#include "re_guest_module_session.h"
#include "re_guest_elf32_pipeline.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
 RE_GUEST_SESSION_LOAD_OK=0,
 RE_GUEST_SESSION_LOAD_E_ARGUMENT=-920,
 RE_GUEST_SESSION_LOAD_E_NOT_READY=-921,
 RE_GUEST_SESSION_LOAD_E_INVENTORY=-922,
 RE_GUEST_SESSION_LOAD_E_PIPELINE=-923
} REGUESTMODULESESSIONLOADSTATUS;
/* Uses only the exact ELF buffers owned by a READY preparation session.
 * Caller supplies guest memory, per-module segment/symbol/relocation scratch,
 * provider scratch, and load biases. On success session becomes LOADED.
 * On failure the session remains READY; discard/reinitialize borrowed scratch
 * buffers before retry. Pipeline's batch loader restores the guest map table
 * on failure. This API does not start the guest CPU or call constructors.
 */
REGUESTMODULESESSIONLOADSTATUS re_guest_module_session_load(
 REGUESTMODULESESSION *session, REGUESTMEMORY *memory,
 const REGUESTELF32PIPELINEMODULE *modules,size_t module_count,
 const char *root_name,REGUESTSYMBOL *provider_scratch,
 size_t provider_capacity,REGUESTELF32PIPELINERESULT *result,
 size_t *failed_module,size_t *failed_index,int *inner_status);
#ifdef __cplusplus
}
#endif
#endif
