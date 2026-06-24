#!/usr/bin/env python3
"""
layout.py — source-LAYOUT + NAME alignment of our reconstruction against the original.

Joins four views per TS class:
  - ORIGINAL FILE  : which .cpp the class lives in originally  (filemap.py: STT_FILE + gcc2 markers)
  - ORIGINAL METHODS+SIGS : the class's real method set/prototypes  (reference/signatures-gcc2.txt)
  - OUR METHODS    : what we've implemented, by name              (nm hl.so + client.so | c++filt)
  - OUR FILE(S)    : which of our .cpp define the class's methods  (source scan of dlls/ + cl_dll/)

Produces reference/coverage/LAYOUT_MAP.md:
  1. Original file -> classes -> our file(s) -> impl status   (the "where should it live" map)
  2. NAME-DRIFT report: methods we named differently from the original (these silently corrupt the
     name-matched coverage in reconstruction.py — fix them or mark as deliberate additions)

NOTE on signatures: the original is gcc-2.x ABI, we build on the newer Itanium SDK, so return/this
types legitimately differ (bool vs int KeyValue/Save, etc.) — per the mission we do NOT chase the
internal ABI. So signatures are shown for REFERENCE (implement against the real prototype), not diffed.

  python3 scripts/coverage/layout.py            -> reference/coverage/LAYOUT_MAP.md
  python3 scripts/coverage/layout.py <Class>    -> print one class's full alignment
"""
import os, re, sys, subprocess
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SO   = os.path.join(ROOT, "reference", "ts_i386.so")
SIGS = os.path.join(ROOT, "reference", "signatures-gcc2.txt")
HLSO = os.path.join(ROOT, "build-amd64", "dlls", "hl.so")
CLSO = os.path.join(ROOT, "build-amd64", "cl_dll", "client.so")
SRC_DIRS = [os.path.join(ROOT, "dlls"), os.path.join(ROOT, "cl_dll")]
OUT  = os.path.join(ROOT, "reference", "coverage", "LAYOUT_MAP.md")

# TS-specific classes (same definition as reconstruction.py).
TS_RE = re.compile(r"^(CTS|CVip|CM61|CInfoBreakable|CBulletManager|CBaseTS|CBreakable)")


def classname(mangled):
    m = re.match(r"^[A-Za-z0-9_]+__(\d+)([A-Za-z0-9_]+)", mangled)
    if m:
        return m.group(2)[: int(m.group(1))]
    if mangled.startswith("__"):
        m = re.match(r"^__\w+__?(\d+)([A-Za-z0-9_]+)", mangled)
        if m:
            return m.group(2)[: int(m.group(1))]
    return None


def split_method(sym):
    """(class, method) from a gnu-v2 mangled FUNC symbol, or None. Skips ctors/dtors/templates."""
    if "__" not in sym:
        return None
    head, _, _rest = sym.partition("__")
    c = classname(sym)
    if not c or not head or not re.match(r"^[A-Za-z]\w*$", head):
        return None          # ctor (empty head), dtor (_$_/_._), operator, template
    return c, head


def orig_methods_from_binary():
    """{class: set(method)} for TS classes, from the COMPLETE ts_i386.so symbol table (not the
    ~74%-complete signatures file). This is the authoritative original method inventory."""
    # raw nm — ts_i386.so symbols are gnu-v2 mangled; this host's c++filt can't demangle them, and we
    # parse the mangled form directly (split_method/classname).
    nm = subprocess.run(f"nm '{SO}'", shell=True, capture_output=True, text=True).stdout
    cls = defaultdict(set)
    for ln in nm.splitlines():
        p = ln.split()
        sym = p[-1]
        if len(p) < 2 or p[-2] not in ("T", "t", "W"):
            continue
        sm = split_method(sym)
        if sm and TS_RE.match(sm[0]):
            cls[sm[0]].add(sm[1])
    return cls


