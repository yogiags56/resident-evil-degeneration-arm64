#!/usr/bin/env python3
"""Collect and attest the ARM64 bootstrap/probe build outputs from a CI tree."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
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


def source_commit(root: Path) -> str:
    # pull_request workflows check out a synthetic merge commit by default.
    # Its second parent is the actual PR head; attest that source revision.
    refs = ["HEAD^2", "HEAD"] if os.environ.get("GITHUB_EVENT_NAME") == "pull_request" else ["HEAD"]
    for ref in refs:
        result = subprocess.run(
            ["git", "-C", str(root), "rev-parse", "--verify", ref],
            check=False, capture_output=True, text=True,
        )
        if result.returncode == 0:
            return result.stdout.strip()
    return os.environ.get("GITHUB_SHA", "unknown")


def is_elf64_aarch64(data: bytes) -> bool:
    # ELF ident + e_machine (little-endian uint16 at byte offset 18).
    return (
        len(data) >= 20
        and data[:4] == b"\x7fELF"
        and data[4] == 2
        and data[5] == 1
        and int.from_bytes(data[18:20], "little") == 183
    )


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
            packaged_native_bytes = package.read(packaged_native)
        if not is_elf64_aarch64(packaged_native_bytes):
            raise ValueError("native library packaged in APK is not ELF64/AArch64")
        output.mkdir(parents=True, exist_ok=True)
        files = []
        outputs = [
            (apk, "resident-evil-degeneration-arm64-bootstrap-probe.apk", "bootstrap-probe-apk"),
            (archive, "liblzma-android-arm64.a", "android-arm64-static-dependency"),
        ]
        for source, name, role in outputs:
            destination = output / name
            shutil.copy2(source, destination)
            files.append({
                "file": name,
                "role": role,
                "size_bytes": destination.stat().st_size,
                "sha256": sha256(destination),
            })

        # Android Gradle Plugin may strip debug/symbol sections from the APK
        # copy, so it is not byte-identical to the unstripped CMake intermediate.
        # Export the actual packaged ARM64 binary and keep the intermediate
        # separately for native debugging.
        packaged_output = output / "libre_asset_probe.so"
        packaged_output.write_bytes(packaged_native_bytes)
        files.append({
            "file": packaged_output.name,
            "role": "android-arm64-native-library-packaged-in-apk",
            "size_bytes": packaged_output.stat().st_size,
            "sha256": sha256(packaged_output),
        })
        unstripped_output = output / "libre_asset_probe-unstripped.so"
        shutil.copy2(native, unstripped_output)
        files.append({
            "file": unstripped_output.name,
            "role": "android-arm64-native-library-unstripped-intermediate",
            "size_bytes": unstripped_output.stat().st_size,
            "sha256": sha256(unstripped_output),
        })
        manifest = {
            "schema_version": 1,
            "project": "Resident Evil: Degeneration Native ARM64 Port",
            "build_kind": "bootstrap-probe-only",
            "game_ready": False,
            "source_commit": source_commit(root),
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
