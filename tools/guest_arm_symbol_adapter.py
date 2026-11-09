#!/usr/bin/env python3
"""Build explicit symbol-provider and unresolved-request inventories from audited ELF JSON."""
from __future__ import annotations
import argparse, json
from pathlib import Path

class SymbolAdapterError(ValueError):
    pass

def adapt(report):
    if not isinstance(report, dict):
        raise SymbolAdapterError("audit report must be an object")
    symbols=report.get("dynamic_symbols")
    versions=report.get("symbol_versioning", {})
    if not isinstance(symbols,list) or not report.get("symbol_resolution",{}).get("available"):
        raise SymbolAdapterError("bounded dynamic symbol inventory is unavailable")
    needed=report.get("needed_libraries", [])
    if not isinstance(needed,list) or any(not isinstance(x,str) or not x for x in needed):
        raise SymbolAdapterError("invalid DT_NEEDED inventory")
    providers=[]
    requests=[]
    for sym in symbols:
        if not isinstance(sym,dict) or not isinstance(sym.get("name"),str):
            raise SymbolAdapterError("malformed dynamic symbol descriptor")
        name=sym["name"]
        if not name:
            continue
        binding=sym.get("binding")
        visibility=sym.get("visibility")
        version=sym.get("version")
        if binding not in (0,1,2) or visibility not in (0,1,2,3):
            raise SymbolAdapterError(f"symbol {name}: invalid binding or visibility")
        if sym.get("undefined"):
            if binding==0:
                continue
            if binding not in (1,2):
                raise SymbolAdapterError(f"symbol {name}: unsupported undefined binding")
            if visibility in (1,2):
                continue
            version_name=None
            library=None
            if version:
                if version.get("kind")=="requirement":
                    version_name=version.get("name")
                    library=version.get("library")
                elif version.get("kind") not in ("global","local"):
                    raise SymbolAdapterError(f"symbol {name}: unknown version requirement kind")
            requests.append({
                "symbol_index":sym["index"],"name":name,"binding":binding,
                "version_name":version_name,"version_library":library,
                "version_hidden":bool(version and version.get("hidden")),
                "weak":binding==2
            })
        else:
            if binding not in (1,2) or visibility in (1,2):
                continue
            value=sym.get("value")
            if not isinstance(value,str) or not value.startswith("0x"):
                raise SymbolAdapterError(f"symbol {name}: malformed value")
            providers.append({
                "symbol_index":sym["index"],"name":name,"value":int(value,16),
                "binding":binding,"visibility":visibility,
                "version_name":version.get("name") if version and version.get("kind")=="definition" else None,
                "version_hidden":bool(version and version.get("hidden"))
            })
    return {
        "schema":"re-guest-symbol-adapter-v1",
        "source_sha256":report.get("sha256"),
        "needed_libraries":needed,
        "provider_definitions":providers,
        "undefined_requests":requests,
        "limits":[
            "This is an inventory adapter, not a runtime dependency loader.",
            "DT_NEEDED names are preserved in original order; no filesystem search is performed.",
            "Version indices are already decoded by the ELF audit; no provider compatibility is inferred.",
            "Symbol values remain ELF virtual values and must receive the provider's verified load bias before relocation."
        ]
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("audit_json",type=Path); p.add_argument("--json",type=Path)
    a=p.parse_args()
    try:
        result=adapt(json.loads(a.audit_json.read_text(encoding="utf-8")))
        rendered=json.dumps(result,indent=2)
        if a.json:
            a.json.parent.mkdir(parents=True,exist_ok=True)
            a.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,json.JSONDecodeError,SymbolAdapterError) as exc:
        p.error(str(exc))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
