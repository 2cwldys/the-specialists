#!/usr/bin/env python3
"""
inventory.py — the unified TS inventory + REFACTOR BLUEPRINT (capstone over the other coverage tools).

Composes, per ORIGINAL source file:
  - the classes that live in it            (filemap: STT_FILE + gcc2 markers)
  - each class's TS-specific functions      (baseline.py's ts_specific.txt) with ✓/✗ impl status (our nm)
  - each class's recovered MEMBERS          (members.py's members.tsv)
  - where we CURRENTLY implement it          (source scan of dlls/ + cl_dll/)

The result is the blueprint for Phase 2: it says exactly what each original file should contain (classes,
functions, members) and where that code lives in our tree today — so the refactor is "move X here, rename
Y" with a checklist, and re-running it verifies the result. Run the upstream tools first
(`baseline.py`, then this); falls back gracefully if their outputs are absent.

  python3 scripts/coverage/inventory.py            -> reference/coverage/INVENTORY.md
  python3 scripts/coverage/inventory.py <File|Class>
"""
import os, re, subprocess, sys
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SO   = os.path.join(ROOT, "reference", "ts_i386.so")
COV  = os.path.join(ROOT, "reference", "coverage")
HLSO = os.path.join(ROOT, "build-amd64", "dlls", "hl.so")
CLSO = os.path.join(ROOT, "build-amd64", "cl_dll", "client.so")
OUT  = os.path.join(COV, "INVENTORY.md")


def classname(s):
    m = re.match(r"^[A-Za-z0-9_]+__(\d+)([A-Za-z0-9_]+)", s)
    return m.group(2)[: int(m.group(1))] if m else None


def class_file():
    """class -> original .cpp (filemap logic: per-TU gcc2_compiled. address ranges)."""
    out = subprocess.check_output(["readelf", "-sW", SO], text=True)
    rows, starts, cur = [], [], None
    for ln in out.splitlines():
        p = ln.split()
        if len(p) < 8 or not p[0].endswith(":"):
            continue
        try:
            val = int(p[1], 16)
        except ValueError:
            continue
        rows.append((p[3], val, p[7]))
    for typ, val, name in rows:
        if typ == "FILE":
            cur = name
        elif name == "gcc2_compiled." and cur is not None:
            starts.append((val, cur)); cur = None
    starts = sorted(set(starts))
    ranges = [(a, (starts[i + 1][0] if i + 1 < len(starts) else 1 << 32), f) for i, (a, f) in enumerate(starts)]
    cf, seen = {}, set()
    for typ, val, name in rows:
        if typ == "FUNC" and val and name not in seen and name != "gcc2_compiled.":
            seen.add(name)
            c = classname(name)
            if c and c not in cf:
                cf[c] = next((f for a, e, f in ranges if a <= val < e), "?")
    return cf


def load_worklist():
    f = os.path.join(COV, "ts_specific.txt")
    cls = defaultdict(set)
    if os.path.exists(f):
        for ln in open(f):
            if "::" in ln:
                c, m = ln.strip().split("::", 1)
                cls[c].add(m)
    return cls


def load_sigs():
    """{(class, method): rendered typed signature} from signatures.py output."""
    f = os.path.join(COV, "signatures.tsv")
    sig = {}
    if os.path.exists(f):
        for ln in open(f).read().splitlines()[1:]:
            p = ln.split("\t")
            if len(p) == 7:
                cls, meth, ret, params, qual, virt, _src = p
                v = "virtual " if virt == "1" else ""
                q = f" {qual}" if qual else ""
                sig[(cls, meth)] = f"{v}{ret} {meth}({params}){q}"
    return sig


def load_base():
    f = os.path.join(COV, "hierarchy.tsv")
    base = {}
    if os.path.exists(f):
        for ln in open(f).read().splitlines()[1:]:
            p = ln.split("\t")
            if len(p) == 2:
                base[p[0]] = p[1]
    return base


def func_addr():
    """{(class, method): addr} from the binary symtab — to order functions as in the original file."""
    nm = subprocess.run(f"nm '{SO}'", shell=True, capture_output=True, text=True).stdout
    out = {}
    for ln in nm.splitlines():
        p = ln.split()
        if len(p) < 3 or p[-2] not in ("T", "t", "W"):
            continue
        sym = p[-1]
        if "__" not in sym:
            continue
        head = sym.partition("__")[0]
        c = classname(sym)
        if c and re.match(r"^[A-Za-z]\w*$", head):
            out.setdefault((c, head), int(p[0], 16))
    return out


