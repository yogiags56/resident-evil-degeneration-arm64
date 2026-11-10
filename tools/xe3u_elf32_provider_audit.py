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

def _load_segments(data: bytes) -> list[dict]:
    if len(data) < 52 or data[:4] != bytes((0x7f, 0x45, 0x4c, 0x46)) or data[4] != 1:
        raise ProviderAuditError("expected ELF32 input")
    endian = "<" if data[5] == 1 else ">" if data[5] == 2 else None
    if endian is None:
        raise ProviderAuditError("unsupported ELF byte order")
    fields = struct.unpack_from(endian + "HHIIIIIHHHHHH", data, 16)
    _, _, _, _, phoff, _, _, ehsize, phentsize, phnum, _, _, _ = fields
    if ehsize < 52 or phentsize < 32 or phoff + phentsize * phnum > len(data):
        raise ProviderAuditError("program-header table out of bounds")
    rows = []
    for i in range(phnum):
        off = phoff + i * phentsize
        typ, file_off, vaddr, _, filesz, memsz, flags, align = struct.unpack_from(
            endian + "IIIIIIII", data, off)
        if file_off + filesz > len(data) or filesz > memsz:
            raise ProviderAuditError(f"invalid program header {i} file/memory range")
        if typ == 1:
            rows.append({"offset": file_off, "vaddr": vaddr, "filesz": filesz,
                         "memsz": memsz, "flags": flags, "align": align})
    return rows


def _vaddr_file_offset(segments: list[dict], address: int, size: int) -> int:
    matches = []
    for seg in segments:
        if address >= seg["vaddr"] and address + size <= seg["vaddr"] + seg["filesz"]:
            matches.append(seg["offset"] + address - seg["vaddr"])
    if len(matches) != 1:
        raise ProviderAuditError(
            f"virtual range 0x{address:x}+0x{size:x} is not uniquely file-backed")
    return matches[0]


def _memory_segment(segments: list[dict], address: int, size: int, required_flags: int = 0) -> dict | None:
    matches = [seg for seg in segments
        if address >= seg["vaddr"] and address + size <= seg["vaddr"] + seg["memsz"]
        and (seg["flags"] & required_flags) == required_flags]
    return matches[0] if len(matches) == 1 else None


def _thumb_ldr_literal(halfword: int, instruction_address: int) -> tuple[int, int] | None:
    if halfword & 0xf800 != 0x4800:
        return None
    rt = (halfword >> 8) & 7
    literal = ((instruction_address + 4) & ~3) + ((halfword & 0xff) * 4)
    return rt, literal


def _thumb_ldr_word_imm(halfword: int) -> tuple[int, int, int] | None:
    if halfword & 0xf800 != 0x6800:
        return None
    return halfword & 7, (halfword >> 3) & 7, ((halfword >> 6) & 31) * 4


def _thumb_str_word_imm(halfword: int) -> tuple[int, int, int] | None:
    if halfword & 0xf800 != 0x6000:
        return None
    return halfword & 7, (halfword >> 3) & 7, ((halfword >> 6) & 31) * 4


def _thumb_bl_target(pc: int, first: int, second: int) -> int | None:
    if first & 0xf800 != 0xf000 or second & 0xd000 != 0xd000:
        return None
    s = (first >> 10) & 1
    j1 = (second >> 13) & 1
    j2 = (second >> 11) & 1
    i1 = (~(j1 ^ s)) & 1
    i2 = (~(j2 ^ s)) & 1
    immediate = ((s << 24) | (i1 << 23) | (i2 << 22) |
                 ((first & 0x03ff) << 12) | ((second & 0x07ff) << 1))
    if immediate & (1 << 24):
        immediate -= 1 << 25
    return (pc + 4 + immediate) & 0xffffffff


