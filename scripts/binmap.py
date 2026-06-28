#!/usr/bin/env python3
"""
binmap.py — map ts_i386.so functions to mp.dll addresses (SAME source, gcc2-Linux vs MSVC-Windows), so a
.so function NAME resolves to a live mp.dll RVA for `live.py break` — INCLUDING the non-exported majority
that mp.dll's export table (358 of ~2400 fns) can't reach. The live original TS runs the Windows mp.dll, so
to breakpoint e.g. CTSGun::ShotgunFire (not exported) you need its mp.dll address; this finds it.

Two signals, combined:
  1. EXPORT-NAME match — mp.dll's demangled MSVC exports vs the .so's gnu-v2 demangled symbols → exact pairs
     (high confidence; e.g. weapon_tsgun, CTSWorldGun::WaitThink).
  2. STRING-ANCHOR match — a function that references "weapons/m3/m3-pump.wav" in the .so references the SAME
     string in mp.dll. Bucket each binary's string-immediate refs by enclosing function and match on shared
     strings (Jaccard). This reaches the un-exported functions, which is the whole point.

  python3 scripts/binmap.py ShotgunFire                 # resolve a .so func name -> mp.dll RVA(s), best first
  python3 scripts/binmap.py CTSGun::ShotgunFire         # class-qualified also works
  python3 scripts/binmap.py 0x72108                     # by .so nm address
  python3 scripts/binmap.py --build                     # cache the full map -> reference/coverage/binmap.tsv

The result feeds live.py:  scripts/live.py break mp.dll 0x<rva> --go "do the thing"
"""
import os, re, subprocess, sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
# Defaults map ts_i386.so (server gcc2) -> mp.dll (server MSVC). Override via env to reuse the SAME machinery
# for the CLIENT pair: our build-i386/cl_dll/client.so -> reference/client.dll (see scripts/coverage/client_members.py).
SO = os.environ.get("BINMAP_SO", os.path.join(ROOT, "reference", "ts_i386.so"))
MP = os.environ.get("BINMAP_MP", os.path.join(ROOT, "reference", "mp.dll"))
MAP = os.environ.get("BINMAP_MAP", os.path.join(ROOT, "reference", "signatures-gcc2.map"))
DEMANGLE = os.path.join(HERE, "gnv2demangle.py")
OUT = os.environ.get("BINMAP_OUT", os.path.join(ROOT, "reference", "coverage", "binmap.tsv"))
GOT = int(os.environ.get("BINMAP_GOT", "0x165E10"), 16)   # _GLOBAL_OFFSET_TABLE_ of the SO side
IMAGEBASE = 0x10000000  # PE imagebase (mp.dll / client.dll)


def sh(*a):
    return subprocess.run(a, capture_output=True, text=True, errors="replace").stdout


# ---------------- ts_i386.so side ----------------
def so_rodata():
    for ln in sh("readelf", "-SW", SO).splitlines():
        m = re.search(r"\.rodata\s+\w+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)", ln)
        if m:
            a = int(m.group(1), 16)
            return a, a + int(m.group(2), 16)
    return 0xF6600, 0x101000


def so_funcs():
    d = {}
    for ln in sh("nm", SO).splitlines():
        p = ln.split()
        if len(p) == 3 and p[1] in "TtWw":
            try:
                d[int(p[0], 16)] = p[2]
            except ValueError:
                pass
    return d


def so_cstr(data, va):
    try:
        e = data.index(b"\0", va)
    except ValueError:
        return ""
    raw = data[va:e]
    # printable-only: a rodata ref into a FLOAT constant (e.g. 0.15 = 0x3e19999a) reads as a garbage
    # "string" that would be a false anchor. Require real ASCII.
    if 4 <= len(raw) <= 200 and all(32 <= b < 127 or b in (9, 10) for b in raw):
        return raw.decode("latin1", "replace")
    return ""


