#include "re_guest_elf32_module_graph.h"
#include <string.h>
static int valid_name(const char *s) {
    if (!s || !*s || strchr(s,'/') || strchr(s,'\\')) return 0;
    return strlen(s)<RE_GUEST_ELF32_SONAME_CAP;
}
static size_t find_module(const REGUESTELF32MODULEDESC *m,size_t n,const char *name) {
    for(size_t i=0;i<n;i++) if(strcmp(m[i].soname,name)==0) return i;
    return n;
}
typedef struct {
    const REGUESTELF32MODULEDESC *modules;
    size_t count;
    REGUESTELF32MODULEGRAPH *graph;
    uint8_t state[RE_GUEST_ELF32_MAX_MODULES];
    uint8_t edge_seen[RE_GUEST_ELF32_MAX_MODULES][RE_GUEST_ELF32_MAX_MODULES];
} WALK;
static void visit(WALK *w,size_t index) {
    w->state[index]=1u;
    w->graph->reachable[index]=1u;
    const REGUESTELF32MODULEDESC *m=&w->modules[index];
    for(size_t k=0;k<m->needed_count;k++) {
        size_t dep=find_module(w->modules,w->count,m->needed[k]);
        if(dep==w->count) {
            size_t q=w->graph->unresolved_count++;
            w->graph->unresolved[q].module_index=index;
            w->graph->unresolved[q].needed_ordinal=k;
            continue;
        }
        if(w->edge_seen[index][dep]) continue;
        w->edge_seen[index][dep]=1u;
        if(w->state[dep]==1u) { w->graph->has_cycle=1u; continue; }
        if(w->state[dep]==0u) visit(w,dep);
    }
    w->state[index]=2u;
    w->graph->order[w->graph->order_count++]=index;
}
REGUESTELF32MODULEGRAPHStatus re_guest_elf32_build_module_graph(
    const REGUESTELF32MODULEDESC *modules,size_t count,const char *root,
    REGUESTELF32MODULEGRAPH *graph,size_t *failed) {
    size_t root_index;
    WALK w;
    if(failed)*failed=0;
    if(graph)memset(graph,0,sizeof(*graph));
    if(!modules||!count||count>RE_GUEST_ELF32_MAX_MODULES||!valid_name(root)||!graph)
        return RE_GUEST_ELF32_MODULE_GRAPH_E_ARGUMENT;
    for(size_t i=0;i<count;i++) {
        if(!valid_name(modules[i].soname)||modules[i].needed_count>RE_GUEST_ELF32_MAX_MODULE_NEEDED||
           (!modules[i].needed&&modules[i].needed_count)) {
            if(failed)*failed=i;
            return RE_GUEST_ELF32_MODULE_GRAPH_E_NAME;
        }
        for(size_t k=0;k<modules[i].needed_count;k++) if(!valid_name(modules[i].needed[k])) {
            if(failed)*failed=i;
            return RE_GUEST_ELF32_MODULE_GRAPH_E_NAME;
        }
        for(size_t j=0;j<i;j++) if(strcmp(modules[i].soname,modules[j].soname)==0) {
            if(failed)*failed=i;
            return RE_GUEST_ELF32_MODULE_GRAPH_E_DUPLICATE;
        }
    }
    root_index=find_module(modules,count,root);
    if(root_index==count)return RE_GUEST_ELF32_MODULE_GRAPH_E_ROOT;
    memset(&w,0,sizeof(w));w.modules=modules;w.count=count;w.graph=graph;
    visit(&w,root_index);
    graph->complete=(uint8_t)(!graph->unresolved_count&&!graph->has_cycle);
    if(graph->has_cycle)return RE_GUEST_ELF32_MODULE_GRAPH_E_CYCLE;
    if(graph->unresolved_count)return RE_GUEST_ELF32_MODULE_GRAPH_E_UNRESOLVED;
    return RE_GUEST_ELF32_MODULE_GRAPH_OK;
}
