#!/usr/bin/env python3
"""Audit original libjirobiodg.so DT_NEEDED and relocation-backed provider coverage."""
from __future__ import annotations
import argparse, json
from pathlib import Path

class AuditError(ValueError):
    pass

def audit(manifest: dict, policy: dict) -> dict:
    if not isinstance(manifest, dict) or not isinstance(policy, dict):
        raise AuditError("manifest and policy must be JSON objects")
    needed = manifest.get("dt_needed_in_order")
    expected = policy.get("expected_dt_needed_in_order")
    if not isinstance(needed, list) or not all(isinstance(x, str) and x for x in needed):
        raise AuditError("manifest DT_NEEDED inventory is invalid")
    if needed != expected:
        raise AuditError("DT_NEEDED order does not match the hash-pinned provider policy")
    reloc = manifest.get("relocations", {})
    required = reloc.get("unique_undefined_symbol_names")
    if not isinstance(required, list) or not all(isinstance(x, str) and x for x in required):
        raise AuditError("relocation-backed undefined symbol inventory is invalid")
    if len(required) != len(set(required)):
        raise AuditError("relocation-backed symbol inventory contains duplicates")
    soname_policy = policy.get("soname_policy")
    abi_policy = policy.get("abi_families")
    if not isinstance(soname_policy, dict) or not isinstance(abi_policy, dict):
        raise AuditError("provider policy sections are missing")
    if set(soname_policy) != set(needed):
        raise AuditError("every direct DT_NEEDED entry must have exactly one policy")
    mapped = {}
    for soname in needed:
        item = soname_policy[soname]
        if not isinstance(item, dict) or not isinstance(item.get("provided_symbols"), list):
            raise AuditError(f"{soname}: invalid provider symbol inventory")
        mode = item.get("mode")
        if mode not in {"guest_provider", "partial_guest_provider", "dependency_only"}:
            raise AuditError(f"{soname}: unsupported policy mode")
        if mode == "dependency_only" and item["provided_symbols"]:
            raise AuditError(f"{soname}: dependency-only entry cannot claim exports")
        for symbol in item["provided_symbols"]:
            if not isinstance(symbol, str) or not symbol:
                raise AuditError(f"{soname}: invalid provided symbol")
            if symbol in mapped:
                raise AuditError(f"symbol {symbol} assigned to multiple providers")
            mapped[symbol] = soname
    abi_mapped = {}
    for family, item in abi_policy.items():
        if not isinstance(item, dict) or not isinstance(item.get("provided_symbols"), list):
            raise AuditError(f"{family}: invalid ABI-family inventory")
        for symbol in item["provided_symbols"]:
            if not isinstance(symbol, str) or not symbol:
                raise AuditError(f"{family}: invalid symbol")
            if symbol in mapped or symbol in abi_mapped:
                raise AuditError(f"symbol {symbol} assigned to multiple providers")
            abi_mapped[symbol] = family
    required_set = set(required)
    declared = set(mapped) | set(abi_mapped)
    unexpected = sorted(declared - required_set)
    covered = sorted(declared & required_set)
    missing = sorted(required_set - declared)
    libm = soname_policy.get("libm.so", {})
    if libm.get("mode") != "dependency_only" or libm.get("provided_symbols") != []:
        raise AuditError("libm.so must remain an explicit dependency-only policy until verified")
    if libm.get("relocation_symbols") != []:
        raise AuditError("libm.so policy must not invent relocation-backed symbol ownership")
    return {
        "schema": "resident-evil-degeneration-provider-closure-audit/v1",
        "dt_needed_order_matches": True,
        "dt_needed_count": len(needed),
        "dt_needed_policy_coverage": True,
        "dependency_only_sonames": sorted(n for n in needed if soname_policy[n]["mode"] == "dependency_only"),
        "required_unique_relocation_symbols": len(required_set),
        "covered_relocation_symbols": len(covered),
        "uncovered_relocation_symbols": missing,
        "unexpected_declared_provider_symbols": unexpected,
        "provider_symbol_mapping": {s: mapped[s] if s in mapped else abi_mapped[s] for s in covered},
        "soname_dependency_policy_complete": True,
        "relocation_symbol_closure_complete": not missing and not unexpected,
        "production_loader_gate": not missing and not unexpected,
        "limitations": [
            "This is a static inventory audit; it does not load, map, relocate, or execute a DSO.",
            "A dependency-only libm.so policy preserves DT_NEEDED but is not proof of runtime loader satisfaction.",
            "Provider symbol inventories are implemented guest adapters only; all remaining imports must block production activation."
        ]
    }

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("policy", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        policy = json.loads(args.policy.read_text(encoding="utf-8"))
        result = audit(manifest, policy)
        rendered = json.dumps(result, indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(rendered + "\\n", encoding="utf-8")
        print(rendered)
        return 0 if result["production_loader_gate"] else 2
    except (OSError, json.JSONDecodeError, AuditError) as exc:
        parser.error(str(exc))

if __name__ == "__main__":
    raise SystemExit(main())
