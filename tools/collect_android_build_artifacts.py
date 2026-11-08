#!/usr/bin/env python3
"""Collect and attest the ARM64 bootstrap/probe build outputs from a CI tree."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import zipfile

FORBIDDEN_APK_ENTRIES = {
    "assets/main.dz",
    "assets/biohazard.s3e",
    "assets/s3e.icf",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def unique_file(root: Path, pattern: str, description: str) -> Path:
    matches = sorted(p for p in root.glob(pattern) if p.is_file())
    if len(matches) != 1:
        raise ValueError(f"expected exactly one {description}; found {len(matches)}")
    return matches[0]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    output = args.output.resolve()
    try:
        apk = unique_file(root, "app/build/outputs/apk/debug/*.apk", "debug APK")
        native = unique_file(root, "app/build/intermediates/cxx/**/obj/arm64-v8a/libre_asset_probe.so", "ARM64 native shared library")
        if native.name != "libre_asset_probe.so" or "arm64-v8a" not in native.parts:
            raise ValueError(f"unexpected native library location: {native}")
        archive = unique_file(root, ".deps/install/liblzma-android-arm64/**/*.a", "Android liblzma archive")
        with zipfile.ZipFile(apk) as package:
            entries = set(package.namelist())
            forbidden = sorted(entries & FORBIDDEN_APK_ENTRIES)
            if forbidden:
                raise ValueError(f"CI APK unexpectedly contains proprietary game assets: {forbidden}")
            packaged_native = "lib/arm64-v8a/libre_asset_probe.so"
            if packaged_native not in entries:
                raise ValueError(f"APK does not package {packaged_native}")
            packaged_digest = hashlib.sha256(package.read(packaged_native)).hexdigest()
        if packaged_digest != sha256(native):
            raise ValueError("standalone ARM64 native library does not match the copy packaged in the APK")
        output.mkdir(parents=True, exist_ok=True)
        files = []
        for source, name, role in (
            (apk, "resident-evil-degeneration-arm64-bootstrap-probe.apk", "bootstrap-probe-apk"),
            (native, "libre_asset_probe.so", "android-arm64-native-library"),
            (archive, "liblzma-android-arm64.a", "android-arm64-static-dependency"),
        ):
            destination = output / name
            shutil.copy2(source, destination)
            files.append({
                "file": name,
                "role": role,
                "size_bytes": destination.stat().st_size,
                "sha256": sha256(destination),
            })
        manifest = {
            "schema_version": 1,
            "project": "Resident Evil: Degeneration Native ARM64 Port",
            "build_kind": "bootstrap-probe-only",
            "game_ready": False,
            "source_commit": os.environ.get("GITHUB_SHA", "unknown"),
            "run_id": os.environ.get("GITHUB_RUN_ID", "local"),
            "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT", "1"),
            "abi": "arm64-v8a",
            "min_sdk": 23,
            "proprietary_game_assets_included": False,
            "files": files,
            "notes": [
                "This APK validates the Android ARM64 native probe and packaging pipeline only.",
                "It is not a playable game build and does not include the user's proprietary game assets.",
                "Guest Marmalade file imports remain quarantined pending original-runtime semantic proof.",
            ],
        }
        (output / "BUILD_ARTIFACTS.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        (output / "SHA256SUMS.txt").write_text(
            "".join(f"{item['sha256']}  {item['file']}\n" for item in files), encoding="utf-8"
        )
        print(f"Collected {len(files)} verified build outputs into {output}")
        return 0
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