# ---- original: class -> file (filemap logic) -------------------------------------------------
def orig_class_file():
    out = subprocess.check_output(["readelf", "-sW", SO], text=True)
    rows = []
    for line in out.splitlines():
        p = line.split()
        if len(p) < 8 or not p[0].endswith(":"):
            continue
        try:
            val = int(p[1], 16)
        except ValueError:
            continue
        rows.append((p[3], val, p[7]))
    starts, cur = [], None
    for typ, val, name in rows:
        if typ == "FILE":
            cur = name
        elif name == "gcc2_compiled." and cur is not None:
            starts.append((val, cur)); cur = None
    starts = sorted(set(starts))
    ranges = [(a, (starts[i + 1][0] if i + 1 < len(starts) else 0xFFFFFFFF), f)
              for i, (a, f) in enumerate(starts)]

    def file_of(addr):
        for a, e, f in ranges:
            if a <= addr < e:
                return f
        return "?"

    cf = defaultdict(set)
    seen = set()
    for typ, val, name in rows:
        if typ == "FUNC" and val and name not in seen and name != "gcc2_compiled.":
            seen.add(name)
            c = classname(name)
            if c:
                cf[c].add(file_of(val))
    return {c: sorted(fs) for c, fs in cf.items()}


# ---- methods by name from a demangled symbol stream ------------------------------------------
def methods_from(lines):
    # No TS_RE filter: the TS work-list now includes stock classes TS extended (CBasePlayer, …), so we
    # must capture ALL our classes to look them up. The report is bounded by the work-list regardless.
    cls = defaultdict(set)
    for ln in lines:
        m = re.search(r"\b([A-Z]\w+)::~?(\w+)\s*\(", ln)
        if m:
            cls[m.group(1)].add(m.group(2))
    return cls


def our_methods():
    cls = defaultdict(set)
    for so in (HLSO, CLSO):
        if os.path.exists(so):
            nm = subprocess.run(f"nm '{so}' | c++filt", shell=True, capture_output=True, text=True).stdout
            for c, ms in methods_from(nm.splitlines()).items():
                cls[c] |= ms
    return cls


# ---- our source files per class (scan for out-of-line method defs) ---------------------------
def our_class_files():
    cf = defaultdict(set)
    defre = re.compile(r"\b(C[A-Za-z0-9_]+)::~?\w+\s*\(")
    for d in SRC_DIRS:
        for fn in sorted(os.listdir(d)):
            if not fn.endswith(".cpp"):
                continue
            path = os.path.join(d, fn)
            txt = open(path, errors="replace").read()
            for m in defre.finditer(txt):
                cf[m.group(1)].add(os.path.relpath(path, ROOT))  # all C-classes (incl. stock TS extended)
    return {c: sorted(fs) for c, fs in cf.items()}