def audit_gl_dispatch_wrappers(elf_bytes: bytes, matched_exports: list[dict],
                               dynamic_relocation_sections: list[dict]) -> dict:
    segments = _load_segments(elf_bytes)
    endian = "<" if elf_bytes[5] == 1 else ">"
    relative_targets = {
        int(entry["offset"], 16)
        for section in dynamic_relocation_sections
        for entry in section["entries"]
        if entry["type"] == 23
    }
    wrappers = []
    for export in matched_exports:
        if not export["name"].startswith("gl") or export["elf_type"] != 2:
            continue
        if not export["thumb_state"] or not export["elf_size"]:
            continue
        start = int(export["elf_value"], 16) & ~1
        size = export["elf_size"]
        code_off = _vaddr_file_offset(segments, start, min(size, 2))
        if _memory_segment(segments, start, min(size, 2), 1) is None:
            raise ProviderAuditError(f"{export['name']} is not in an executable PT_LOAD")
        halfwords = [
            struct.unpack_from(endian + "H", elf_bytes, code_off + i * 2)[0]
            for i in range(min(size // 2, 32))
        ]
        dispatch = None
        for i, halfword in enumerate(halfwords):
            literal_load = _thumb_ldr_literal(halfword, start + i * 2)
            if literal_load is None:
                continue
            base_reg, literal_address = literal_load
            try:
                literal_off = _vaddr_file_offset(segments, literal_address, 4)
            except ProviderAuditError:
                continue
            table_base = struct.unpack_from(endian + "I", elf_bytes, literal_off)[0]
            if _memory_segment(segments, table_base, 4, 2) is None:
                continue
            for j in range(i + 1, min(len(halfwords), i + 9)):
                load = _thumb_ldr_word_imm(halfwords[j])
                if load is None:
                    continue
                target_reg, source_reg, slot_offset = load
                if source_reg != base_reg:
                    continue
                slot_address = table_base + slot_offset
                slot_seg = _memory_segment(segments, slot_address, 4, 2)
                if slot_seg is None:
                    continue
                slot_is_bss = (
                    slot_address >= slot_seg["vaddr"] + slot_seg["filesz"] and
                    slot_address + 4 <= slot_seg["vaddr"] + slot_seg["memsz"])
                dispatch = {
                    "table_base": table_base,
                    "table_base_literal_address": literal_address,
                    "table_base_literal_has_r_arm_relative": literal_address in relative_targets,
                    "table_register": base_reg,
                    "function_register": target_reg,
                    "slot_offset": slot_offset,
                    "slot_address": slot_address,
                    "slot_is_zero_fill_bss": slot_is_bss,
                }
                break
            if dispatch is not None:
                break
        if dispatch is None:
            wrappers.append({"name": export["name"], "elf_value": export["elf_value"],
                             "classification": "unclassified_thumb_function"})
            continue

        name_literals = []
        resolver_targets = []
        resolver_instruction_indices = []
        for i, halfword in enumerate(halfwords):
            literal_load = _thumb_ldr_literal(halfword, start + i * 2)
            if literal_load is not None and literal_load[0] == 0:
                try:
                    name_off = _vaddr_file_offset(segments, literal_load[1], 4)
                    name_ptr = struct.unpack_from(endian + "I", elf_bytes, name_off)[0]
                    string_off = _vaddr_file_offset(segments, name_ptr, 1)
                    string_end = elf_bytes.find(bytes((0,)), string_off,
                        min(len(elf_bytes), string_off + 128))
                    if string_end >= 0:
                        candidate = elf_bytes[string_off:string_end].decode("ascii")
                        if candidate.startswith("gl"):
                            name_literals.append({"address": f"0x{name_ptr:08x}",
                                                  "name": candidate})
                except (ProviderAuditError, UnicodeDecodeError):
                    pass
            if i + 1 < len(halfwords):
                target = _thumb_bl_target(start + i * 2, halfword, halfwords[i + 1])
                if target is not None:
                    resolver_targets.append(target)
                    resolver_instruction_indices.append(i)

        stores_resolved_pointer = any(
            _thumb_str_word_imm(halfwords[j]) == (0, dispatch["table_register"],
                                                   dispatch["slot_offset"])
            for call_index in resolver_instruction_indices
            for j in range(call_index + 2, len(halfwords)))
        wrappers.append({
            "name": export["name"], "elf_value": export["elf_value"],
            "classification": "thumb_lazy_dispatch_wrapper",
            **{key: f"0x{value:08x}" if isinstance(value, int) else value
               for key, value in dispatch.items()},
            "function_name_literals": name_literals,
            "shared_call_targets": [f"0x{x:08x}" for x in resolver_targets],
            "name_literal_matches_export": any(x["name"] == export["name"]
                                               for x in name_literals),
            "stores_resolved_pointer_to_dispatch_slot": stores_resolved_pointer,
        })

    classified = [x for x in wrappers
                  if x["classification"] == "thumb_lazy_dispatch_wrapper"]
    table_bases = sorted({int(x["table_base"], 16) for x in classified})
    slot_offsets = sorted({int(x["slot_offset"], 16) for x in classified})
    holes = []
    if len(table_bases) == 1 and slot_offsets:
        holes = [f"0x{x:02x}" for x in range(0, max(slot_offsets) + 4, 4)
                 if x not in slot_offsets]
    resolver_targets = sorted({target for row in classified
                               for target in row["shared_call_targets"]})
    return {
        "candidate_gl_export_count": len(wrappers),
        "classified_lazy_dispatch_wrapper_count": len(classified),
        "all_classified_slots_zero_fill_bss": bool(classified) and all(
            x["slot_is_zero_fill_bss"] for x in classified),
        "all_table_base_literals_have_r_arm_relative": bool(classified) and all(
            x["table_base_literal_has_r_arm_relative"] for x in classified),
        "all_function_name_literals_match": bool(classified) and all(
            x["name_literal_matches_export"] for x in classified),
        "all_wrappers_cache_resolved_pointer": bool(classified) and all(
            x["stores_resolved_pointer_to_dispatch_slot"] for x in classified),
        "table_bases": [f"0x{x:08x}" for x in table_bases],
        "slot_offsets": [f"0x{x:02x}" for x in slot_offsets],
        "unclaimed_slot_offsets": holes,
        "shared_call_targets": resolver_targets,
        "wrappers": wrappers,
        "limits": [
            "The table slots are in the ELF PT_LOAD zero-fill tail; their initial runtime values are zero after compliant materialization.",
            "A shared call target is identified from Thumb BL encoding; its full resolver ABI and host callback source are not established here.",
            "This audit does not execute the provider, apply relocations, or install host GL function pointers.",
        ],
    }


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
        "gl_dispatch_wrapper_audit": audit_gl_dispatch_wrappers(
            elf_bytes, intersection["matched"],
            dynamic_report["dynamic_relocation_sections"]),
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
