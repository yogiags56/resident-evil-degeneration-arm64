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
MAX_RELR_SIZE = 64 * 1024 * 1024
WORD_SIZE = 4
WORD_MASK = 0xffffffff


def decode_relr(blob: bytes, endian: str = "<", word_size: int = WORD_SIZE) -> list[int]:
    """Expand an ELF32 RELR table into relocation target virtual addresses."""
    if endian not in ("<", ">"):
        raise ELFAuditError("RELR endianness must be little or big")
    if word_size != WORD_SIZE:
        raise ELFAuditError("RELR decoder supports ELF32 four-byte words only")
    if len(blob) > MAX_RELR_SIZE:
        raise ELFAuditError("RELR table exceeds 64 MiB safety limit")
    if len(blob) % word_size:
        raise ELFAuditError("RELR table size is not a multiple of DT_RELRENT")
    if len(blob) // word_size > MAX_RELR_SIZE // word_size:
        raise ELFAuditError("RELR entry count exceeds safety limit")

    targets: list[int] = []
    cursor: int | None = None
    previous = -1
    for index in range(0, len(blob), word_size):
        entry = struct.unpack_from(endian + "I", blob, index)[0]
        if (entry & 1) == 0:
            if entry & (word_size - 1):
                raise ELFAuditError("RELR direct address is not word-aligned")
            if cursor is not None and entry < cursor:
                raise ELFAuditError("RELR direct address moves backwards or overlaps prior bitmap")
            if entry <= previous:
                raise ELFAuditError("RELR targets are not strictly increasing")
            targets.append(entry)
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
                targets.append(target)
                previous = target
        cursor += 31 * word_size
        if cursor > WORD_MASK + 1:
            raise ELFAuditError("RELR bitmap cursor overflows ELF32 address space")
    return targets


def collect_relr_tables(data: bytes, tags: dict[int, int], loads: list[dict], endian: str, mapper):
    """Validate standard/Android RELR tag sets, map tables, and decode them."""
    standard_tags = (DT_RELR, DT_RELRSZ, DT_RELRENT)
    android_tags = (DT_ANDROID_RELR, DT_ANDROID_RELRSZ, DT_ANDROID_RELRENT)
    present_standard = [tag in tags for tag in standard_tags]
    present_android = [tag in tags for tag in android_tags]
    if any(present_standard) and not all(present_standard):
        raise ELFAuditError("DT_RELR, DT_RELRSZ, and DT_RELRENT must appear together")
    if any(present_android) and not all(present_android):
        raise ELFAuditError("Android DT_ANDROID_RELR tag triplet must appear together")
    if not any(present_standard) and not any(present_android):
        return []

    specs = []
    if all(present_standard):
        specs.append(("DT_RELR", tags[DT_RELR], tags[DT_RELRSZ], tags[DT_RELRENT]))
    if all(present_android):
        specs.append(("DT_ANDROID_RELR", tags[DT_ANDROID_RELR], tags[DT_ANDROID_RELRSZ], tags[DT_ANDROID_RELRENT]))

    decoded = []
    for source, address, size, entry_size in specs:
        if entry_size != WORD_SIZE:
            raise ELFAuditError(f"{source}: DT_RELRENT must be 4 for ELF32")
        if size > MAX_RELR_SIZE or size % WORD_SIZE:
            raise ELFAuditError(f"{source}: invalid or excessive RELR table size")
        if size == 0:
            offset, raw, targets = 0, b"", []
        else:
            offset, raw = mapper(data, address, size, loads, source)
            targets = decode_relr(raw, endian)
        decoded.append({
            "source_tag": source,
            "virtual_address": f"0x{address:08x}",
            "file_offset": offset,
            "size": size,
            "entry_size": entry_size,
            "entry_count": size // WORD_SIZE,
            "relocation_count": len(targets),
            "targets": [f"0x{target:08x}" for target in targets],
        })

    if len(decoded) == 2:
        first, second = decoded
        if (first["virtual_address"], first["size"], first["entry_size"], first["targets"]) != (
            second["virtual_address"], second["size"], second["entry_size"], second["targets"]
        ):
            raise ELFAuditError("standard and Android RELR tag triplets describe conflicting tables")
        first["source_tags"] = ["DT_RELR", "DT_ANDROID_RELR"]
        return [first]
    decoded[0]["source_tags"] = [decoded[0]["source_tag"]]
    return decoded
