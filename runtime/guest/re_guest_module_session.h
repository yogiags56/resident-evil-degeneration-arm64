#ifndef RE_GUEST_MODULE_SESSION_H
#define RE_GUEST_MODULE_SESSION_H
#include "re_guest_module_preflight.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
 RE_GUEST_SESSION_EMPTY=0,
 RE_GUEST_SESSION_READY=1,
 RE_GUEST_SESSION_LOADED=2
} REGUESTMODULESESSIONSTATE;
typedef struct {
 REGUESTMODULESESSIONSTATE state;
 REGUESTMODULEMANIFEST manifest;
 REGUESTMATERIALIZEDSET materialized;
 REGUESTMODULEPREFLIGHT preflight;
} REGUESTMODULESESSION;
typedef enum {
 RE_GUEST_SESSION_OK=0,
 RE_GUEST_SESSION_E_ARGUMENT=-900,
 RE_GUEST_SESSION_E_MANIFEST=-901,
 RE_GUEST_SESSION_E_MATERIALIZE=-902,
 RE_GUEST_SESSION_E_PREFLIGHT=-903,
 RE_GUEST_SESSION_E_NOT_READY=-904
} REGUESTMODULESESSIONSTATUS;
/* Build a complete, caller-owned preparation session transactionally.
 * On failure, out remains unchanged. No guest mapping, relocation,
 * constructors, CPU initialization, or execution is performed.
 * out must be zero-initialized and released before reuse.
 */
REGUESTMODULESESSIONSTATUS re_guest_module_session_prepare(
 const char *manifest_text,size_t manifest_length,
 const REGUESTMODULESOURCE *source,void *source_user,
 REGUESTMODULESESSION *out,size_t *failed_module,size_t *failed_line);
void re_guest_module_session_release(REGUESTMODULESESSION *session);
#ifdef __cplusplus
}
#endif
#endif
