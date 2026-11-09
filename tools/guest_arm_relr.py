#!/usr/bin/env python3
"""Decode ELF32 RELR relocation bitmaps without applying relocations."""
from __future__ import annotations

import struct
from guest_arm_elf_audit import ELFAuditError

DT_RELRSZ = 35
DT_RELR = 36
DT_RELRENT = 37
DT_ANDROID_RELR = 0x6fffe000
DT_ANDROID_RELRSZ = 0x6fffe001
DT_ANDROID_RELRENT = 0x6fffe003
DT_ANDROID_RELRCOUNT = 0x6fffe005
MAX_RELR_SIZE = 64 * 1024 * 1024
MAX_RELR_RELOCATIONS = 4_000_000
WORD_SIZE = 4
WORD_MASK = 0xffffffff


def decode_relr_records(blob: bytes, endian: str = "<", word_size: int = WORD_SIZE) -> list[dict]:
    """Expand RELR entries while preserving the encoded table-entry provenance."""
    if endian not in ("<", ">"):
        raise ELFAuditError("RELR endianness must be little or big")
    if word_size != WORD_SIZE:
        raise ELFAuditError("RELR decoder supports ELF32 four-byte words only")
    if len(blob) > MAX_RELR_SIZE:
        raise ELFAuditError("RELR table exceeds 64 MiB safety limit")
    if len(blob) % word_size:
        raise ELFAuditError("RELR table size is not a multiple of DT_RELRENT")

    targets: list[dict] = []
    cursor: int | None = None
    previous = -1
    for entry_index in range(0, len(blob), word_size):
        entry = struct.unpack_from(endian + "I", blob, entry_index)[0]
        table_index = entry_index // word_size
        if (entry & 1) == 0:
            if entry & (word_size - 1):
                raise ELFAuditError("RELR direct address is not word-aligned")
            if cursor is not None and entry < cursor:
                raise ELFAuditError("RELR direct address moves backwards or overlaps prior bitmap")
            if entry <= previous:
                raise ELFAuditError("RELR targets are not strictly increasing")
            if len(targets) >= MAX_RELR_RELOCATIONS:
                raise ELFAuditError("RELR relocation count exceeds safety limit")
            targets.append({"index": len(targets), "table_entry_index": table_index,
                            "encoding": "direct", "target_address": f"0x{entry:08x}",
                            "target_offset": entry})
            previous = entry
            cursor = entry + word_size
            if cursor > WORD_MASK + 1:
                raise ELFAuditError("RELR cursor overflows ELF32 address space")
            continue

        if cursor is None:
            raise ELFAuditError("RELR bitmap appears before an initial direct address")
        bitmap = entry >> 1
        for bit in range(31):
            if bitmap & (1 << bit):
                target = cursor + bit * word_size
                if target > WORD_MASK:
                    raise ELFAuditError("RELR bitmap target overflows ELF32 address space")
                if target <= previous:
                    raise ELFAuditError("RELR targets are not strictly increasing")
                if len(targets) >= MAX_RELR_RELOCATIONS:
                    raise ELFAuditError("RELR relocation count exceeds safety limit")
                targets.append({"index": len(targets), "table_entry_index": table_index,
                                "encoding": "bitmap", "bitmap_bit": bit,
                                "target_address": f"0x{target:08x}", "target_offset": target})
                previous = target
        cursor += 31 * word_size
        if cursor > WORD_MASK + 1:
            raise ELFAuditError("RELR bitmap cursor overflows ELF32 address space")
    return targets


def decode_relr(blob: bytes, endian: str = "<", word_size: int = WORD_SIZE) -> list[int]:
    """Compatibility wrapper returning only expanded target virtual addresses."""
    return [item["target_offset"] for item in decode_relr_records(blob, endian, word_size)]


def collect_relr_tables(data: bytes, tags: dict[int, int], loads: list[dict], endian: str, mapper):
    """Validate standard/Android RELR aliases, map tables, and decode them."""
    alias_groups = (
        ("address", DT_RELR, DT_ANDROID_RELR),
        ("size", DT_RELRSZ, DT_ANDROID_RELRSZ),
        ("entry_size", DT_RELRENT, DT_ANDROID_RELRENT),
    )
    selected = {}
    source_tags = []
    for name, standard, android in alias_groups:
        present = [(tag, tags[tag]) for tag in (standard, android) if tag in tags]
        if not present:
            raise_if_any = any(tag in tags for _, a, b in alias_groups for tag in (a, b))
            if raise_if_any:
                raise ELFAuditError(f"RELR {name} tag is missing")
            return []
        values = {value for _, value in present}
        if len(values) != 1:
            raise ELFAuditError(f"standard and Android RELR {name} tags conflict")
        selected[name] = present[0][1]
        source_tags.extend(tag for tag, _ in present)

    address = selected["address"]
    size = selected["size"]
    entry_size = selected["entry_size"]
    if entry_size != WORD_SIZE:
        raise ELFAuditError("DT_RELRENT must be 4 for ELF32")
    if size > MAX_RELR_SIZE or size % WORD_SIZE:
        raise ELFAuditError("invalid or excessive RELR table size")
    if size == 0:
        offset, targets = 0, []
    else:
        offset, raw = mapper(data, address, size, loads, "DT_RELR")
        targets = decode_relr(raw, endian)
    result = {
        "source_tag": "DT_RELR" if DT_RELR in tags else "DT_ANDROID_RELR",
        "source_tags": [
            name for name, tag in (
                ("DT_RELR", DT_RELR), ("DT_RELRSZ", DT_RELRSZ),
                ("DT_RELRENT", DT_RELRENT), ("DT_ANDROID_RELR", DT_ANDROID_RELR),
                ("DT_ANDROID_RELRSZ", DT_ANDROID_RELRSZ), ("DT_ANDROID_RELRENT", DT_ANDROID_RELRENT),
            ) if tag in tags
        ],
        "virtual_address": f"0x{address:08x}",
        "file_offset": offset,
        "size": size,
        "entry_size": entry_size,
        "entry_count": size // WORD_SIZE,
        "relocation_count": len(targets),
        "targets": [f"0x{target:08x}" for target in targets],
        "android_relr_count_hint": tags.get(DT_ANDROID_RELRCOUNT),
    }
    return [result]


def find_cross_format_overlaps(groups: list[tuple[str, list[str]]]) -> list[dict]:
    """Report target-address collisions between distinct relocation encodings."""
    owners: dict[str, set[str]] = {}
    for source, targets in groups:
        for target in targets:
            owners.setdefault(target, set()).add(source)
    return [
        {"target_address": target, "formats": sorted(formats)}
        for target, formats in sorted(owners.items())
        if len(formats) > 1
    ]

