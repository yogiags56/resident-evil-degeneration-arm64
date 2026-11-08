#!/usr/bin/env python3
"""Extract only named probe assets from a user-supplied APK into the local app assets folder."""
import argparse
import zipfile
from pathlib import Path

ASSETS = ("main.dz", "app.icf", "s3e.icf")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("apk", type=Path)
    p.add_argument("--dest", type=Path, default=Path("app/src/main/assets"))
    args = p.parse_args()
    args.dest.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.apk) as z:
        names = set(z.namelist())
        missing = [f"assets/{name}" for name in ASSETS if f"assets/{name}" not in names]
        if missing:
            p.error("APK is missing: " + ", ".join(missing))
        for name in ASSETS:
            data = z.read(f"assets/{name}")
            target = args.dest / name
            target.write_bytes(data)
            print(f"extracted {name}: {len(data)} bytes -> {target}")
    print("These files are local-only and are ignored by Git.")

if __name__ == "__main__":
    main()