def so_func_strings():
    """{so_symbol: set(strings)} from @GOTOFF rodata refs, bucketed per function."""
    data = open(SO, "rb").read()
    ro0, ro1 = so_rodata()
    dis = sh("objdump", "-d", "--no-show-raw-insn", SO)
    out = defaultdict(set)
    cur = None
    for ln in dis.splitlines():
        h = re.match(r"^[0-9a-f]+ <([^>]+)>:", ln)
        if h:
            cur = h.group(1)
            continue
        if cur is None:
            continue
        for m in re.finditer(r"(-?0x[0-9a-f]+)\(%ebx\)", ln):
            off = int(m.group(1), 16)
            if off > 0x7FFFFFFF:
                off -= 0x100000000
            va = GOT + off
            if ro0 <= va < ro1:
                s = so_cstr(data, va)
                if len(s) >= 4:
                    out[cur].add(s)
    return out


# ---------------- mp.dll side ----------------
def mp_sections():
    secs = {}
    for ln in sh("objdump", "-h", MP).splitlines():
        m = re.match(r"\s*\d+\s+(\.\w+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)", ln)
        if m:
            secs[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16), int(m.group(4), 16))  # size,vma,fileoff
    return secs


def mp_cstr(data, va, secs):
    """resolve a VMA in .rdata/.data to a null-terminated string (file offset = vma - section_base + fileoff)."""
    for name in (".rdata", ".data"):
        if name not in secs:
            continue
        size, vma, fo = secs[name]
        if vma <= va < vma + size:
            off = va - vma + fo
            try:
                e = data.index(b"\0", off)
            except ValueError:
                return ""
            raw = data[off:e]
            if 4 <= len(raw) <= 200 and all(32 <= b < 127 or b in (9, 10) for b in raw):
                return raw.decode("latin1", "replace")
            return ""
    return ""


def mp_func_starts(secs, exp):
    """sorted mp.dll function-start RVAs. MSVC prologues are irregular (frame-pointer omission, thunks), so
    detecting starts by prologue bytes fails. Instead: every CALL destination is a function entry — collect
    all `call 0x10xxxxxx` targets in .text (+ the export entry points). Robust and compiler-agnostic."""
    size, vma, fo = secs[".text"]
    lo, hi = vma, vma + size
    starts = set(IMAGEBASE + r for r in exp.values())  # exports are starts (stored as RVA)
    dis = sh("objdump", "-d", "--no-show-raw-insn", MP)
    for m in re.finditer(r"\bcall\s+0x([0-9a-f]+)", dis):
        va = int(m.group(1), 16)
        if lo <= va < hi:
            starts.add(va)
    return sorted(r - IMAGEBASE for r in starts)


def mp_func_strings(secs, starts):
    """{mp_func_rva: set(strings)} — string immediates bucketed into the enclosing function (prologue list)."""
    import bisect
    data = open(MP, "rb").read()
    rd = secs.get(".rdata"); da = secs.get(".data")
    lo = rd[1] if rd else 0
    hi = (da[1] + da[0]) if da else (rd[1] + rd[0] if rd else 0)
    dis = sh("objdump", "-d", "--no-show-raw-insn", MP)
    out = defaultdict(set)
    cur_rva = None
    for ln in dis.splitlines():
        h = re.match(r"^\s*([0-9a-f]+):", ln)
        if h:
            addr_rva = int(h.group(1), 16) - IMAGEBASE
            k = bisect.bisect_right(starts, addr_rva) - 1
            cur_rva = starts[k] if k >= 0 else None
        for m in re.finditer(r"\$0x([0-9a-f]{6,8})\b", ln):
            va = int(m.group(1), 16)
            if lo <= va < hi and cur_rva is not None:
                s = mp_cstr(data, va, secs)
                if len(s) >= 4:
                    out[cur_rva].add(s)
    return out


def mp_exports():
    """{demangled_name: rva} for mp.dll's exports (rz-bin -E gives vaddr; rva = vaddr - imagebase)."""
    d = {}
    for ln in sh("rz-bin", "-E", MP).splitlines():
        m = re.search(r"0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+\w+\s+FUNC\s+\d+\s+\S+\s+(.+)$", ln)
        if m:
            vaddr = int(m.group(2), 16)
            name = m.group(3).strip()
            # strip MSVC signature decoration to a bare Class::Method / name
            base = re.sub(r"\(.*$", "", name).split()[-1] if name else name
            d[base] = vaddr - IMAGEBASE
            d[name] = vaddr - IMAGEBASE
    return d


# ---------------- demangle helpers ----------------
def demap():
    d = {}
    if os.path.exists(MAP):
        for ln in open(MAP, errors="replace"):
            if "=>" in ln:
                a, b = ln.split("=>", 1)
                d[a.strip()] = b.strip()
    return d