def load_members():
    f = os.path.join(COV, "members.tsv")
    mem = defaultdict(list)
    if os.path.exists(f):
        for ln in open(f).read().splitlines()[1:]:
            p = ln.split("\t")
            if len(p) == 6:
                mem[p[0]].append((p[1], p[3], p[4], p[5]))  # offset, cpptype, name, count
    return mem


def our_methods():
    cls = defaultdict(set)
    for so in (HLSO, CLSO):
        if os.path.exists(so):
            nm = subprocess.run(f"nm '{so}' | c++filt", shell=True, capture_output=True, text=True).stdout
            for ln in nm.splitlines():
                m = re.search(r"\b([A-Z]\w+)::~?(\w+)\s*\(", ln)
                if m:
                    cls[m.group(1)].add(m.group(2))
    return cls


def our_files():
    cf = defaultdict(set)
    defre = re.compile(r"\b(C[A-Za-z0-9_]+)::~?\w+\s*\(")
    for d in ("dlls", "cl_dll"):
        dd = os.path.join(ROOT, d)
        for fn in sorted(os.listdir(dd)):
            if fn.endswith(".cpp"):
                for m in defre.finditer(open(os.path.join(dd, fn), errors="replace").read()):
                    cf[m.group(1)].add(f"{d}/{fn}")
    return cf


def main():
    cf, work, mem, um, ouf = class_file(), load_worklist(), load_members(), our_methods(), our_files()
    sigs, base, addr = load_sigs(), load_base(), func_addr()
    by_file = defaultdict(list)
    for c in work:
        by_file[cf.get(c, "?")].append(c)

    def cls_addr(c):
        return min((addr.get((c, m), 1 << 32) for m in work[c]), default=1 << 32)

    def emit_class(c, L):
        u = um.get(c, set())
        o = sorted(work[c], key=lambda m: addr.get((c, m), 1 << 32))   # ORIGINAL file order (by address)
        impl = sum(1 for m in o if m in u)
        decl = f"class {c}" + (f" : public {base[c]}" if c in base else "")
        ours = ", ".join(sorted(ouf.get(c, []))) or "—"
        L += [f"### `{decl}` — {impl}/{len(o)} fns · {len(mem.get(c, []))} members · ours: {ours}", ""]
        for m in o:
            mark = "✓" if m in u else "✗"
            L.append(f"- {mark} `{sigs.get((c, m), m + '(…)')}`")
        if mem.get(c):
            L += ["", "members: " + ", ".join(f"`{cpp} {nm}`(+{off})" for off, cpp, nm, cnt in mem[c])]
        L.append("")

    arg = sys.argv[1] if len(sys.argv) > 1 else None
    if arg:
        L = []
        if arg in by_file:
            L.append(f"# {arg}")
            for c in sorted(by_file[arg], key=cls_addr):
                emit_class(c, L)
        elif arg in work:
            emit_class(arg, L)
        else:
            L.append(f"no class/file '{arg}' in the work-list")
        print("\n".join(L))
        return

    tot_fn = sum(len(work[c]) for c in work)
    tot_impl = sum(sum(1 for m in work[c] if m in um.get(c, set())) for c in work)
    tot_mem = sum(len(mem.get(c, [])) for c in work)
    L = ["# TS inventory + refactor blueprint", "",
         "Per ORIGINAL file (and classes/functions in the original ADDRESS order), with recovered typed "
         "signature, base class, saved members, ✓impl/✗missing in our build, and WHERE we implement each "
         "today. Generated by `scripts/coverage/inventory.py` (composes filemap + baseline + signatures + "
         "hierarchy + members + our tree). The Phase-2 refactor target — move/rename to match, then re-run.", "",
         f"**{len(work)} TS classes across {len(by_file)} original files · "
         f"{tot_impl}/{tot_fn} functions implemented · {tot_mem} saved members.**", ""]
    for f in sorted(by_file):
        L += [f"## {f}", ""]
        for c in sorted(by_file[f], key=cls_addr):
            emit_class(c, L)
    open(OUT, "w").write("\n".join(L) + "\n")
    print(f"wrote {OUT}: {len(work)} classes / {len(by_file)} files / "
          f"{tot_impl}/{tot_fn} fns / {tot_mem} members")


if __name__ == "__main__":
    main()
