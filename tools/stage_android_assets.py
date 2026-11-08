#!/usr/bin/env python3
"""Stage only required proprietary game assets from a user-supplied APK.

The extracted files are local build inputs and must not be committed or included
in source repository archives.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

REQUIRED_ASSETS = (
    "assets/main.dz",
    "assets/biohazard.s3e",
    "assets/s3e.icf",
)


def stage(apk_path: Path, output_dir: Path, force: bool = False) -> dict:
    if not apk_path.is_file():
        raise ValueError(f"APK not found: {apk_path}")
    output_dir.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(apk_path, "r") as archive:
        names = archive.namelist()
        for name in REQUIRED_ASSETS:
            if names.count(name) != 1:
                raise ValueError(
                    f"Expected exactly one {name!r} in APK; found {names.count(name)}"
                )
        payloads = {name: archive.read(name) for name in REQUIRED_ASSETS}

    if not payloads["assets/main.dz"].startswith(b"DTRZ"):
        raise ValueError("assets/main.dz does not have the expected DTRZ magic")
    if not payloads["assets/biohazard.s3e"]:
        raise ValueError("assets/biohazard.s3e is empty")
    if not payloads["assets/s3e.icf"]:
        raise ValueError("assets/s3e.icf is empty")

    manifest = {
        "schema": 1,
        "source_apk_name": apk_path.name,
        "notice": (
            "Local build inputs extracted from a user-supplied APK. "
            "Do not commit, redistribute, or include in source archives."
        ),
        "assets": [],
    }
    for apk_name, data in payloads.items():
        filename = apk_name.split("/", 1)[1]
        target = output_dir / filename
        if target.exists() and not force:
            raise FileExistsError(
                f"{target} already exists; pass --force to replace it"
            )
        target.write_bytes(data)
        manifest["assets"].append({
            "apk_path": apk_name,
            "file": filename,
            "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        })

    manifest_path = output_dir / "local-assets-manifest.json"
    if manifest_path.exists() and not force:
        # Avoid leaving newly staged assets with a stale manifest.
        for item in manifest["assets"]:
            (output_dir / item["file"]).unlink(missing_ok=True)
        raise FileExistsError(
            f"{manifest_path} already exists; pass --force to replace it"
        )
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("apk", type=Path, help="user-supplied original APK")
    parser.add_argument(
        "--output", type=Path,
        default=Path("app/src/main/assets"),
        help="Android assets directory (default: app/src/main/assets)",
    )
    parser.add_argument(
        "--force", action="store_true",
        help="replace previously staged local assets",
    )
    args = parser.parse_args(argv)
    try:
        manifest = stage(args.apk, args.output, args.force)
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    total = sum(item["size"] for item in manifest["assets"])
    print(f"Staged {len(manifest['assets'])} local APK assets ({total} bytes).")
    print(f"Output: {args.output}")
    print("These proprietary files are local build inputs, not repository content.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
