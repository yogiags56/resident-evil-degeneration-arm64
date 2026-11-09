#!/usr/bin/env python3
"""Decode Android APS2 packed dynamic relocations from ELF32 ARM PT_DYNAMIC."""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path
from guest_arm_elf_audit import ELFAuditError
from guest_arm_dynamic_relocation_segment_audit import (
    _read_dynamic, _mapped, _parse_symbols, DT_HASH, DT_SYMTAB, DT_SYMENT,
    DT_STRTAB, DT_STRSZ, RELOCS, _parse_symbol_versions,
)
from guest_arm_relr import collect_relr_tables

DT_ANDROID_REL = 0x6000000f
DT_ANDROID_RELSZ = 0x60000010
DT_ANDROID_RELA = 0x60000011
DT_ANDROID_RELASZ = 0x60000012
MAX_PACKED_SIZE = 64 * 1024 * 1024
MAX_RELOCATIONS = 4_000_000
MAX_GROUP_SIZE = 1_000_000
GROUPED_BY_INFO = 1
GROUPED_BY_OFFSET_DELTA = 2
GROUPED_BY_ADDEND = 4
HAS_ADDEND = 8
KNOWN_FLAGS = GROUPED_BY_INFO | GROUPED_BY_OFFSET_DELTA | GROUPED_BY_ADDEND | HAS_ADDEND


def _sleb128(data: bytes, pos: int, label: str = "APS2 value") -> tuple[int, int]:
    """Read one bounded signed 32-bit LEB128 value."""
    value = 0
    shift = 0
    start = pos
    byte = 0
    for _ in range(5):
        if pos >= len(data):
            raise ELFAuditError(f"{label}: truncated SLEB128")
        byte = data[pos]
        pos += 1
        if shift == 28 and not ((byte & 0x7f) <= 0x07 or (byte & 0x7f) >= 0x78):
            raise ELFAuditError(f"{label}: invalid fifth-byte SLEB128 sign extension")
        value |= (byte & 0x7f) << shift
        shift += 7
        if not (byte & 0x80):
            if byte & 0x40:
                value |= -(1 << shift)
            if value < -(1 << 31) or value > 0x7fffffff:
                raise ELFAuditError(f"{label}: SLEB128 value exceeds signed 32-bit range")
            # A fifth continuation byte would have been rejected by the loop.
            return value, pos
    raise ELFAuditError(f"{label}: overlong SLEB128 at byte {start}")


def decode_aps2(blob: bytes, kind: str, max_relocations: int = MAX_RELOCATIONS) -> list[dict]:
    """Decode APS2 REL or RELA stream; no target memory is read or modified."""
    if kind not in ("REL", "RELA"):
        raise ELFAuditError("APS2 kind must be REL or RELA")
    if len(blob) > MAX_PACKED_SIZE:
        raise ELFAuditError("APS2 stream exceeds 64 MiB safety limit")
    if len(blob) < 4 or blob[:4] != b"APS2":
        raise ELFAuditError("invalid APS2 magic")
    pos = 4
    count, pos = _sleb128(blob, pos, "relocation count")
    offset_raw, pos = _sleb128(blob, pos, "initial relocation offset")
    offset = offset_raw & 0xffffffff
    if count < 0 or count > max_relocations:
        raise ELFAuditError("APS2 relocation count is outside the supported limit")
    entries = []
    info = 0
    addend = 0
    while len(entries) < count:
        group_size, pos = _sleb128(blob, pos, "group size")
        flags, pos = _sleb128(blob, pos, "group flags")
        if group_size <= 0 or group_size > MAX_GROUP_SIZE:
            raise ELFAuditError("APS2 group size is invalid")
        if group_size > count - len(entries):
            raise ELFAuditError("APS2 group exceeds remaining relocation count")
        if flags < 0 or flags & ~KNOWN_FLAGS:
            raise ELFAuditError("APS2 group contains unknown flags")
        has_addend = bool(flags & HAS_ADDEND)
        if flags & GROUPED_BY_ADDEND and not has_addend:
            raise ELFAuditError("APS2 GROUPED_BY_ADDEND requires HAS_ADDEND")
        if kind == "REL" and has_addend:
            raise ELFAuditError("APS2 REL stream must not contain addends")
        grouped_delta = None
        grouped_info = None
        grouped_addend = None
        if flags & GROUPED_BY_OFFSET_DELTA:
            grouped_delta, pos = _sleb128(blob, pos, "grouped offset delta")
        if flags & GROUPED_BY_INFO:
            grouped_info, pos = _sleb128(blob, pos, "grouped relocation info")
            info = grouped_info & 0xffffffff
        if has_addend and flags & GROUPED_BY_ADDEND:
            grouped_addend, pos = _sleb128(blob, pos, "grouped addend delta")
        if kind == "RELA":
            if not has_addend:
                addend = 0
            elif grouped_addend is not None:
                addend += grouped_addend
                if addend < -(1 << 31) or addend > 0x7fffffff:
                    raise ELFAuditError("APS2 cumulative addend exceeds signed 32-bit range")
        for group_index in range(group_size):
            if grouped_delta is None:
                delta, pos = _sleb128(blob, pos, "relocation offset delta")
            else:
                delta = grouped_delta
            if delta < 0:
                raise ELFAuditError("APS2 relocation offset delta must not be negative")
            offset += delta
            if offset > 0xffffffff:
                raise ELFAuditError("APS2 relocation offset overflows ELF32")
            if grouped_info is None:
                raw_info, pos = _sleb128(blob, pos, "relocation info")
                # r_info is an absolute field value, not a delta. SLEB128
                # sign extension is converted back to the ELF32 bit pattern.
                info = raw_info & 0xffffffff
            if has_addend and grouped_addend is None:
                delta_addend, pos = _sleb128(blob, pos, "relocation addend delta")
                addend += delta_addend
                if addend < -(1 << 31) or addend > 0x7fffffff:
                    raise ELFAuditError("APS2 cumulative addend exceeds signed 32-bit range")
            sym_index, reloc_type = info >> 8, info & 0xff
            entries.append({
                "index": len(entries),
                "target_address": f"0x{offset:08x}",
                "symbol_index": sym_index,
                "type": reloc_type,
                "type_name": RELOCS.get(reloc_type, f"R_ARM_UNKNOWN_{reloc_type}"),
                "addend": addend if kind == "RELA" else None,
            })
    if pos != len(blob):
        raise ELFAuditError("APS2 stream contains trailing bytes after declared relocations")
    return entries


