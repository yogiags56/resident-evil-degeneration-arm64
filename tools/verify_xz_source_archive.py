#!/usr/bin/env python3
"""Verify a pinned XZ Utils source archive before extraction."""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import re
import sys
import tarfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
LOCK = ROOT / "config/android-toolchain.lock.json"

def load_lock():
    return json.loads(LOCK.read_text(encoding="utf-8"))["xz"]

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("--expected-sha256", help="override only for isolated verifier tests")
    parser.add_argument("--expected-version", help="override only for isolated verifier tests")
    args = parser.parse_args()
    lock = load_lock()
    expected_hash = (args.expected_sha256 or lock["sha256"]).lower()
    expected_version = args.expected_version or lock["version"]
    if not re.fullmatch(r"[0-9a-f]{64}", expected_hash):
        print("ERROR: expected SHA-256 must be 64 lowercase/uppercase hex characters", file=sys.stderr)
        return 2
    if not args.archive.is_file():
        print(f"ERROR: archive not found: {args.archive}", file=sys.stderr)
        return 2
    digest = hashlib.sha256(args.archive.read_bytes()).hexdigest()
    if digest != expected_hash:
        print(f"ERROR: XZ source archive SHA-256 mismatch: expected {expected_hash}; actual {digest}", file=sys.stderr)
        return 1
    try:
        with tarfile.open(args.archive, "r:gz") as archive:
            members = archive.getmembers()
            for member in members:
                path = pathlib.PurePosixPath(member.name)
                if path.is_absolute() or ".." in path.parts:
                    print(f"ERROR: archive contains an unsafe path: {member.name}", file=sys.stderr)
                    return 1
                if member.issym() or member.islnk():
                    target = pathlib.PurePosixPath(member.linkname)
                    if target.is_absolute() or ".." in target.parts:
                        print(f"ERROR: archive contains an unsafe link: {member.name} -> {member.linkname}", file=sys.stderr)
                        return 1
            candidates = [
                m for m in members
                if m.name.endswith("/src/liblzma/api/lzma/version.h")
                or m.name == "src/liblzma/api/lzma/version.h"
            ]
            if len(candidates) != 1:
                print(f"ERROR: expected exactly one liblzma version header; found {len(candidates)}", file=sys.stderr)
                return 1
            stream = archive.extractfile(candidates[0])
            if stream is None:
                print("ERROR: cannot read liblzma version header", file=sys.stderr)
                return 1
            header = stream.read().decode("ascii", errors="replace")
    except (tarfile.TarError, OSError) as exc:
        print(f"ERROR: invalid gzip tar archive: {exc}", file=sys.stderr)
        return 1
    fields = {}
    for name in ("MAJOR", "MINOR", "PATCH"):
        match = re.search(rf"^#define LZMA_VERSION_{name}\s+(\d+)\s*$", header, re.M)
        if match:
            fields[name] = match.group(1)
    version = ".".join(fields.get(k, "") for k in ("MAJOR", "MINOR", "PATCH"))
    if version != expected_version:
        print(f"ERROR: expected XZ Utils {expected_version}; archive contains {version or 'unknown'}", file=sys.stderr)
        return 1
    print(f"PASS: SHA-256, safe tar paths/links, and XZ Utils {version} version header verified")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