def bare(demangled):
    s = demangled.split("(", 1)[0]
    return s.replace("::", "::").strip()


# ---------------- match ----------------
def build():
    print("scanning ts_i386.so …", file=sys.stderr)
    sof = so_funcs()
    sostr = so_func_strings()
    print("scanning mp.dll …", file=sys.stderr)
    secs = mp_sections()
    exp = mp_exports()
    starts = mp_func_starts(secs, exp)
    mpstr = mp_func_strings(secs, starts)
    dm = demap()
    print(f"  .so: {len(sof)} fns, {len(sostr)} w/ strings · mp.dll: {len(starts)} prologues, "
          f"{len(mpstr)} w/ strings, {len(exp)//2} exports", file=sys.stderr)

    # .so functions that share an IDENTICAL string-set can't be told apart by strings alone (e.g. a class's
    # methods all referencing the same few sound/anim strings) — flag those as ambiguous, never a false single.
    setowners = defaultdict(list)
    for sym, S in sostr.items():
        if S:
            setowners[frozenset(S)].append(sym)

    rows = []   # (so_symbol, demangled, mp_rva, method, confidence)
    for addr, sym in sorted(sof.items()):
        dem = dm.get(sym, sym)
        b = bare(dem)
        # 1) export-name match (exact) — always trustworthy
        rva = exp.get(b) if b in exp else (exp.get(b.split("::")[-1]) if "::" in b else None)
        if rva is not None:
            rows.append((sym, dem, rva, "export", "1.00"))
            continue
        # 2) string-anchor match
        S = sostr.get(sym)
        if not S:
            continue
        cands = sorted(((len(S & MS) / len(S | MS), mrva) for mrva, MS in mpstr.items() if S & MS),
                       key=lambda t: (-t[0], t[1]))
        if not cands or cands[0][0] < 0.34:
            continue
        topscore, toprva = cands[0]
        shared = len(setowners[frozenset(S)]) > 1                                   # siblings share these strings
        tie = len(cands) > 1 and (topscore - cands[1][0]) < 1e-9                     # >1 mp fn ties for best
        method = "string?" if (shared or tie) else "string"                         # '?' = ambiguous, verify
        rows.append((sym, dem, toprva, method, f"{topscore:.2f}"))
    return rows


def write_map(rows):
    with open(OUT, "w") as w:
        w.write("so_symbol\tdemangled\tmp_rva\tmethod\tconfidence\n")
        for sym, dem, rva, meth, conf in sorted(rows, key=lambda r: r[1]):
            w.write(f"{sym}\t{dem}\t0x{rva:x}\t{meth}\t{conf}\n")
    print(f"wrote {OUT}  ({len(rows)} matched of the .so's functions)")


def resolve_one(query):
    rows = build()
    q = query.lower()
    if re.fullmatch(r"0x[0-9a-f]+", q):
        target = int(q, 16)
        sof = so_funcs()
        sym = sof.get(target)
        hits = [r for r in rows if r[0] == sym] if sym else []
    else:
        hits = [r for r in rows if q in r[1].lower() or q in r[0].lower()]
    if not hits:
        print(f"no match for '{query}'. (Exported? try the bare name. Else it may reference no unique string —\n"
              f"  fall back to the §4b prologue-scan, or pick a caller that does.)")
        return
    print(f"# binmap: {query}")
    hits = sorted(hits, key=lambda r: (r[3] == "string?", -float(r[4])))   # confident first, ambiguous last
    for sym, dem, rva, meth, conf in hits[:10]:
        flag = "  ⚠ AMBIGUOUS (shares strings w/ siblings — verify)" if meth == "string?" else ""
        print(f"  mp.dll 0x{rva:<7x} [{meth} {conf}]  {dem}{flag}")
    top = hits[0]
    if top[3] == "string?":
        print(f"\n⚠ best guess is AMBIGUOUS — its strings aren't unique to it. Confirm the RVA with a live §4b")
        print(f"  trace before trusting it (or breakpoint a uniquely-string'd CALLER and step in).")
    print(f"\n→ scripts/live.py break mp.dll 0x{top[2]:x} --go \"...\"")


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    if args[0] == "--build":
        write_map(build())
    else:
        resolve_one(args[0])


if __name__ == "__main__":
    main()
