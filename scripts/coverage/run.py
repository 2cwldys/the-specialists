#!/usr/bin/env python3
"""
run.py — regenerate the whole coverage/inventory toolchain in dependency order.

Order matters: extract_vtables → hierarchy; baseline → (layout, reconstruction, globals, inventory);
signatures + members + filemap → inventory (the capstone). One command rebuilds every report under
reference/coverage/ from the binaries + our build.

  python3 scripts/coverage/run.py
"""
import os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
# (script, why-it-runs-here) in dependency order
STEPS = [
    "extract_vtables.py",   # → VTABLES.md          (hierarchy needs it)
    "hierarchy.py",         # → hierarchy.tsv       (inventory needs it)
    "baseline.py",          # → ts_specific.txt     (layout/reconstruction/globals/inventory need it)
    "signatures.py",        # → signatures.tsv      (inventory needs it)
    "members.py",           # → members.tsv         (globals/inventory need it)
    "globals.py",           # → globals.tsv
    "precache.py",          # → precache.tsv        (per-entity asset lists; export folds in)
    "commands.py",          # → commands.tsv        (command → handler/dispatcher)
    "stock_methods.py",     # → stock_methods.tsv   (upstream base methods; audit's stock-SDK whitelist)
    "filemap.py",           # → FILE_MAP.md
    "layout.py",            # → LAYOUT_MAP.md
    "reconstruction.py",    # → RECONSTRUCTION_COVERAGE.md
    "divergence.py",        # → SDK_DIVERGENCE.md
    "inventory.py",         # → INVENTORY.md        (capstone — composes all of the above)
    "coverage.py",          # → REPORT.md + original.json/ours.json (external contract: cmds/cvars/msgs)
    "export.py",            # → ts_inventory.json   (omni: the unified machine-readable mapping)
    "audit.py",             # → AUDIT.md            (ours-vs-original: missing/invented classes/methods/files/globals)
]


def main():
    fails = 0
    for s in STEPS:
        path = os.path.join(HERE, s)
        if not os.path.exists(path):
            print(f"  SKIP {s} (not found)")
            continue
        r = subprocess.run([sys.executable, path], capture_output=True, text=True)
        ok = r.returncode == 0
        fails += not ok
        tail = (r.stdout.strip().splitlines() or [""])[-1] if ok else (r.stderr.strip().splitlines() or [""])[-1]
        print(f"  {'ok ' if ok else 'ERR'} {s:24} {tail}")
    print(f"\n{'all good' if not fails else str(fails)+' step(s) failed'} — reports in reference/coverage/")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
