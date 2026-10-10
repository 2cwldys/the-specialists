#!/usr/bin/env python3
"""Compare the sound, model and sprite names in a reTS server build with the original's.

A name the original server lacks was added by reTS. If a client does not have that file, it asks the
server for it on join; when the server has no such file either, the transfer fails and the client is
dropped ("server failed to transmit file"). Puppet tests never download, so this checks the names.
Names come from the built library's strings, so names held in arrays are covered too.

Usage: scripts/check-precache.py [--so build-i386/dlls/hl.so] GAMEDIR [GAMEDIR ...]
  e.g. scripts/check-precache.py ~/.local/share/Steam/steamapps/common/Half-Life/{ts,valve}
GAMEDIRs are searched in order, as the engine searches the mod then its fallback_dir.
Prints every name only reTS has, with where its file was found; exit 1 if any of those is missing.
Names the original has are never reported: reTS mimics them even when their file is missing.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NAME = re.compile(rb'[A-Za-z0-9_/\\%.-]+\.(?:wav|mdl|spr)$')


def names(so):
    out = subprocess.run(["strings", "-a", str(so)], capture_output=True, check=True).stdout
    found = set()
    for line in out.splitlines():
        m = NAME.search(line)
        if m:
            found.add(m.group(0).decode().replace("\\", "/").lower())
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--so", default=str(ROOT / "build-i386/dlls/hl.so"))
    ap.add_argument("--orig", default=str(ROOT / "reference/ts_i386.so"))
    ap.add_argument("gamedirs", nargs="+")
    a = ap.parse_args()
    dirs = [Path(d).expanduser() for d in a.gamedirs]
    for d in dirs:
        if not d.is_dir():
            sys.exit(f"not a directory: {d}")
    ours_only = sorted(names(a.so) - names(a.orig))
    missing = 0
    for n in ours_only:
        if "%" in n:
            print(f"pattern  {n}")
            continue
        path = ("sound/" + n) if n.endswith(".wav") else n
        hit = next((d.name for d in dirs if (d / path).is_file()), None)
        if hit is None:
            missing += 1
        print(f"{hit or 'MISSING':8} {n}")
    print(f"{len(ours_only)} names only in reTS, {missing} with no file")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
