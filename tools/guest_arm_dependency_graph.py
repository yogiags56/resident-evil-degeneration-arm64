#!/usr/bin/env python3
"""Resolve an explicit ELF DT_NEEDED inventory into a deterministic dependency graph."""
from __future__ import annotations
import argparse, json
from pathlib import Path

class DependencyGraphError(ValueError):
    pass

def build_graph(manifest):
    if not isinstance(manifest,dict):
        raise DependencyGraphError("manifest must be an object")
    raw_nodes=manifest.get("libraries")
    root=manifest.get("root")
    if not isinstance(raw_nodes,list) or not raw_nodes:
        raise DependencyGraphError("libraries must be a non-empty array")
    if not isinstance(root,str) or not root:
        raise DependencyGraphError("root must name a library SONAME")
    by_name={}
    for node in raw_nodes:
        if not isinstance(node,dict):
            raise DependencyGraphError("library entry must be an object")
        soname=node.get("soname")
        adapter=node.get("adapter")
        if not isinstance(soname,str) or not soname or "/" in soname or "\\" in soname:
            raise DependencyGraphError("SONAME must be a non-empty basename")
        if soname in by_name:
            raise DependencyGraphError(f"duplicate provider SONAME: {soname}")
        if not isinstance(adapter,dict):
            raise DependencyGraphError(f"{soname}: adapter inventory must be an object")
        needed=adapter.get("needed_libraries")
        if not isinstance(needed,list) or any(not isinstance(x,str) or not x for x in needed):
            raise DependencyGraphError(f"{soname}: invalid DT_NEEDED inventory")
        by_name[soname]={"soname":soname,"needed":needed,"adapter":adapter}
    if root not in by_name:
        raise DependencyGraphError(f"root library not supplied: {root}")
    edges=[]
    unresolved=[]
    reachable=set()
    visiting=[]
    visited=set()
    cycles=[]
    def visit(name):
        if name in visiting:
            start=visiting.index(name)
            cycle=visiting[start:]+[name]
            if cycle not in cycles:
                cycles.append(cycle)
            return
        if name in visited:
            return
        visiting.append(name)
        reachable.add(name)
        node=by_name[name]
        seen_edges=set()
        for ordinal,dep in enumerate(node["needed"]):
            if dep not in by_name:
                unresolved.append({"from":name,"needed":dep,"ordinal":ordinal})
                continue
            if dep not in seen_edges:
                edges.append({"from":name,"to":dep,"ordinal":ordinal})
                seen_edges.add(dep)
            visit(dep)
        visiting.pop()
        visited.add(name)
    visit(root)
    # Postorder means dependencies precede the library that needs them.
    load_order=[]
    ordered=set()
    def order(name):
        if name in ordered:
            return
        ordered.add(name)
        for dep in by_name[name]["needed"]:
            if dep in reachable and dep not in ordered:
                order(dep)
        load_order.append(name)
    if not cycles:
        order(root)
    return {
        "schema":"re-guest-dependency-graph-v1",
        "root":root,
        "nodes":[{"soname":name,"needed_libraries":by_name[name]["needed"]} for name in sorted(reachable)],
        "edges":edges,
        "unresolved_dependencies":unresolved,
        "cycles":cycles,
        "dependency_first_load_order":load_order,
        "complete":not unresolved and not cycles,
        "limits":[
            "Only caller-supplied adapter inventories are considered; no filesystem or host linker search is performed.",
            "SONAME uniqueness is required; duplicate providers are rejected rather than guessed.",
            "The order is a deterministic dependency-first graph order, not a complete Android/Bionic linker namespace policy.",
            "No library is opened, mapped, relocated, or executed."
        ]
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("manifest",type=Path); p.add_argument("--json",type=Path)
    a=p.parse_args()
    try:
        result=build_graph(json.loads(a.manifest.read_text(encoding="utf-8")))
        rendered=json.dumps(result,indent=2)
        if a.json:
            a.json.parent.mkdir(parents=True,exist_ok=True)
            a.json.write_text(rendered+"\\n",encoding="utf-8")
        print(rendered)
        return 0 if result["complete"] else 2
    except (OSError,json.JSONDecodeError,DependencyGraphError) as exc:
        p.error(str(exc))

if __name__=="__main__":
    raise SystemExit(main())