def audit(data: bytes) -> dict:
    ph = __import__("guest_arm_pt_dynamic_audit").parse_program_headers(data)
    dynamic, tags, loads, endian = _read_dynamic(data, ph)
    specs = []
    for address_tag, size_tag, kind in (
        (DT_ANDROID_REL, DT_ANDROID_RELSZ, "REL"),
        (DT_ANDROID_RELA, DT_ANDROID_RELASZ, "RELA"),
    ):
        if address_tag in tags or size_tag in tags:
            if address_tag not in tags or size_tag not in tags:
                raise ELFAuditError(f"{kind} APS2 address and size tags must appear together")
            size = tags[size_tag]
            if size == 0 or size > MAX_PACKED_SIZE:
                raise ELFAuditError(f"{kind} APS2 size is empty or exceeds safety limit")
            off, blob = _mapped(data, tags[address_tag], size, loads, f"DT_ANDROID_{kind}")
            specs.append((kind, tags[address_tag], off, blob))
    if not specs:
        raise ELFAuditError("no Android APS2 relocation tags found in PT_DYNAMIC")
    relr_tables = collect_relr_tables(data, tags, loads, endian, _mapped)
    decoded = []
    max_symbol = 0
    for kind, address, file_offset, blob in specs:
        entries = decode_aps2(blob, kind)
        for entry in entries:
            max_symbol = max(max_symbol, entry["symbol_index"])
        decoded.append({
            "kind": kind, "source_tag": f"DT_ANDROID_{kind}",
            "virtual_address": f"0x{address:08x}", "file_offset": file_offset,
            "packed_size": len(blob), "entry_count": len(entries), "entries": entries,
        })
    symbol_table, symbol_info = _parse_symbols(data, tags, loads, endian, max_symbol)
    version_info = _parse_symbol_versions(data, tags, loads, endian, symbol_table, symbol_info)
    for table in decoded:
        for entry in table["entries"]:
            idx = entry["symbol_index"]
            if symbol_table is not None:
                if idx >= len(symbol_table):
                    raise ELFAuditError(f"APS2 symbol index {idx} exceeds DT_HASH nchain")
                sym = symbol_table[idx]
                entry["symbol"] = sym["name"]
                entry["symbol_undefined"] = sym["undefined"]
                entry["symbol_size"] = sym["size"]
                entry["symbol_defined"] = sym["section_index"] != 0
                entry["symbol_section_index"] = sym["section_index"]
                entry["symbol_version"] = sym.get("version")
            else:
                entry["symbol"] = None
                entry["symbol_undefined"] = None
                entry["symbol_size"] = None
                entry["symbol_defined"] = None
                entry["symbol_section_index"] = None
                entry["symbol_version"] = None
    return {
        "source_size": len(data), "sha256": hashlib.sha256(data).hexdigest(),
        "elf_class": 32, "machine": "ARM", "endianness": ph["endianness"],
        "section_headers_present": ph["section_headers_present"],
        "dynamic_entries": dynamic,
        "dynamic_tags": {f"0x{k:08x}": v for k, v in tags.items()},
        "symbol_resolution": symbol_info,
        "symbol_versioning": version_info,
        "packed_relocation_tables": decoded,
        "relocation_count": sum(x["entry_count"] for x in decoded),
        "relr_tables": relr_tables,
        "relr_relocation_count": sum(x["relocation_count"] for x in relr_tables),
        "evidence_limits": [
            "Only ELF32 EM_ARM Android APS2 REL/RELA streams described by PT_DYNAMIC are supported.",
            "Packed relocation targets are reported but never dereferenced, patched, or applied.",
            "Symbol names, sizes, and definition state are resolved only when bounded hash and dynamic symbol/string-table metadata establish the dynamic symbol table.",
            "Symbol versions are decoded only from bounded DT_VERSYM/DT_VERDEF/DT_VERNEED metadata; provider compatibility is not inferred.",
            "RELR direct/bitmap tables are inventoried separately; target contents are never dereferenced or modified.",\n            "Runtime load-bias calculation and provider inference are not implemented.",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    try:
        report = audit(args.input.read_bytes())
        report["source"] = args.input.name
        rendered = json.dumps(report, indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(rendered + "\n", encoding="utf-8")
        print(rendered)
    except (OSError, ELFAuditError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
