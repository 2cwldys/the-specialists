#!/usr/bin/env python3
"""
extract_vtables.py — recover the ORIGINAL's complete vtable layouts (slot byte-offset ->
method) for all 226 classes, straight from the unstripped .so.

The vtables (`__vt_<class>` symbols) live in .data but their slots are ZERO in the file —
they're filled by ELF relocations at load. i386 uses R_386_32 against named symbols, so
`readelf -r` gives us, for each vtable+offset, the exact method symbol. That's authoritative
(no guessing): it tells us which virtual sits at which vtable offset — e.g. a decompiled
`call [vptr+0x128]` resolves to the real method. Also a faithfulness cross-check: it pins the
original's virtual-override set per class.

Pure readelf/nm — no Ghidra. Output:
  reference/coverage/vtables_original.json   { class: { "0x128": "CTSGun::Deploy()", ... } }
  reference/coverage/VTABLES.md              human-readable per-class tables

Usage:  python3 scripts/coverage/extract_vtables.py [ClassNameFilter]
"""
import json, os, re, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SO = os.path.join(ROOT, "reference", "ts_i386.so")
OUT = os.path.join(ROOT, "reference", "coverage")
DEMANGLE = os.path.join(ROOT, "scripts", "gnv2demangle.py")
os.makedirs(OUT, exist_ok=True)

def sh(*a): return subprocess.run(a, capture_output=True, text=True, errors="replace").stdout

def vtable_symbols():
    """{vaddr: classname} for every __vt_<mangled> symbol, + sorted vaddr list for extents."""
    vts = {}
    for ln in sh("nm", SO).splitlines():
        p = ln.split()
        if len(p) >= 3 and p[2].startswith("__vt_"):
            try: vts[int(p[0], 16)] = p[2]
            except ValueError: pass
    return vts

def reloc_map():
    """{location_vaddr: symbol_name} from .rel.dyn R_386_32 entries (the vtable slot targets).
    -W/--wide stops readelf truncating long symbol names with '[...]'."""
    rel = {}
    for ln in sh("readelf", "-W", "-r", SO).splitlines():
        m = re.match(r"\s*([0-9a-f]{8})\s+[0-9a-f]+\s+R_386_32\s+[0-9a-f]+\s+(\S+)", ln)
        if m:
            rel[int(m.group(1), 16)] = m.group(2)
    return rel

def batch_demangle(mangled_names):
    """Demangle a whole set of gnu-v2 symbols in ONE pass (gnv2demangle reads stdin,
    emits 'name  =>  demangled'). Returns {mangled: demangled}, falling back to raw."""
    names = sorted(n for n in mangled_names if n)
    dm = {n: n for n in names}
    try:
        p = subprocess.run([sys.executable, DEMANGLE], input="\n".join(names),
                           capture_output=True, text=True)
        for ln in p.stdout.splitlines():
            if "=>" in ln:
                raw, dem = ln.split("=>", 1)
                raw, dem = raw.strip(), dem.strip()
                if raw in dm and dem:
                    dm[raw] = dem
    except Exception:
        pass
    return dm

def main():
    filt = sys.argv[1] if len(sys.argv) > 1 else ""
    vts = vtable_symbols()
    rel = reloc_map()
    addrs = sorted(vts)

    # Demangle every method symbol referenced by a vtable slot, in one pass.
    needed = set()
    for i, base in enumerate(addrs):
        end = addrs[i + 1] if i + 1 < len(addrs) else base + 0x800
        off = 0
        while base + off < end:
            if base + off in rel: needed.add(rel[base + off])
            off += 4
    dm = batch_demangle(needed)

    def vt_classname(sym):  # __vt_6CTSGun -> CTSGun ; __vt_11CBasePlayer -> CBasePlayer
        m = re.match(r"__vt_(\d+)(.*)", sym)
        if m:
            return m.group(2)[:int(m.group(1))]
        return sym[5:] if sym.startswith("__vt_") else sym

    out = {}
    for i, base in enumerate(addrs):
        sym = vts[base]
        cls = vt_classname(sym)
        if filt and filt not in sym and filt not in cls:
            continue
        end = addrs[i + 1] if i + 1 < len(addrs) else base + 0x800
        slots = {}
        off = 0
        while base + off < end:
            tgt = rel.get(base + off)
            if tgt:
                slots[f"0x{off:x}"] = dm.get(tgt, tgt)
            off += 4
        if slots:
            out[cls] = slots
    json.dump(out, open(os.path.join(OUT, "vtables_original.json"), "w"), indent=1, sort_keys=True)

    # human-readable
    L = ["# Original TS vtable layouts (slot byte-offset -> method)", "",
         "Recovered from ts_i386.so `__vt_` symbols + R_386_32 relocations (see extract_vtables.py).",
         "A decompiled `call [vptr+0xNN]` = the method at offset 0xNN here.", ""]
    for cls in sorted(out):
        L.append(f"## {cls}")
        for off in sorted(out[cls], key=lambda x: int(x, 16)):
            L.append(f"- `{off}` → {out[cls][off]}")
        L.append("")
    open(os.path.join(OUT, "VTABLES.md"), "w").write("\n".join(L))
    print(f"wrote {len(out)} class vtables -> reference/coverage/vtables_original.json + VTABLES.md")

if __name__ == "__main__":
    main()
