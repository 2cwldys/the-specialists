#!/usr/bin/env python3
"""
vtcall.py — resolve gcc2 virtual-dispatch offsets to method names using the recovered vtable
layouts (reference/coverage/vtables_original.json from extract_vtables.py).

In the original's decompiles a virtual call looks like
    (**(code **)(*(int *)(this + 0x60) + 0x128))(...)        // this->vptr (@+0x60) slot 0x128
and you otherwise have to hand-resolve 0x128. This does it from the vtable map.

Two modes:
  # single lookup — "what method is at this class's vtable offset?"
  python3 scripts/coverage/vtcall.py CTSGun 0x128
      -> CTSGun::Deploy()

  # annotate a decompiled .c file IN PLACE-ish (writes <file>.annotated): append a
  #   /* vt -> Class::Method */ comment after each `+ 0xNN))(` virtual call, resolved against
  #   the given class's vtable (best-effort: assumes self/`this` dispatch, the common case).
  python3 scripts/coverage/vtcall.py --file recovered/server/CTSGun.c CTSGun
"""
import json, os, re, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
VT = os.path.join(ROOT, "reference", "coverage", "vtables_original.json")

def load():
    if not os.path.exists(VT):
        sys.exit("missing vtables_original.json — run scripts/coverage/extract_vtables.py first")
    return json.load(open(VT))

def lookup(vts, cls, off):
    if cls not in vts:
        cand = [c for c in vts if cls.lower() in c.lower()]
        return None, cand
    key = off if off.startswith("0x") else f"0x{int(off):x}"
    key = f"0x{int(key, 16):x}"
    return vts[cls].get(key), None

def main():
    a = sys.argv[1:]
    vts = load()
    if a and a[0] == "--file":
        path, cls = a[1], a[2]
        if cls not in vts: sys.exit(f"unknown class {cls}")
        slots = vts[cls]
        out = []
        # match a virtual call: a hex offset immediately before `))(` (the gcc2 dispatch shape)
        pat = re.compile(r"\+\s*(0x[0-9a-fA-F]+)\)\)\s*\(")
        for ln in open(path, errors="replace"):
            line = ln.rstrip("\n")
            offs = pat.findall(line)
            hits = [f"{o}->{slots[f'0x{int(o,16):x}']}" for o in offs if f"0x{int(o,16):x}" in slots]
            if hits and "vt ->" not in line:
                line += "    /* vt: " + ", ".join(hits) + " */"
            out.append(line)
        dest = path + ".annotated"
        open(dest, "w").write("\n".join(out) + "\n")
        print(f"wrote {dest} ({sum(1 for l in out if 'vt:' in l)} virtual calls annotated)")
        return
    if len(a) == 2:
        cls, off = a
        method, cand = lookup(vts, cls, off)
        if method: print(method)
        elif cand: print(f"unknown class '{cls}'. did you mean: {', '.join(cand)}")
        else: print(f"no method at {cls} vtable {off}")
        return
    sys.exit(__doc__)

if __name__ == "__main__":
    main()
