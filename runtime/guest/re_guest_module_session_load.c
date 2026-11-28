#include "re_guest_module_session_load.h"
#include <string.h>
REGUESTMODULESESSIONLOADSTATUS re_guest_module_session_load(
 REGUESTMODULESESSION *session,REGUESTMEMORY *memory,
 const REGUESTELF32PIPELINEMODULE *modules,size_t module_count,
 const char *root_name,REGUESTSYMBOL *provider_scratch,
 size_t provider_capacity,REGUESTELF32PIPELINERESULT *result,
 size_t *failed_module,size_t *failed_index,int *inner_status) {
 REGUESTELF32PIPELINERESULT pending;
 size_t root=session?session->manifest.root_module_index:0u;
 if(failed_module)*failed_module=0u;
 if(failed_index)*failed_index=0u;
 if(inner_status)*inner_status=0;
 if(!session||!memory||!modules||!root_name||!result||
    (!provider_scratch&&provider_capacity))
  return RE_GUEST_SESSION_LOAD_E_ARGUMENT;
 if(session->state!=RE_GUEST_SESSION_READY)
  return RE_GUEST_SESSION_LOAD_E_NOT_READY;
 if(module_count!=session->manifest.module_count||
    root>=module_count||strcmp(root_name,session->manifest.modules[root].soname)!=0)
  return RE_GUEST_SESSION_LOAD_E_INVENTORY;
 for(size_t i=0;i<module_count;i++) {
  const REGUESTMODULEMANIFESTENTRY *entry=&session->manifest.modules[i];
  const REGUESTMATERIALIZEDMODULE *asset=&session->materialized.modules[i];
  if(!modules[i].module_name||strcmp(modules[i].module_name,entry->soname)!=0||
     modules[i].elf_bytes!=asset->bytes||modules[i].elf_size!=asset->size)
   return RE_GUEST_SESSION_LOAD_E_INVENTORY;
 }
 memset(&pending,0,sizeof(pending));
 REGUESTELF32PIPELINEStatus status=re_guest_elf32_load_pipeline(
  memory,modules,module_count,root_name,provider_scratch,provider_capacity,
  &pending,failed_module,failed_index,inner_status);
 if(status!=RE_GUEST_ELF32_PIPELINE_OK)return RE_GUEST_SESSION_LOAD_E_PIPELINE;
 *result=pending;
 session->state=RE_GUEST_SESSION_LOADED;
 return RE_GUEST_SESSION_LOAD_OK;
}
