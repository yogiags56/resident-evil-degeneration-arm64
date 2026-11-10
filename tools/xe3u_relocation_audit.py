#!/usr/bin/env python3
"""Read-only audit of XE3U internal and external relocation cells.

Accepts an APK containing assets/biohazard.s3e or a compressed/raw XE3U image.
It never modifies or executes the game image.
"""
from __future__ import annotations
import argparse
import collections
import hashlib
import json
import lzma
import struct
import zipfile
from pathlib import Path

class AuditError(ValueError):
    pass

def u16(data: bytes, off: int) -> int:
    if off < 0 or off + 2 > len(data):
        raise AuditError(f"u16 out of range at 0x{off:x}")
    return struct.unpack_from("<H", data, off)[0]

def u32(data: bytes, off: int) -> int:
    if off < 0 or off + 4 > len(data):
        raise AuditError(f"u32 out of range at 0x{off:x}")
    return struct.unpack_from("<I", data, off)[0]

def verify_arm_import_stub(code: bytes, cell_offset: int):
    """Verify the observed 3-instruction ARM import thunk addressing one cell."""
    cell_base, stub_base = 0x13B708, 0x84
    delta = cell_offset - cell_base
    if delta < 0 or delta % 4:
        return None
    slot = delta // 4
    stub = stub_base + slot * 12
    if stub + 12 > len(code):
        return None
    first, second, third = struct.unpack_from("<III", code, stub)
    if first != 0xE28FC601:
        return None
    if second == 0xE28CCA3B:
        second_add = 0x3B000
    elif second == 0xE28CCA3A:
        second_add = 0x3A000
    else:
        return None
    if (third & 0xFFFFF000) != 0xE5BCF000:
        return None
    ldr_offset = third & 0xFFF
    # The first ADD uses ARM's PC value (instruction address + 8).
    effective_cell = stub + 8 + 0x100000 + second_add + ldr_offset
    if effective_cell != cell_offset:
        return None
    return {"stub_offset": f"0x{stub:x}", "slot_index": slot,
            "ldr_offset": f"0x{ldr_offset:x}",
            "effective_cell": f"0x{effective_cell:x}"}

def load_image(path: Path, asset: str) -> tuple[bytes, str]:
    raw = path.read_bytes()
    source = path.name
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as zf:
            try:
                raw = zf.read(asset)
            except KeyError as exc:
                raise AuditError(f"APK missing {asset}") from exc
        source += "!/" + asset
    if not raw.startswith(b"XE3U"):
        try:
            raw = lzma.decompress(raw, format=lzma.FORMAT_ALONE,
                                  memlimit=64 * 1024 * 1024)
        except lzma.LZMAError as exc:
            raise AuditError(f"invalid LZMA-alone stream: {exc}") from exc
    if not raw.startswith(b"XE3U"):
        raise AuditError("decoded input does not start with XE3U")
    return raw, source

