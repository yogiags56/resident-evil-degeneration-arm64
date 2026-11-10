#!/usr/bin/env python3
"""Build a read-only PT_LOAD memory layout plan for ELF32 EM_ARM images.

This module describes segment/page ranges only. It does not allocate memory,
copy bytes, zero BSS, apply relocations, or calculate an ET_DYN load bias.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from guest_arm_elf_audit import ELFAuditError
from guest_arm_pt_dynamic_audit import PT_LOAD, parse_program_headers

ELF32_LIMIT = 1 << 32
PF_X, PF_W, PF_R = 1, 2, 4
FLAG_NAMES = ((PF_R, "R"), (PF_W, "W"), (PF_X, "X"))
DEFAULT_PAGE_SIZE = 4096


def _align_down(value: int, alignment: int) -> int:
    return value & ~(alignment - 1)


def _align_up(value: int, alignment: int) -> int:
    if value > ELF32_LIMIT:
        raise ELFAuditError("page-rounded segment end overflows ELF32 address space")
    if value == ELF32_LIMIT:
        return ELF32_LIMIT
    rounded = (value + alignment - 1) & ~(alignment - 1)
    if rounded > ELF32_LIMIT:
        raise ELFAuditError("page-rounded segment end overflows ELF32 address space")
    return rounded


def _permissions(flags: int) -> list[str]:
    return [name for bit, name in FLAG_NAMES if flags & bit]


def plan_load_segments(data: bytes, page_size: int = DEFAULT_PAGE_SIZE) -> dict:
    """Return validated PT_LOAD ranges and page-level overlap diagnostics."""
    if page_size < 1024 or page_size > 1024 * 1024 or page_size & (page_size - 1):
        raise ELFAuditError("page size must be a power of two between 1 KiB and 1 MiB")

    ph = parse_program_headers(data)
    if ph["type"] not in (2, 3):
        raise ELFAuditError("load planner requires ET_EXEC or ET_DYN")
    if ph["machine"] != 40:
        raise ELFAuditError("load planner requires EM_ARM")

    loads = [item for item in ph["program_headers"] if item["type"] == PT_LOAD]
    if not loads:
        raise ELFAuditError("ELF contains no PT_LOAD segments")

    planned = []
    for segment in loads:
        start = segment["vaddr"]
        filesz = segment["filesz"]
        memsz = segment["memsz"]
        end_file = start + filesz
        end_memory = start + memsz
        if end_memory > ELF32_LIMIT:
            raise ELFAuditError("PT_LOAD memory range overflows ELF32 address space")
        if filesz > memsz:
            raise ELFAuditError("PT_LOAD file size exceeds memory size")
        if segment["offset"] + filesz > len(data):
            raise ELFAuditError("PT_LOAD file range exceeds input")
        if memsz == 0:
            page_start = page_end = start
            file_page_offset = None
        else:
            if (start - segment["offset"]) % page_size:
                raise ELFAuditError("PT_LOAD virtual address and file offset are not page-congruent")
            page_start = _align_down(start, page_size)
            page_end = _align_up(end_memory, page_size)
            leading = start - page_start
            if segment["offset"] < leading:
                raise ELFAuditError("PT_LOAD cannot derive a nonnegative page-aligned file offset")
            file_page_offset = segment["offset"] - leading
        perms = _permissions(segment["flags"])
        planned.append({
            "program_header_index": segment["index"],
            "virtual_address": start,
            "file_offset": segment["offset"],
            "file_size": filesz,
            "memory_size": memsz,
            "file_backed_range": [start, end_file],
            "zero_fill_range": [end_file, end_memory] if memsz > filesz else None,
            "memory_range": [start, end_memory],
            "page_range": [page_start, page_end],
            "page_file_offset": file_page_offset,
            "alignment": segment["align"],
            "flags": segment["flags"],
            "permissions": perms,
            "writable_executable": bool(segment["flags"] & PF_W and segment["flags"] & PF_X),
        })

    # Report page-rounded collisions without allocating a page list; ELF ranges
    # may be very large and must not cause memory use proportional to memsz.
    overlaps = []
    for index, left in enumerate(planned):
        if left["memory_size"] == 0:
            continue
        for right in planned[index + 1:]:
            if right["memory_size"] == 0:
                continue
            start = max(left["page_range"][0], right["page_range"][0])
            end = min(left["page_range"][1], right["page_range"][1])
            if start < end:
                union = sorted(set(left["permissions"]) | set(right["permissions"]))
                overlaps.append({
                    "left_program_header": left["program_header_index"],
                    "right_program_header": right["program_header_index"],
                    "shared_page_range": [start, end],
                    "combined_permissions": union,
                    "writable_executable": "W" in union and "X" in union,
                    "classification": "page_overlap_requires_loader_policy",
                })

    entry = ph["entry"]
    executable_owners = [
        item["program_header_index"] for item in planned
        if "X" in item["permissions"]
        and item["memory_range"][0] <= entry < item["memory_range"][1]
    ]
    warnings = []
    for item in planned:
        if item["writable_executable"]:
            warnings.append(
                f"PT_LOAD[{item['program_header_index']}] is both writable and executable"
            )
    for item in overlaps:
        if item["writable_executable"]:
            warnings.append(
                "page overlap combines writable and executable permissions"
            )
    if not executable_owners:
        warnings.append("ELF entry point is not inside an executable PT_LOAD memory range")

    return {
        "source_size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "elf_class": 32,
        "machine": "ARM",
        "elf_type": "ET_EXEC" if ph["type"] == 2 else "ET_DYN",
        "endianness": ph["endianness"],
        "entry_virtual_address": entry,
        "page_size": page_size,
        "load_bias_policy": (
            "fixed_virtual_addresses_no_bias" if ph["type"] == 2
            else "required_at_runtime_not_calculated"
        ),
        "load_segment_count": len(planned),
        "segments": planned,
        "page_overlaps": overlaps,
        "page_overlap_count": len(overlaps),
        "entry_executable_owners": executable_owners,
        "warnings": warnings,
        "execution_performed": False,
        "memory_modified": False,
        "evidence_limits": [
            "Read-only PT_LOAD planning; no memory is allocated or modified.",
            "File-backed and zero-fill ranges are described but not copied or cleared.",
            "ET_DYN load bias is required at runtime but is not selected or calculated.",
            "Page-rounded overlap is reported for a later loader policy decision.",
            "No relocation is applied and no entry point is executed.",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--page-size", type=int, default=DEFAULT_PAGE_SIZE)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    try:
        report = plan_load_segments(args.input.read_bytes(), args.page_size)
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
