#!/usr/bin/env python3
"""Conservative inspector for the observed Resident Evil: Degeneration DTRZ profile.

Legacy conservative boundary inspector. Stage 91 supersedes its metadata
inspection with tools/dtrz_extract.py, which parses attributes, location
records, payload windows and the observed LZMA-alone entries.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path
from typing import Any


MAGIC = b"DTRZ/"
HEADER_SIZE = 9
PROFILE_ROOT_NAMES = 47
PROFILE_PATHS = 34


class DTRZError(ValueError):
    pass


def _read_cstrings(data: bytes, offset: int, count: int, label: str):
    values = []
    cursor = offset
    for index in range(count):
        end = data.find(b"\0", cursor)
        if end < 0:
            raise DTRZError(
                f"{label}[{index}] is unterminated (started at 0x{cursor:x})"
            )
        raw = data[cursor:end]
        try:
            value = raw.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise DTRZError(f"{label}[{index}] is not UTF-8/ASCII") from exc
        values.append({"index": index, "value": value, "offset": cursor})
        cursor = end + 1
    return values, cursor


def inspect_bytes(
    data: bytes,
    root_count: int = PROFILE_ROOT_NAMES,
    path_count: int = PROFILE_PATHS,
    preview_bytes: int = 96,
) -> dict[str, Any]:
    if len(data) < HEADER_SIZE:
        raise DTRZError("file is shorter than the 9-byte observed header")
    if data[:5] != MAGIC:
        raise DTRZError("missing observed DTRZ/ signature")
    if root_count < 0 or path_count < 0:
        raise DTRZError("table counts must be non-negative")
    if preview_bytes < 0:
        raise DTRZError("preview_bytes must be non-negative")

    roots, after_roots = _read_cstrings(data, HEADER_SIZE, root_count, "root_names")
    paths, metadata_offset = _read_cstrings(data, after_roots, path_count, "paths")
    if not roots:
        raise DTRZError("root filename table is empty")
    if any(not item["value"] for item in roots):
        raise DTRZError("empty root filename record is not valid in this profile")
    if paths and paths[-1]["value"] == "":
        sentinel = True
    else:
        sentinel = False

    metadata = data[metadata_offset:]
    words = [
        struct.unpack_from("<H", metadata, i)[0]
        for i in range(0, min(len(metadata) - 1, 256), 2)
    ]
    histogram: dict[str, int] = {}
    for value in words:
        key = f"0x{value:04x}"
        histogram[key] = histogram.get(key, 0) + 1

    return {
        "format": "DTRZ observed profile",
        "signature": data[:5].decode("ascii"),
        "file_size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "header_hex": data[:HEADER_SIZE].hex(" "),
        "profile": {
            "root_name_count": root_count,
            "path_record_count": path_count,
            "counts_are_profile_inputs_not_universal_format_constants": True,
        },
        "root_names": roots,
        "paths": paths,
        "path_table_has_empty_final_sentinel": sentinel,
        "metadata": {
            "offset": metadata_offset,
            "size": len(metadata),
            "preview_hex": metadata[:preview_bytes].hex(" "),
            "first_u16_le_histogram_256_bytes": histogram,
            "interpretation": "opaque; record schema and payload mapping unresolved",
            "payload_offsets_decoded": False,
            "payload_lengths_decoded": False,
            "compression_codecs_identified": False,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="path to a user-supplied main.dz")
    parser.add_argument("--root-count", type=int, default=PROFILE_ROOT_NAMES)
    parser.add_argument("--path-count", type=int, default=PROFILE_PATHS)
    parser.add_argument("--preview-bytes", type=int, default=96)
    parser.add_argument("--output", type=Path, help="optional JSON report path")
    args = parser.parse_args()

    try:
        result = inspect_bytes(
            args.archive.read_bytes(),
            root_count=args.root_count,
            path_count=args.path_count,
            preview_bytes=max(0, args.preview_bytes),
        )
    except (OSError, DTRZError) as exc:
        parser.error(str(exc))

    rendered = json.dumps(result, indent=2, ensure_ascii=False)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    print(rendered)
    print(
        "\nNOTE: metadata is intentionally left opaque; this report is not a "
        "payload extraction result."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
