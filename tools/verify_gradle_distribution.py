#!/usr/bin/env python3
"""Verify and safely extract the pinned Gradle binary distribution."""
from __future__ import annotations
import argparse
import hashlib
import pathlib
import sys
import zipfile

def verify_and_extract(archive: pathlib.Path, expected_sha256: str,
                       destination: pathlib.Path) -> None:
    expected = expected_sha256.lower()
    if len(expected) != 64 or any(c not in "0123456789abcdef" for c in expected):
        raise ValueError("expected SHA-256 must be 64 hexadecimal characters")
    if not archive.is_file():
        raise FileNotFoundError(f"Gradle archive not found: {archive}")
    actual = hashlib.sha256(archive.read_bytes()).hexdigest()
    if actual != expected:
        raise ValueError(f"Gradle SHA-256 mismatch: expected {expected}; got {actual}")
    with zipfile.ZipFile(archive) as z:
        members = z.infolist()
        for item in members:
            path = pathlib.PurePosixPath(item.filename)
            if path.is_absolute() or ".." in path.parts:
                raise ValueError(f"unsafe Gradle ZIP path: {item.filename}")
            mode = item.external_attr >> 16
            if mode and (mode & 0o170000) == 0o120000:
                raise ValueError(f"symbolic links are not accepted in Gradle ZIP: {item.filename}")
        if not members or not any(x.filename.endswith("/bin/gradle") for x in members):
            raise ValueError("Gradle ZIP lacks bin/gradle")
        executable = destination / "bin/gradle"
        if destination.exists():
            if executable.is_file():
                return
            raise ValueError(f"incomplete existing Gradle installation: {destination}")
        destination.parent.mkdir(parents=True, exist_ok=True)
        z.extractall(destination.parent)
    if not executable.is_file():
        raise ValueError("extracted Gradle executable not found")
    executable.chmod(executable.stat().st_mode | 0o111)

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("destination", type=pathlib.Path)
    parser.add_argument("--expected-sha256", required=True)
    args = parser.parse_args()
    try:
        verify_and_extract(args.archive, args.expected_sha256, args.destination)
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    print(f"PASS: Gradle distribution verified and extracted: {args.destination}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
