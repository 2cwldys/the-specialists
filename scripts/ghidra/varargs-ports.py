#!/usr/bin/env python3
"""Join ScanVarargs.java's call-site list with the client.dll / mp.dll addresses reTS cites in its comments.

Before ApplyWinTypes.java the decompiles of client.dll and mp.dll dropped every argument after the format of
a varargs call, so a port written from them may have guessed those arguments. This lists, for each binary
function that makes a variadic call with at least one variadic argument, the reTS lines citing an address
inside it, so each port can be compared against the raw instructions.

  scripts/ghidra/varargs-ports.py --client va-client.tsv --mp va-mp.tsv [--all]

A citation is a hex number in cl_dll/, dlls/, pm_shared/, common/ or game_shared/. A 0x100xxxxx value is a
virtual address, a smaller one an RVA (imagebase 0x10000000). The module comes from the line ("client" or
"mp.dll"), else from the directory: cl_dll/ means client.dll; in dlls/ an RVA with no module named is taken
to be a ts_i386.so address and skipped, and a bare VA is checked against both DLLs.
"""
import argparse
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DIRS = ["cl_dll", "dlls", "pm_shared", "common", "game_shared"]
HEX = re.compile(r"0x([0-9a-fA-F]{4,8})\b")


def load(path):
    rows = []
    if not path:
        return rows
    csv.field_size_limit(sys.maxsize)
    with open(path, newline="") as f:
        for r in csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE):
            r["lo"] = int(r["caller"], 16)
            r["hi"] = int(r["caller_end"], 16)
            rows.append(r)
    return rows


def citations():
    out = []  # (module or None, va, file, line no, text)
    for d in DIRS:
        base = os.path.join(ROOT, d)
        for dp, _, files in os.walk(base):
            for fn in files:
                if not fn.endswith((".cpp", ".h", ".c")):
                    continue
                p = os.path.join(dp, fn)
                rel = os.path.relpath(p, ROOT)
                with open(p, encoding="latin-1") as f:
                    for n, line in enumerate(f, 1):
                        if "0x" not in line:
                            continue
                        low = line.lower()
                        mod = "client" if "client" in low else "mp" if "mp.dll" in low else None
                        for m in HEX.finditer(line):
                            v = int(m.group(1), 16)
                            if 0x10000000 <= v < 0x10400000:
                                va = v
                            elif v < 0x400000:
                                if mod is None and not rel.startswith("cl_dll"):
                                    continue
                                va = v + 0x10000000
                            else:
                                continue
                            m2 = mod or ("client" if rel.startswith("cl_dll") else None)
                            out.append((m2, va, rel, n, line.strip()))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--client")
    ap.add_argument("--mp")
    ap.add_argument("--all", action="store_true", help="also list callers whose variadic calls pass no varargs")
    a = ap.parse_args()
    tables = {"client": load(a.client), "mp": load(a.mp)}
    cites = citations()
    for mod, rows in tables.items():
        callers = {}
        for r in rows:
            callers.setdefault(r["caller"], []).append(r)
        for caller, calls in sorted(callers.items()):
            if not a.all and not any(int(c["varargs"]) > 0 for c in calls):
                continue
            lo, hi = calls[0]["lo"], calls[0]["hi"]
            hits = [c for c in cites if (c[0] in (mod, None)) and lo <= c[1] <= hi]
            if not hits:
                continue
            print(f"== {mod} {caller} {calls[0]['caller_name']}")
            for c in calls:
                print(f"   call {c['call']} {c['callee']} fmt={c['format']!r} varargs={c['varargs']} computed={c['computed']}")
                print(f"      decompiled: {c['decompiled']}")
                if c["args_raw"]:
                    for arg in c["args_raw"].split(" | "):
                        print(f"      arg: {arg}")
            seen = set()
            for c in hits:
                key = (c[2], c[3])
                if key in seen:
                    continue
                seen.add(key)
                tag = "" if c[0] else " (module not named; checked against both DLLs)"
                print(f"   cited {c[2]}:{c[3]}{tag}: {c[4][:160]}")


if __name__ == "__main__":
    main()
