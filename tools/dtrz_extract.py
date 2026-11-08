#!/usr/bin/env python3
"""Inspect, verify, and extract observed Marmalade DTRZ/Derbh archives.

The parser is deliberately bounded. It validates the 9-byte DTRZ header,
NUL-terminated filename/folder tables, per-file attributes, the 4-byte
location header, and 16-byte location records. Payload windows are derived
from absolute offsets and the next record's offset (or EOF for the last file).

For the Stage 91 Resident Evil: Degeneration archive profile, method 0x200 is
LZMA-alone and both recorded size fields equal the decompressed byte count.
No proprietary game data is included in this repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import lzma
import posixpath
import struct
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

MAGIC = b"DTRZ"
HEADER_SIZE = 9
ATTRIBUTE_SIZE = 6
LOCATION_HEADER_SIZE = 4
LOCATION_RECORD_SIZE = 16
METHOD_STORED = 0x100
METHOD_LZMA_ALONE = 0x200
METHOD_GZIP = 0x008


class DTRZError(ValueError):
    """Raised when a DTRZ archive is malformed or unsupported."""


@dataclass(frozen=True)
class DTRZEntry:
    index: int
    path: str
    folder_index: int
    file_index: int
    flags: int
    offset: int
    size_a: int
    size_b: int
    method: int
    window_end: int

    @property
    def packed_size(self) -> int:
        return self.window_end - self.offset


@dataclass(frozen=True)
class DTRZArchive:
    file_count: int
    folder_count: int
    root_placeholder: int
    attribute_offset: int
    location_header_offset: int
    location_header_unknown: int
    location_count: int
    location_table_offset: int
    payload_offset: int
    archive_size: int
    sha256: str
    entries: tuple[DTRZEntry, ...]


def _cstring_table(data: bytes, cursor: int, count: int, label: str) -> tuple[list[str], int]:
    values: list[str] = []
    for index in range(count):
        end = data.find(b"\0", cursor)
        if end < 0:
            raise DTRZError(f"{label}[{index}] has no NUL terminator")
        raw = data[cursor:end]
        try:
            value = raw.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise DTRZError(f"{label}[{index}] is not valid UTF-8/ASCII") from exc
        values.append(value)
        cursor = end + 1
    return values, cursor


def _safe_component(value: str, label: str) -> str:
    if not value or value in {".", ".."}:
        raise DTRZError(f"{label} contains an empty or traversal component")
    if value.startswith(("/", "\\")) or ":" in value:
        raise DTRZError(f"{label} contains an absolute/drive path")
    if "/" in value or "\\" in value:
        raise DTRZError(f"{label} must be a single path component")
    return value


def _safe_folder(value: str) -> str:
    if not value:
        raise DTRZError("folder table contains an empty non-root folder")
    normalized = value.replace("\\", "/")
    if normalized.startswith("/") or ":" in normalized:
        raise DTRZError(f"folder path is absolute or drive-qualified: {value!r}")
    parts = normalized.split("/")
    if any(part in {"", ".", ".."} for part in parts):
        raise DTRZError(f"folder path contains unsafe components: {value!r}")
    return "/".join(parts)


def parse_archive(data: bytes) -> DTRZArchive:
    if len(data) < HEADER_SIZE:
        raise DTRZError("archive is shorter than the 9-byte header")
    if data[:4] != MAGIC:
        raise DTRZError("missing DTRZ signature")

    file_count, folder_count = struct.unpack_from("<HH", data, 4)
    root_placeholder = data[8]
    if file_count == 0:
        raise DTRZError("file_count must be non-zero")
    if folder_count == 0:
        raise DTRZError("folder_count must include the root folder")
    if root_placeholder != 0:
        raise DTRZError(f"unsupported root placeholder value 0x{root_placeholder:02x}")

    cursor = HEADER_SIZE
    filenames, cursor = _cstring_table(data, cursor, file_count, "filename")
    folders, cursor = _cstring_table(data, cursor, folder_count - 1, "folder")

    for name in filenames:
        _safe_component(name, "filename")
    safe_folders = [_safe_folder(folder) for folder in folders]

    attribute_offset = cursor
    attribute_end = attribute_offset + file_count * ATTRIBUTE_SIZE
    if attribute_end > len(data):
        raise DTRZError("truncated attribute table")

    attributes: list[tuple[int, int, int]] = []
    for index in range(file_count):
        folder_index, file_index, flags = struct.unpack_from(
            "<HHH", data, attribute_offset + index * ATTRIBUTE_SIZE
        )
        if folder_index >= folder_count:
            raise DTRZError(
                f"attribute {index} folder index {folder_index} exceeds folder_count {folder_count}"
            )
        if file_index != index:
            raise DTRZError(
                f"attribute {index} points to file index {file_index}; expected {index}"
            )
        attributes.append((folder_index, file_index, flags))

    location_header_offset = attribute_end
    if location_header_offset + LOCATION_HEADER_SIZE > len(data):
        raise DTRZError("truncated location header")
    location_header_unknown, location_count = struct.unpack_from(
        "<HH", data, location_header_offset
    )
    if location_count != file_count:
        raise DTRZError(
            f"location count {location_count} does not match file_count {file_count}"
        )

    location_table_offset = location_header_offset + LOCATION_HEADER_SIZE
    payload_offset = location_table_offset + location_count * LOCATION_RECORD_SIZE
    if payload_offset > len(data):
        raise DTRZError("truncated location table")

    raw_locations = [
        struct.unpack_from("<IIII", data, location_table_offset + i * LOCATION_RECORD_SIZE)
        for i in range(location_count)
    ]
    offsets = [record[0] for record in raw_locations]
    if not offsets or offsets[0] != payload_offset:
        raise DTRZError(
            f"first payload offset 0x{offsets[0] if offsets else 0:x} does not match payload boundary 0x{payload_offset:x}"
        )
    if any(a >= b for a, b in zip(offsets, offsets[1:])):
        raise DTRZError("payload offsets must be strictly increasing")
    if offsets[-1] >= len(data):
        raise DTRZError("last payload offset is outside the archive")

    entries: list[DTRZEntry] = []
    for index, ((folder_index, file_index, flags), location) in enumerate(
        zip(attributes, raw_locations)
    ):
        offset, size_a, size_b, method = location
        window_end = offsets[index + 1] if index + 1 < len(offsets) else len(data)
        if window_end <= offset:
            raise DTRZError(f"entry {index} has an empty or negative payload window")
        folder = "" if folder_index == 0 else safe_folders[folder_index - 1]
        path = f"{folder}/{filenames[index]}" if folder else filenames[index]
        normalized = posixpath.normpath(path)
        if normalized.startswith("../") or normalized in {"..", "."} or normalized.startswith("/"):
            raise DTRZError(f"entry {index} resolves outside extraction root")
        entries.append(
            DTRZEntry(
                index=index,
                path=normalized,
                folder_index=folder_index,
                file_index=file_index,
                flags=flags,
                offset=offset,
                size_a=size_a,
                size_b=size_b,
                method=method,
                window_end=window_end,
            )
        )

    return DTRZArchive(
        file_count=file_count,
        folder_count=folder_count,
        root_placeholder=root_placeholder,
        attribute_offset=attribute_offset,
        location_header_offset=location_header_offset,
        location_header_unknown=location_header_unknown,
        location_count=location_count,
        location_table_offset=location_table_offset,
        payload_offset=payload_offset,
        archive_size=len(data),
        sha256=hashlib.sha256(data).hexdigest(),
        entries=tuple(entries),
    )


def decode_entry(data: bytes, entry: DTRZEntry) -> bytes:
    blob = data[entry.offset : entry.window_end]
    if not blob:
        raise DTRZError(f"entry {entry.index} has no payload bytes")

    if entry.method == METHOD_LZMA_ALONE:
        try:
            decoded = lzma.decompress(blob, format=lzma.FORMAT_ALONE)
        except lzma.LZMAError as exc:
            raise DTRZError(f"entry {entry.index} LZMA-alone decode failed: {exc}") from exc
    elif entry.method == METHOD_STORED:
        decoded = blob
    elif entry.method == METHOD_GZIP:
        raise DTRZError(
            f"entry {entry.index} uses method 0x8; gzip variant handling is not implemented yet"
        )
    else:
        raise DTRZError(f"entry {entry.index} uses unsupported method 0x{entry.method:x}")

    # The examined Resident Evil: Degeneration profile stores the expected
    # decoded size twice. Keep both fields visible and require agreement here.
    if entry.size_a != entry.size_b:
        raise DTRZError(
            f"entry {entry.index} has differing size fields (0x{entry.size_a:x}, 0x{entry.size_b:x}); profile semantics unresolved"
        )
    if len(decoded) != entry.size_a:
        raise DTRZError(
            f"entry {entry.index} decoded to {len(decoded)} bytes; metadata expects {entry.size_a}"
        )
    return decoded


def verify_archive(data: bytes, archive: DTRZArchive) -> dict[str, object]:
    decoded_total = 0
    compressed_total = 0
    method_counts: dict[str, int] = {}
    entry_results = []
    for entry in archive.entries:
        decoded = decode_entry(data, entry)
        decoded_total += len(decoded)
        compressed_total += entry.packed_size
        method_key = f"0x{entry.method:08x}"
        method_counts[method_key] = method_counts.get(method_key, 0) + 1
        entry_results.append(
            {
                "index": entry.index,
                "path": entry.path,
                "offset": entry.offset,
                "packed_window_size": entry.packed_size,
                "decoded_size": len(decoded),
                "sha256": hashlib.sha256(decoded).hexdigest(),
                "method": method_key,
            }
        )
    return {
        "verified": True,
        "file_count": archive.file_count,
        "folder_count_including_root": archive.folder_count,
        "payload_offset": archive.payload_offset,
        "archive_size": archive.archive_size,
        "archive_sha256": archive.sha256,
        "compressed_payload_bytes": compressed_total,
        "decoded_total_bytes": decoded_total,
        "method_counts": method_counts,
        "entries": entry_results,
    }


def extract_archive(data: bytes, archive: DTRZArchive, outdir: Path) -> int:
    root = outdir.resolve()
    root.mkdir(parents=True, exist_ok=True)
    count = 0
    for entry in archive.entries:
        target = (root / Path(*entry.path.split("/"))).resolve()
        if target != root and root not in target.parents:
            raise DTRZError(f"entry {entry.index} escapes extraction root")
        target.parent.mkdir(parents=True, exist_ok=True)
        decoded = decode_entry(data, entry)
        target.write_bytes(decoded)
        count += 1
    return count


def _archive_json(archive: DTRZArchive) -> dict[str, object]:
    return {
        "format": "Marmalade DTRZ/Derbh observed profile",
        "file_count": archive.file_count,
        "folder_count_including_root": archive.folder_count,
        "attribute_offset": archive.attribute_offset,
        "location_header_offset": archive.location_header_offset,
        "location_header_unknown_u16": archive.location_header_unknown,
        "location_count": archive.location_count,
        "location_table_offset": archive.location_table_offset,
        "payload_offset": archive.payload_offset,
        "archive_size": archive.archive_size,
        "sha256": archive.sha256,
        "entries": [asdict(entry) | {"packed_size": entry.packed_size} for entry in archive.entries],
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="user-supplied DTRZ/Derbh archive")
    parser.add_argument("--list", action="store_true", help="print parsed entry metadata")
    parser.add_argument("--verify", action="store_true", help="decompress and verify every entry")
    parser.add_argument("--extract", type=Path, help="extract entries into this directory")
    parser.add_argument("--json", type=Path, help="write machine-readable report")
    args = parser.parse_args(argv)

    try:
        data = args.archive.read_bytes()
        archive = parse_archive(data)
        result = _archive_json(archive)
        if args.verify or args.extract:
            verification = verify_archive(data, archive)
            result["verification"] = verification
        if args.extract:
            result["extracted_files"] = extract_archive(data, archive, args.extract)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        if args.list or not (args.verify or args.extract or args.json):
            print(json.dumps(result, indent=2))
        if args.verify or args.extract:
            print(
                f"Verified {archive.file_count} entries; "
                f"decoded {result['verification']['decoded_total_bytes']:,} bytes."
            )
    except (OSError, DTRZError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
