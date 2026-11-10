#!/usr/bin/env python3
"""Cross-audit the original XE3U imports against libjirobiodg.so.

This is a read-only join over the repository's bounded XE3U and ELF32 auditors.
It does not load, relocate, initialize, or execute either binary.
"""
from __future__ import annotations
import argparse
import collections
import json
import struct
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import s3e_binary_audit
import guest_arm_elf_audit
import guest_arm_dynamic_import_inventory

class ProviderAuditError(ValueError):
    pass

PT_NAMES = {1: "PT_LOAD", 2: "PT_DYNAMIC", 4: "PT_NOTE",
            0x70000001: "PT_ARM_EXIDX", 0x6474e551: "GNU_STACK"}
PF_NAMES = ((4, "R"), (2, "W"), (1, "X"))
STT_TLS = 6
SHN_ABS = 0xfff1
INIT_TAGS = {"DT_INIT", "DT_FINI", "DT_INIT_ARRAY",
             "DT_INIT_ARRAYSZ", "DT_FINI_ARRAY", "DT_FINI_ARRAYSZ"}
INIT_SECTIONS = {".init", ".fini", ".init_array", ".fini_array",
                 ".ctors", ".dtors"}

def parse_program_headers(data: bytes) -> list[dict]:
    if len(data) < 52 or data[:4] != b"\x7fELF" or data[4] != 1:
        raise ProviderAuditError("expected ELF32 input")
    endian = "<" if data[5] == 1 else ">" if data[5] == 2 else None
    if endian is None:
        raise ProviderAuditError("unsupported ELF byte order")
    fields = struct.unpack_from(endian + "HHIIIIIHHHHHH", data, 16)
    _, _, _, _, phoff, _, _, ehsize, phentsize, phnum, _, _, _ = fields
    if ehsize < 52 or phentsize < 32 or phoff + phentsize * phnum > len(data):
        raise ProviderAuditError("program-header table out of bounds")
    result = []
    for i in range(phnum):
        off = phoff + i * phentsize
        typ, p_off, vaddr, paddr, filesz, memsz, flags, align = struct.unpack_from(
            endian + "IIIIIIII", data, off)
        if p_off + filesz > len(data) or filesz > memsz:
            raise ProviderAuditError(f"invalid program header {i} file/memory range")
        result.append({
            "index": i, "type": typ, "type_name": PT_NAMES.get(typ, f"PT_{typ:x}"),
            "offset": f"0x{p_off:x}", "vaddr": f"0x{vaddr:x}",
            "file_size": f"0x{filesz:x}", "memory_size": f"0x{memsz:x}",
            "flags": "".join(name for bit, name in PF_NAMES if flags & bit) or "-",
            "alignment": f"0x{align:x}",
        })
    return result

def defined_exports(symbols: list[dict]) -> dict[str, dict]:
    """Match the guest provider-catalog visibility/binding/TLS export policy."""
    out = {}
    for sym in symbols:
        if not sym["name"] or not sym["defined"]:
            continue
        if sym["binding"] not in (1, 2) or sym["visibility"] not in (0, 3):
            continue
        if sym["type"] == STT_TLS:
            continue
        # Preserve ABS values; the runtime provider catalog does not rebase them.
        out.setdefault(sym["name"], sym)
    return out

def compare_imports(import_manifest: list[dict], exports: dict[str, dict]) -> dict:
    matches, missing = [], []
    for item in import_manifest:
        name = item["name"]
        export = exports.get(name)
        row = {
            "symbol_index": item["symbol_index"], "name": name,
            "external_relocation_count": item["external_relocation_count"],
            "cells": item["cells"],
        }
        if export is None:
            missing.append(row)
        else:
            matches.append(row | {
                "elf_value": f"0x{export['value']:08x}",
                "elf_size": export["size"], "elf_type": export["type"],
                "elf_binding": export["binding"],
                "elf_visibility": export["visibility"],
                "elf_section_index": export["section_index"],
                "thumb_state": bool(export["value"] & 1),
            })
    return {"matched": matches, "unmatched": missing}

