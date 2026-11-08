#!/usr/bin/env python3
"""Audit a Marmalade S3E image or an APK containing assets/biohazard.s3e.

This parser is intentionally read-only. It reports XE3U header fields and
symbol-to-external-relocation cells; it does not patch or execute the image.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import lzma
import struct
import zipfile
from pathlib import Path
from typing import Any

HEADER_SIZE = 0x48
TARGET_SYMBOLS = (
    "s3eFileOpen",
    "s3eFileRead",
    "s3eFileGetChar",
    "s3eFileClose",
)


class S3EAuditError(ValueError):
    pass


def _u16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise S3EAuditError(f"u16 out of bounds at 0x{offset:x}")
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise S3EAuditError(f"u32 out of bounds at 0x{offset:x}")
    return struct.unpack_from("<I", data, offset)[0]


def decompress_s3e_stream(data: bytes) -> bytes:
    if data.startswith(b"XE3U"):
        return data
    try:
        decoded = lzma.decompress(data, format=lzma.FORMAT_ALONE)
    except lzma.LZMAError as exc:
        raise S3EAuditError(f"not a valid XE3U or LZMA-alone S3E stream: {exc}") from exc
    if not decoded.startswith(b"XE3U"):
        raise S3EAuditError("LZMA stream did not decode to an XE3U image")
    return decoded


def parse_header(data: bytes) -> dict[str, int | str]:
    if len(data) < HEADER_SIZE or data[:4] != b"XE3U":
        raise S3EAuditError("missing XE3U header or truncated header")
    fields = struct.unpack_from("<4sIHH" + "I" * 14, data, 0)
    (
        magic, version, flags, arch, fixup_offset, fixup_size, code_offset,
        code_file_size, code_mem_size, sig_offset, sig_size, entry_offset,
        config_offset, config_size, base_addr, extra_offset, extra_size,
        ext_hdr_size,
    ) = fields
    # The base XE3U header is 0x44 bytes; extended data_offset is present here.
    data_offset = _u32(data, 0x44) if ext_hdr_size >= 8 and len(data) >= 0x48 else 0
    if fixup_offset + fixup_size > len(data):
        raise S3EAuditError("fixup section exceeds image bounds")
    if code_offset + code_file_size > len(data):
        raise S3EAuditError("code section exceeds image bounds")
    if code_file_size > code_mem_size:
        raise S3EAuditError("code_file_size exceeds code_mem_size")
    return {
        "magic": magic.decode("ascii"),
        "version_raw": version,
        "flags": flags,
        "arch": arch,
        "fixup_offset": fixup_offset,
        "fixup_size": fixup_size,
        "code_offset": code_offset,
        "code_file_size": code_file_size,
        "code_mem_size": code_mem_size,
        "sig_offset": sig_offset,
        "sig_size": sig_size,
        "entry_offset": entry_offset,
        "config_offset": config_offset,
        "config_size": config_size,
        "base_addr": base_addr,
        "extra_offset": extra_offset,
        "extra_size": extra_size,
        "ext_hdr_size": ext_hdr_size,
        "data_offset": data_offset,
    }


def parse_fixups(data: bytes, header: dict[str, int | str]) -> dict[str, Any]:
    start = int(header["fixup_offset"])
    end = start + int(header["fixup_size"])
    pos = start
    symbols: list[str] = []
    relocations: list[dict[str, int]] = []
    sections: list[dict[str, int]] = []
    while pos < end:
        if pos + 8 > end:
            raise S3EAuditError("truncated fixup subsection header")
        section_type = _u32(data, pos)
        section_size = _u32(data, pos + 4)
        if section_size < 8 or pos + section_size > end:
            raise S3EAuditError(f"invalid fixup subsection size at 0x{pos:x}")
        sections.append({"type": section_type, "size": section_size, "offset": pos})
        payload = pos + 8
        section_end = pos + section_size
        if section_type == 0:
            count = _u16(data, payload)
            cursor = payload + 2
            for _ in range(count):
                nul = data.find(b"\0", cursor, section_end)
                if nul < 0:
                    raise S3EAuditError("unterminated S3E API symbol name")
                symbols.append(data[cursor:nul].decode("ascii", "strict"))
                cursor = nul + 1
            if len(symbols) != count:
                raise S3EAuditError("symbol table count mismatch")
        elif section_type == 1:
            count = _u32(data, payload)
            if payload + 4 + count * 4 > section_end:
                raise S3EAuditError("internal relocation table exceeds subsection")
        elif section_type in (2, 3, 4):
            count = _u32(data, payload)
            if payload + 4 + count * 6 > section_end:
                raise S3EAuditError("external relocation table exceeds subsection")
            for index in range(count):
                off_hi, off_lo, symbol_index = struct.unpack_from(
                    "<HHH", data, payload + 4 + index * 6
                )
                relocations.append({
                    "type": section_type,
                    "offset": (off_hi << 16) | off_lo,
                    "symbol_index": symbol_index,
                })
        pos += section_size
    if pos != end:
        raise S3EAuditError("fixup section boundary mismatch")
    mapped = []
    for item in relocations:
        if item["symbol_index"] >= len(symbols):
            raise S3EAuditError("external relocation references missing symbol")
        mapped.append(item | {"symbol": symbols[item["symbol_index"]]})
    return {
        "sections": sections,
        "symbol_count": len(symbols),
        "symbols": symbols,
        "external_relocations": mapped,
    }


def audit_image(data: bytes) -> dict[str, Any]:
    header = parse_header(data)
    fixups = parse_fixups(data, header)
    target_map: dict[str, list[int]] = {name: [] for name in TARGET_SYMBOLS}
    target_indices: dict[str, list[int]] = {name: [] for name in TARGET_SYMBOLS}
    relocation_types: dict[str, dict[str, int]] = {
        name: {} for name in TARGET_SYMBOLS
    }
    for relocation in fixups["external_relocations"]:
        name = relocation["symbol"]
        if name in target_map:
            target_map[name].append(relocation["offset"])
            target_indices[name].append(relocation["symbol_index"])
            type_key = str(relocation["type"])
            relocation_types[name][type_key] = (
                relocation_types[name].get(type_key, 0) + 1
            )
    return {
        "image_size": len(data),
        "image_sha256": hashlib.sha256(data).hexdigest(),
        "code_blob_sha256": hashlib.sha256(
            data[int(header["code_offset"]):
                 int(header["code_offset"]) + int(header["code_file_size"])]
        ).hexdigest(),
        "header": header,
        "fixups": {
            "sections": fixups["sections"],
            "symbol_count": fixups["symbol_count"],
            "external_relocation_count": len(fixups["external_relocations"]),
        },
        "target_import_relocation_offsets": {
            name: [f"0x{offset:x}" for offset in offsets]
            for name, offsets in target_map.items()
        },
        "target_import_symbol_indices": {
            name: sorted(set(indices))
            for name, indices in target_indices.items()
        },
        "target_import_relocation_counts_by_type": relocation_types,
    }


def load_input(path: Path, asset_name: str) -> tuple[bytes, str]:
    raw = path.read_bytes()
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            try:
                raw = archive.read(asset_name)
            except KeyError as exc:
                raise S3EAuditError(f"APK is missing {asset_name}") from exc
        source = f"{path.name}!/{asset_name}"
    else:
        source = path.name
    return decompress_s3e_stream(raw), source


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="APK or compressed/uncompressed S3E image")
    parser.add_argument("--asset", default="assets/biohazard.s3e",
                        help="S3E asset path inside APK (default: assets/biohazard.s3e)")
    parser.add_argument("--json", type=Path, help="write audit report JSON")
    parser.add_argument("--code-out", type=Path,
                        help="optionally write the extracted code+initialized-data blob")
    args = parser.parse_args()
    try:
        image, source = load_input(args.input, args.asset)
        report = audit_image(image)
        report["source"] = source
        if args.code_out:
            h = report["header"]
            code = image[int(h["code_offset"]):
                         int(h["code_offset"]) + int(h["code_file_size"])]
            args.code_out.parent.mkdir(parents=True, exist_ok=True)
            args.code_out.write_bytes(code)
            report["code_blob_written_to"] = str(args.code_out)
        rendered = json.dumps(report, indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(rendered + "\n", encoding="utf-8")
        print(rendered)
    except (OSError, zipfile.BadZipFile, S3EAuditError, UnicodeDecodeError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
