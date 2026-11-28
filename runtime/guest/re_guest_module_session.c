#include "re_guest_module_session.h"
#include <string.h>
REGUESTMODULESESSIONSTATUS re_guest_module_session_prepare(
 const char *manifest_text,size_t manifest_length,
 const REGUESTMODULESOURCE *source,void *source_user,
 REGUESTMODULESESSION *out,size_t *failed_module,size_t *failed_line) {
 REGUESTMODULESESSION pending;
 REGUESTMODULEMANIFESTStatus ms;
 REGUESTMATERIALIZESTATUS materialize_status;
 REGUESTMODULEPREFLIGHTSTATUS preflight_status;
 size_t failed=0u;
 if(failed_module)*failed_module=0u;
 if(failed_line)*failed_line=0u;
 if(!manifest_text||!manifest_length||!source||!out||
    out->state!=RE_GUEST_SESSION_EMPTY||out->materialized.module_count!=0u)
  return RE_GUEST_SESSION_E_ARGUMENT;
 for(size_t i=0;i<RE_GUEST_MANIFEST_MAX_MODULES;i++)
  if(out->materialized.modules[i].bytes||out->materialized.modules[i].size)
   return RE_GUEST_SESSION_E_ARGUMENT;
 memset(&pending,0,sizeof(pending));
 ms=re_guest_module_manifest_parse(manifest_text,manifest_length,
     &pending.manifest,failed_line);
 if(ms!=RE_GUEST_MANIFEST_OK)return RE_GUEST_SESSION_E_MANIFEST;
 materialize_status=re_guest_module_materialize(&pending.manifest,source,
     source_user,&pending.materialized);
 if(materialize_status!=RE_GUEST_MATERIALIZE_OK) {
  re_guest_module_materialized_free(&pending.materialized);
  return RE_GUEST_SESSION_E_MATERIALIZE;
 }
 preflight_status=re_guest_module_preflight(&pending.manifest,
     &pending.materialized,&pending.preflight,&failed);
 if(preflight_status!=RE_GUEST_PREFLIGHT_OK) {
  if(failed_module)*failed_module=failed;
  re_guest_module_materialized_free(&pending.materialized);
  return RE_GUEST_SESSION_E_PREFLIGHT;
 }
 pending.state=RE_GUEST_SESSION_READY;
 *out=pending;
 return RE_GUEST_SESSION_OK;
}
void re_guest_module_session_release(REGUESTMODULESESSION *session) {
 if(!session)return;
 re_guest_module_materialized_free(&session->materialized);
 memset(session,0,sizeof(*session));
}