def main():
    ocf = orig_class_file()
    osig = defaultdict(dict)   # class -> {method: full original signature line} (typed, for display)
    if os.path.exists(SIGS):   # optional: only enriches the per-class typed-signature view
        for ln in open(SIGS, errors="replace"):
            m = re.search(r"\b([A-Z]\w+)::~?(\w+)\s*\(", ln)
            if m and TS_RE.match(m.group(1)):
                osig[m.group(1)].setdefault(m.group(2), ln.strip())
    # Prefer baseline.py's data-driven TS work-list (stock SDK subtracted, incl. TS methods added to
    # stock classes); fall back to the TS_RE heuristic on the full binary if it hasn't been generated.
    worklist = os.path.join(ROOT, "reference", "coverage", "ts_specific.txt")
    if os.path.exists(worklist):
        om = defaultdict(set)
        for ln in open(worklist, errors="replace"):
            ln = ln.strip()
            if "::" in ln:
                c, m = ln.split("::", 1)
                om[c].add(m)
        om = dict(om)
    else:
        om = orig_methods_from_binary()
    um = our_methods()
    ucf = our_class_files()

    def extras(c, o, u):
        """our methods not in the original, minus ctor (==class name) noise."""
        return sorted(m for m in (u - o) if m != c)

    if len(sys.argv) > 1:
        c = sys.argv[1]
        print(f"# {c}")
        print(f"  original file : {', '.join(ocf.get(c, ['?']))}")
        print(f"  our file(s)   : {', '.join(ucf.get(c, ['(none)']))}")
        o, u = om.get(c, set()), um.get(c, set())
        print(f"  methods       : {len(o&u)}/{len(o)} impl;  missing: {', '.join(sorted(o-u)) or '-'};  extra: {', '.join(m for m in sorted(u-o) if m!=c) or '-'}")
        for meth in sorted(o):
            sig = osig.get(c, {}).get(meth, f"{c}::{meth}(…)  [no typed sig — binary symbol only]")
            print(f"    [{'x' if meth in u else ' '}] {sig}")
        return

    # classes to report = the TS work-list only (um is for lookup; don't pull in all ~200 stock classes)
    classes = sorted(om)

    # ---- Section 1: original file -> classes -> our file(s) ----
    by_ofile = defaultdict(list)
    for c in classes:
        ofile = ocf.get(c, ["(not in binary)"])[0]
        by_ofile[ofile].append(c)

    L = ["# Source-layout + name alignment (our reconstruction vs original)", "",
         "Generated by `scripts/coverage/layout.py`. Shows, per ORIGINAL source file, the classes it "
         "holds and WHERE we implement them — i.e. where each class *should* live by the original's "
         "grouping. Our layout is finer-grained (per-class files); a class spanning several of our "
         "files, or several classes the original co-located, is informational, not an error.", "",
         "Legend: ✅ implemented · ⬜ not yet · ⚠️ name-drift (our method names differ — see §2).", "",
         "## 1. Original file → classes → our file(s)", "",
         "| original file | class | methods (impl/total) | our file(s) |", "|---|---|--:|---|"]
    for ofile in sorted(by_ofile):
        if not ofile.startswith("TS_") and ofile not in ("func_break.cpp",):
            # only report files that hold TS classes
            if not any(TS_RE.match(c) for c in by_ofile[ofile]):
                continue
        for c in sorted(by_ofile[ofile]):
            o, u = om.get(c, set()), um.get(c, set())
            impl = len(o & u)
            drift = bool(extras(c, o, u) and (o - u))  # we have non-ctor extras AND miss originals
            mark = "✅" if o and impl == len(o) else ("⬜" if impl == 0 else "🔶")
            if drift:
                mark += "⚠️"
            files = ", ".join(f"`{x}`" for x in ucf.get(c, [])) or "—"
            L.append(f"| `{ofile}` | {mark} `{c}` | {impl}/{len(o)} | {files} |")

    # ---- Section 2: name drift ----
    L += ["", "## 2. Method NAME-drift (fix or mark as deliberate additions)", "",
          "Methods the original has that we're MISSING **by name** while we have differently-named "
          "methods — likely renames that break the name-matched coverage metric. Original inventory is "
          "the COMPLETE ts_i386.so symbol table (not signatures-gcc2.txt). Constructors are filtered.", "",
          "Two categories to read differently: (a) genuine renames to FIX — e.g. our `HillThink`/"
          "`FallThink` vs the original's consistent `WaitThink`, our `*Touch` vs `WaitTouch`/`BounceTouch`/"
          "`OnGround`/`Fall`; (b) expected SDK-design differences NOT to chase — `Save`/`Restore`/`KeyValue` "
          "are explicit in the gcc2 SDK but handled by our modern field tables, so their absence is fine.", "",
          "| class | original file | missing (orig names) | our extra names |", "|---|---|---|---|"]
    for c in classes:
        o, u = om.get(c, set()), um.get(c, set())
        miss, extra = sorted(o - u), extras(c, o, u)
        if miss and extra:  # only the genuinely-ambiguous ones
            L.append(f"| `{c}` | `{ocf.get(c, ['?'])[0]}` | {', '.join(miss)} | {', '.join(extra)} |")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, "w").write("\n".join(L) + "\n")
    print(f"wrote {OUT}: {len(classes)} TS classes across {len(by_ofile)} original files")


if __name__ == "__main__":
    main()
