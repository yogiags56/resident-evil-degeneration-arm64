#include "re_guest_elf32_module_graph.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    const char *root_deps[]={"libc.so","libgfx.so","libc.so"};
    const char *gfx_deps[]={"libmath.so"};
    const char *math_deps[]={0};
    REGUESTELF32MODULEDESC modules[]={
        {"game.so",root_deps,3},
        {"libc.so",NULL,0},
        {"libgfx.so",gfx_deps,1},
        {"libmath.so",NULL,0}
    };
    REGUESTELF32MODULEGRAPH graph;
    size_t failed=99;
    assert(re_guest_elf32_build_module_graph(modules,4,"game.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_OK);
    assert(graph.complete&&graph.order_count==4&&graph.unresolved_count==0&&!graph.has_cycle);
    assert(graph.order[0]==1&&graph.order[1]==3&&graph.order[2]==2&&graph.order[3]==0);
    assert(graph.reachable[0]&&graph.reachable[1]&&graph.reachable[2]&&graph.reachable[3]);
    (void)math_deps;

    const char *missing[]={"libmissing.so"};
    REGUESTELF32MODULEDESC unresolved[]={{"game.so",missing,1}};
    assert(re_guest_elf32_build_module_graph(unresolved,1,"game.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_E_UNRESOLVED);
    assert(!graph.complete&&graph.unresolved_count==1&&graph.unresolved[0].module_index==0&&graph.unresolved[0].needed_ordinal==0);

    const char *a_dep[]={"b.so"},*b_dep[]={"a.so"};
    REGUESTELF32MODULEDESC cycle[]={{"a.so",a_dep,1},{"b.so",b_dep,1}};
    assert(re_guest_elf32_build_module_graph(cycle,2,"a.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_E_CYCLE);
    assert(graph.has_cycle&&!graph.complete);

    REGUESTELF32MODULEDESC duplicate[]={{"a.so",NULL,0},{"a.so",NULL,0}};
    assert(re_guest_elf32_build_module_graph(duplicate,2,"a.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_E_DUPLICATE);
    assert(failed==1);
    REGUESTELF32MODULEDESC path[]={{"dir/a.so",NULL,0}};
    assert(re_guest_elf32_build_module_graph(path,1,"dir/a.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_E_ARGUMENT);
    assert(re_guest_elf32_build_module_graph(modules,4,"missing-root.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_E_ROOT);

    /* Fixed-size unterminated SONAME and whitespace must fail closed. */
    {
        char bad_soname[RE_GUEST_ELF32_SONAME_CAP];
        memset(bad_soname,'A',sizeof(bad_soname));
        REGUESTELF32MODULEDESC bad[]={{bad_soname,NULL,0}};
        assert(re_guest_elf32_build_module_graph(bad,1,"game.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_E_NAME);
        assert(failed==0);
    }
    {
        REGUESTELF32MODULEDESC bad[]={{"lib bad.so",NULL,0}};
        assert(re_guest_elf32_build_module_graph(bad,1,"lib bad.so",&graph,&failed)==RE_GUEST_ELF32_MODULE_GRAPH_E_NAME);
        assert(failed==0);
    }
    puts("Guest ELF32 module graph tests: PASS");
    return 0;
}
