#include "re_guest_module_preflight.h"
#include "re_guest_elf32_identity.h"
#include <stdlib.h>
#include <string.h>
REGUESTMODULEPREFLIGHTSTATUS re_guest_module_preflight(
 const REGUESTMODULEMANIFEST *manifest,
 const REGUESTMATERIALIZEDSET *materialized,
 REGUESTMODULEPREFLIGHT *out, size_t *failed_module) {
 REGUESTMODULEPREFLIGHT pending;
 REGUESTELF32IDENTITY *identities;
 REGUESTELF32MODULEDESC desc[RE_GUEST_ELF32_MAX_MODULES];
 const char *needed[RE_GUEST_ELF32_MAX_MODULES][RE_GUEST_ELF32_MAX_MODULE_NEEDED];
 size_t graph_failed=0u;
 REGUESTELF32MODULEGRAPHStatus graph_status;
 if(failed_module)*failed_module=0u;
 if(!manifest||!materialized||!out)return RE_GUEST_PREFLIGHT_E_ARGUMENT;
 if(manifest->version!=1u||!manifest->module_count||
    manifest->module_count>RE_GUEST_ELF32_MAX_MODULES||
    manifest->root_module_index>=manifest->module_count||
    materialized->module_count!=manifest->module_count)
  return RE_GUEST_PREFLIGHT_E_MANIFEST;
 memset(&pending,0,sizeof(pending));
 memset(desc,0,sizeof(desc));
 memset(needed,0,sizeof(needed));
 identities=(REGUESTELF32IDENTITY*)calloc(manifest->module_count,sizeof(*identities));
 if(!identities)return RE_GUEST_PREFLIGHT_E_MEMORY;
 for(size_t i=0;i<manifest->module_count;i++){
  const REGUESTMODULEMANIFESTENTRY *entry=&manifest->modules[i];
  const REGUESTMATERIALIZEDMODULE *module=&materialized->modules[i];
  const int is_root=(i==manifest->root_module_index);
  REGUESTELF32IDENTITYTYPE expected_type=is_root?
   RE_GUEST_ELF32_IDENTITY_ANY:RE_GUEST_ELF32_IDENTITY_SHARED;
  REGUESTELF32IDENTITYSTATUS status;
  if(!module->bytes||!module->size||!memchr(entry->soname,'\\0',sizeof(entry->soname))){
   if(failed_module)*failed_module=i;
   free(identities);return RE_GUEST_PREFLIGHT_E_MANIFEST;
  }
  status=re_guest_elf32_verify_identity(module->bytes,module->size,
    is_root?NULL:entry->soname,expected_type,&identities[i],NULL);
  if(status!=RE_GUEST_ELF32_IDENTITY_OK){
   if(failed_module)*failed_module=i;
   free(identities);return RE_GUEST_PREFLIGHT_E_IDENTITY;
  }
  if(is_root&&identities[i].dynamic.has_soname&&
     strcmp(identities[i].dynamic.soname,entry->soname)!=0){
   if(failed_module)*failed_module=i;
   free(identities);return RE_GUEST_PREFLIGHT_E_ROOT_SONAME;
  }
  if(identities[i].dynamic.needed_count>RE_GUEST_ELF32_MAX_MODULE_NEEDED){
   if(failed_module)*failed_module=i;
   free(identities);return RE_GUEST_PREFLIGHT_E_MANIFEST;
  }
  for(size_t n=0;n<identities[i].dynamic.needed_count;n++)
   needed[i][n]=identities[i].dynamic.needed[n];
  desc[i].soname=entry->soname;
  desc[i].needed=needed[i];
  desc[i].needed_count=identities[i].dynamic.needed_count;
 }
 graph_status=re_guest_elf32_build_module_graph(desc,manifest->module_count,
  manifest->modules[manifest->root_module_index].soname,&pending.graph,&graph_failed);
 if(graph_status!=RE_GUEST_ELF32_MODULE_GRAPH_OK){
  if(failed_module&&graph_failed<manifest->module_count)*failed_module=graph_failed;
  free(identities);return RE_GUEST_PREFLIGHT_E_GRAPH;
 }
 pending.module_count=manifest->module_count;
 pending.root_module_index=manifest->root_module_index;
 *out=pending;
 free(identities);
 return RE_GUEST_PREFLIGHT_OK;
}