def audit_image(image: bytes, source: str = "<memory>") -> dict:
    if len(image) < 0x48:
        raise AuditError("truncated XE3U header")
    version = u32(image, 4)
    flags = u16(image, 8)
    arch = u16(image, 10)
    fixoff, fixsize, codeoff, codefilesz, codemem = struct.unpack_from(
        "<5I", image, 12)
    base = u32(image, 52)
    extraoff, extrasize, ext_hdr = struct.unpack_from("<3I", image, 56)
    dataoff = u32(image, 68) if ext_hdr >= 8 else 0
    if not fixsize or fixoff < 0x48 or fixoff + fixsize > len(image):
        raise AuditError("invalid fixup range")
    if not codefilesz or codefilesz > codemem or codeoff + codefilesz > len(image):
        raise AuditError("invalid code file/memory range")
    if dataoff > codefilesz:
        raise AuditError("data_offset exceeds code_file_size")
    if base + codemem > 0x100000000:
        raise AuditError("link-time image address range overflows 32-bit space")
    if fixoff + fixsize > codeoff:
        raise AuditError("fixup section overlaps code blob")
    code = image[codeoff:codeoff + codefilesz]

    symbols: list[str] = []
    internal: list[int] = []
    external: list[tuple[int, int, int]] = []
    sections = []
    pos, end = fixoff, fixoff + fixsize
    while pos < end:
        if end - pos < 8:
            raise AuditError("truncated fixup subsection header")
        typ, size = struct.unpack_from("<II", image, pos)
        if size < 8 or pos + size > end:
            raise AuditError(f"invalid fixup subsection size at 0x{pos:x}")
        payload, section_end = pos + 8, pos + size
        count = 0
        if typ == 0:
            if section_end - payload < 2:
                raise AuditError("truncated symbol table")
            count = u16(image, payload)
            cursor = payload + 2
            for _ in range(count):
                nul = image.find(b"\0", cursor, section_end)
                if nul < 0:
                    raise AuditError("unterminated API symbol")
                try:
                    name = image[cursor:nul].decode("ascii")
                except UnicodeDecodeError as exc:
                    raise AuditError("non-ASCII API symbol") from exc
                if not name:
                    raise AuditError("empty API symbol")
                symbols.append(name)
                cursor = nul + 1
        elif typ == 1:
            if section_end - payload < 4:
                raise AuditError("truncated internal relocation count")
            count = u32(image, payload)
            if count * 4 > section_end - payload - 4:
                raise AuditError("internal relocation array exceeds subsection")
            internal.extend(struct.unpack_from("<" + "I" * count,
                                               image, payload + 4))
        elif typ in (2, 3, 4):
            if section_end - payload < 4:
                raise AuditError("truncated external relocation count")
            count = u32(image, payload)
            if count * 6 > section_end - payload - 4:
                raise AuditError("external relocation array exceeds subsection")
            for i in range(count):
                hi, lo, sym = struct.unpack_from("<HHH", image,
                                                 payload + 4 + i * 6)
                if sym >= len(symbols):
                    raise AuditError("external relocation symbol index out of range")
                external.append((typ, (hi << 16) | lo, sym))
        sections.append({"type": typ, "offset": pos, "size": size,
                         "count": count})
        pos = section_end
    if pos != end:
        raise AuditError("fixup section boundary mismatch")

    def read_cell(offset: int, kind: str) -> int:
        if offset & 3:
            raise AuditError(f"{kind} patch site is not 4-byte aligned: 0x{offset:x}")
        if offset + 4 > len(code):
            raise AuditError(f"{kind} patch site outside file-backed code: 0x{offset:x}")
        return u32(code, offset)

    internal_sites = set()
    patch_buckets = collections.Counter()
    target_buckets = collections.Counter()
    pointer_bit0 = collections.Counter()
    targets = []
    for offset in internal:
        if offset in internal_sites:
            raise AuditError(f"duplicate internal patch site 0x{offset:x}")
        internal_sites.add(offset)
        word = read_cell(offset, "internal")
        patch_buckets["code-side" if offset < dataoff else "initialized-data-side"] += 1
        # ARM/Thumb function pointers encode Thumb state in bit 0. Strip it
        # only for address-range classification; preserve it in relocation.
        target = word & ~1
        if target < base or target >= base + codemem:
            target_buckets["out-of-image"] += 1
            raise AuditError(f"internal pointer outside XE3U image at 0x{offset:x}")
        rva = target - base
        if rva < dataoff:
            bucket = "code-side"
        elif rva < codefilesz:
            bucket = "initialized-data-side"
        else:
            bucket = "memory-only-tail"
        target_buckets[bucket] += 1
        pointer_bit0[word & 1] += 1
        targets.append(rva)

    external_sites = set()
    external_values = collections.Counter()
    external_zero_records = []
    external_nonzero_count = 0
    verified_arm_stubs = []
    for typ, offset, sym in external:
        if offset in external_sites:
            raise AuditError(f"duplicate external patch site 0x{offset:x}")
        external_sites.add(offset)
        word = read_cell(offset, "external")
        external_values[f"0x{word:08x}"] += 1
        if word == 0:
            external_zero_records.append({
                "offset": f"0x{offset:x}", "symbol": symbols[sym],
                "section_type": typ})
        else:
            external_nonzero_count += 1
            match = verify_arm_import_stub(code, offset) if typ == 4 else None
            if match:
                verified_arm_stubs.append({
                    "offset": f"0x{offset:x}", "symbol": symbols[sym],
                    "cell_value": f"0x{word:08x}", **match})
    return {
        "source": source,
        "image_size": len(image),
        "image_sha256": hashlib.sha256(image).hexdigest(),
        "code_blob_sha256": hashlib.sha256(code).hexdigest(),
        "header": {
            "version": f"0x{version:08x}", "flags": flags,
            "architecture": arch, "base_address": f"0x{base:08x}",
            "fixup_offset": fixoff, "fixup_size": fixsize,
            "code_offset": codeoff, "code_file_size": codefilesz,
            "code_memory_size": codemem, "data_offset": dataoff,
            "extra_offset": extraoff, "extra_size": extrasize,
        },
        "sections": sections,
        "symbols": len(symbols),
        "internal_relocations": {
            "count": len(internal), "unique_patch_sites": len(internal_sites),
            "patch_sites_by_region": dict(patch_buckets),
            "targets_by_region": dict(target_buckets),
            "target_rva_min": f"0x{min(targets):x}" if targets else None,
            "target_rva_max": f"0x{max(targets):x}" if targets else None,
            "pointer_bit0_set": pointer_bit0[1],
            "pointer_bit0_clear": pointer_bit0[0],
            "candidate_semantics": "stored 32-bit in-image pointers; likely add runtime load bias, preserving bit 0; not applied",
        },
        "external_relocations": {
            "count": len(external), "unique_patch_sites": len(external_sites),
            "cell_values": dict(external_values),
            "nonzero_cells": external_nonzero_count,
            "zero_cells": external_zero_records,
            "verified_arm_import_stubs": len(verified_arm_stubs),
            "verified_stub_details": verified_arm_stubs,
            "note": "external cells are reported separately; no internal relocation rule is applied",
        },
    }

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="original APK or S3E image")
    parser.add_argument("--asset", default="assets/biohazard.s3e")
    parser.add_argument("--json", type=Path, help="optional JSON report path")
    args = parser.parse_args()
    try:
        image, source = load_image(args.input, args.asset)
        report = audit_image(image, source)
    except (OSError, zipfile.BadZipFile, AuditError) as exc:
        parser.error(str(exc))
    rendered = json.dumps(report, indent=2)
    print(rendered)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(rendered + "\\n", encoding="utf-8")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