def build_report(apk_path: Path, s3e_asset: str, elf_asset: str) -> dict:
    try:
        with zipfile.ZipFile(apk_path) as apk:
            s3e_raw = apk.read(s3e_asset)
            elf_bytes = apk.read(elf_asset)
    except (OSError, zipfile.BadZipFile, KeyError) as exc:
        raise ProviderAuditError(f"cannot read required APK assets: {exc}") from exc

    image = s3e_binary_audit.decompress_s3e_stream(s3e_raw)
    s3e_report = s3e_binary_audit.audit_image(image)
    elf_report = guest_arm_elf_audit.audit_elf(elf_bytes)
    dynamic_report = guest_arm_dynamic_import_inventory.inventory(elf_bytes)

    if elf_report["elf_class"] != 32 or elf_report["machine"] != 40:
        raise ProviderAuditError("libjirobiodg asset is not ELF32 EM_ARM")

    dynsym_sections = [s for s in elf_report["sections"] if s["type"] == 11]
    if len(dynsym_sections) != 1:
        raise ProviderAuditError("expected exactly one conventional dynamic symbol table")
    dyn_symbols, _ = guest_arm_dynamic_import_inventory._symbols(
        elf_bytes, elf_report, dynsym_sections[0])
    exports = defined_exports(dyn_symbols)
    intersection = compare_imports(s3e_report["import_manifest"], exports)

    dynamic_entries = [
        entry
        for section in dynamic_report["dynamic_sections"]
        for entry in section["entries"]
    ]
    init_tags = [e for e in dynamic_entries if e["tag_name"] in INIT_TAGS]
    init_sections = [
        {"name": s["name"], "address": s["address"], "size": s["size"]}
        for s in elf_report["sections"] if s["name"] in INIT_SECTIONS
    ]
    relocation_types = collections.Counter()
    for section in dynamic_report["dynamic_relocation_sections"]:
        for entry in section["entries"]:
            relocation_types[entry["type_name"]] += 1

    return {
        "schema": "resident-evil-degeneration-stage123h-provider-audit-v1",
        "source_apk": apk_path.name,
        "elf": {
            "size": len(elf_bytes), "sha256": elf_report["sha256"],
            "class": elf_report["elf_class"], "machine": elf_report["machine_name"],
            "endianness": elf_report["endianness"], "type": elf_report["type"],
            "entry": elf_report["entry"], "flags": elf_report["flags"],
            "program_headers": parse_program_headers(elf_bytes),
            "arm_attributes": elf_report["arm_attributes"],
            "dynamic_symbol_count": sum(
                s["symbol_count"] for s in dynamic_report["dynamic_symbol_tables"]),
            "defined_provider_export_count": len(exports),
            "undefined_dynamic_symbol_count": dynamic_report["undefined_dynamic_symbol_count"],
            "undefined_dynamic_symbols": [
                sym["name"]
                for table in dynamic_report["dynamic_symbol_tables"]
                for sym in table["undefined_symbols"]
            ],
            "needed_libraries": dynamic_report["dynamic_dependencies"],
            "dynamic_relocation_count": dynamic_report["dynamic_relocation_count"],
            "dynamic_relocation_types": dict(sorted(relocation_types.items())),
            "constructor_dynamic_tags": init_tags,
            "constructor_sections": init_sections,
            "has_soname": any(e["tag_name"] == "DT_SONAME" for e in dynamic_entries),
            "has_symbol_version_tags": any(
                e["tag_name"] in {"DT_VERSYM", "DT_VERNEED", "DT_VERNEEDNUM"}
                for e in dynamic_entries),
        },
        "xe3u": {
            "symbol_count": s3e_report["fixups"]["symbol_count"],
            "external_relocation_count": s3e_report["external_relocations"]["count"],
            "verified_arm_import_stubs": s3e_report["external_relocations"]["verified_arm_import_stubs"],
            "zero_cells": s3e_report["external_relocations"]["zero_cells"],
        },
        "provider_intersection": {
            "matched_count": len(intersection["matched"]),
            "unmatched_count": len(intersection["unmatched"]),
            "matched": intersection["matched"],
            "unmatched": intersection["unmatched"],
        },
        "limits": [
            "Exact-name intersection is candidate provider evidence, not proof that a wrapper is callable.",
            "Undefined ELF symbols are not assigned to a specific DT_NEEDED library by this report.",
            "No Android/Bionic namespace search, library load, relocation, constructor, JNI_OnLoad, or guest execution occurs.",
            "ET_DYN load bias is deliberately not selected; ELF st_value values remain link-time values.",
            "The original APK and extracted proprietary assets are not written to the repository.",
        ],
    }

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("apk", type=Path)
    parser.add_argument("--s3e-asset", default="assets/biohazard.s3e")
    parser.add_argument("--elf-asset", default="lib/armeabi/libjirobiodg.so")
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    try:
        report = build_report(args.apk, args.s3e_asset, args.elf_asset)
    except (OSError, ProviderAuditError, s3e_binary_audit.S3EAuditError,
            guest_arm_elf_audit.ELFAuditError,
            guest_arm_dynamic_import_inventory.ELFAuditError) as exc:
        parser.error(str(exc))
    rendered = json.dumps(report, indent=2)
    print(rendered)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(rendered + "\n", encoding="utf-8")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
