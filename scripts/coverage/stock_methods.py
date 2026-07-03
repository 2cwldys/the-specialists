#!/usr/bin/env python3
"""stock_methods.py — recover the set of (class, method) DEFINED in the upstream halflife-updated SDK
(branch `upstream/64bit-demo`, the fork base we reconstruct ONTO) and cache it to stock_methods.tsv.

Why: audit.py flags a method we have that the ORIGINAL 2007-SDK-based TS lacked as "invented". But many
of those are stock methods of the *modern* halflife-updated base (EquipWeapon, InitStatusBar, DoRetireWeapon,
FAllowFlashlight, …) that we inherited verbatim — not TS drift. A method DEFINED on the exact class in
upstream is provably stock-inherited. We match EXACT (class, method) only: a bare-name match would wrongly
whitelist TS virtual overrides (PlayerKilled/IsTeamplay/Classify) whose names also live on other stock
classes — those must stay visible for review. So this is the conservative, defensible stock signal.

Run when the upstream base moves (rare). audit.py reads the tsv if present and degrades gracefully if not.
"""
import os, re, subprocess, sys

ROOT = subprocess.run("git rev-parse --show-toplevel", shell=True, capture_output=True, text=True).stdout.strip()
OUT = os.path.join(ROOT, "reference", "coverage", "stock_methods.tsv")
BRANCH = "upstream/64bit-demo"
DEFRE = re.compile(r"\b([A-Za-z_]\w*)::(~?[A-Za-z_]\w*)\s*\(")


CLASSRE = re.compile(r"\b(?:class|struct)\s+(?:DLLEXPORT\s+)?([A-Za-z_]\w*)[^;{]*\{")
INLINE_DEF = re.compile(r"\b(~?[A-Za-z_]\w*)\s*\([^;{()]*\)\s*(?:const\s*)?(?:override\s*)?\{")
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "catch"}


def header_inline_pairs(txt):
    """(class, method) for methods DEFINED (with a body) inside a class declaration in a header —
    e.g. `virtual CBaseToggle* MyTogglePointer() { return NULL; }`, `void EXPORT CallDoRetireWeapon() {…}`.
    The .cpp-only Class::Method scan misses these (they were the audit\'s inline blind spot)."""
    pairs = set()
    for cm in CLASSRE.finditer(txt):
        cls, depth, i = cm.group(1), 1, cm.end()
        body_start = i
        while i < len(txt) and depth:
            if txt[i] == "{": depth += 1
            elif txt[i] == "}": depth -= 1
            i += 1
        body = txt[body_start:i]
        for m in INLINE_DEF.finditer(body):
            name = m.group(1)
            if name not in KEYWORDS:
                pairs.add((cls, name))
    return pairs


def main():
    files = subprocess.run(f"git ls-tree -r {BRANCH} --name-only -- dlls cl_dll game_shared",
                           shell=True, capture_output=True, text=True, cwd=ROOT).stdout.split()
    files = [f for f in files if f.endswith((".cpp", ".h"))]
    if not files:
        print(f"stock_methods: branch {BRANCH} not found / no files — skipping (audit degrades gracefully)", file=sys.stderr)
        return 1
    pairs = set()
    for f in files:
        txt = subprocess.run(f"git show {BRANCH}:{f}", shell=True, capture_output=True, text=True, cwd=ROOT).stdout
        if f.endswith(".cpp"):
            for m in DEFRE.finditer(txt):
                pairs.add((m.group(1), m.group(2)))
        else:
            pairs |= header_inline_pairs(txt)
    with open(OUT, "w") as fh:
        fh.write(f"# (class, method) DEFINED in {BRANCH} — exact-match stock-inherited methods. Gen: stock_methods.py\n")
        for cls, meth in sorted(pairs):
            fh.write(f"{cls}\t{meth}\n")
    print(f"wrote {OUT}: {len(pairs)} stock (class,method) pairs from {BRANCH}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
