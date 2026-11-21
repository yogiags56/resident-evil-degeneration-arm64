#!/usr/bin/env python3
"""Capture readelf/llvm-readobj metadata for a local ELF without executing it."""
from __future__ import annotations
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path

def capture(path: Path, requested: str = "auto", timeout: int = 30) -> dict:
    data=path.read_bytes()
    candidates = ["readelf", "llvm-readobj"] if requested=="auto" else [requested]
    executable = next(((name,shutil.which(name)) for name in candidates if shutil.which(name)),None)
    if executable is None:
        raise RuntimeError("No requested inspection tool found in PATH")
    name,binary=executable
    if name=="readelf":
        version_args=["--version"]
        inspect_args=["-h","-l","-S","-s","-r","-A",str(path)]
    else:
        version_args=["--version"]
        inspect_args=["--file-headers","--program-headers","--sections","--symbols","--relocations","--arm-attributes",str(path)]
    version=subprocess.run([binary,*version_args],capture_output=True,text=True,timeout=timeout,check=False)
    result=subprocess.run([binary,*inspect_args],capture_output=True,text=True,timeout=timeout,check=False)
    return {
      "source":path.name,"file_size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
      "tool":{"name":name,"path":binary,"version":(version.stdout or version.stderr).strip()},
      "command":inspect_args[:-1],"exit_code":result.returncode,
      "stdout":result.stdout,"stderr":result.stderr,
      "interpretation":"External inspection output only; input was not executed or modified."
    }

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input",type=Path)
    parser.add_argument("--tool",choices=("auto","readelf","llvm-readobj"),default="auto")
    parser.add_argument("--json",type=Path)
    parser.add_argument("--timeout",type=int,default=30)
    args=parser.parse_args()
    if args.timeout<1: parser.error("--timeout must be positive")
    try:
        report=capture(args.input,args.tool,args.timeout)
        rendered=json.dumps(report,indent=2)
        if args.json:
            args.json.parent.mkdir(parents=True,exist_ok=True)
            args.json.write_text(rendered+"\n",encoding="utf-8")
        print(rendered)
    except (OSError,RuntimeError,subprocess.TimeoutExpired) as exc:
        parser.error(str(exc))
    return 0
if __name__=="__main__": raise SystemExit(main())
