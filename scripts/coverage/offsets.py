#!/usr/bin/env python3
"""
offsets.py — map the original TS 3.0 member offsets to where reTS puts them, per arch.

The original side is fixed (ts_i386.so never changes), so it lives in a checked-in spec:
reference/coverage/offset_map.tsv  (key, class, orig offset, reTS member path, kind, used_by, evidence).
The reTS side is measured: pahole reads each member's offset from a DEBUG build's DWARF.
From the binary the tool also derives each class's original object size (the push in its
GetClassPtr template) and how many of the class's own original methods touch each spec offset,
so a stale spec row shows up as 0 instead of passing silently.

  scripts/build.sh i386 / amd64 / aarch64 with BUILDTYPE=Debug first (pahole needs DWARF).

  python3 scripts/coverage/offsets.py                  # spec table (markdown) for every built arch
  python3 scripts/coverage/offsets.py --tsv out.tsv    # also write a machine-readable table
  python3 scripts/coverage/offsets.py --all            # every named original member (members.tsv +
                                                       #   members_re.tsv) joined to reTS by name
  python3 scripts/coverage/offsets.py --root <tree>    # read <tree>/build-<arch>/dlls/hl.so
  python3 scripts/coverage/offsets.py --so i386=/path/hl.so --so amd64=/path/hl.so
"""
import argparse, os, re, subprocess, sys
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ORIG = os.path.join(ROOT, "reference", "ts_i386.so")
COV = os.path.join(ROOT, "reference", "coverage")
ARCHES = ["i386", "amd64", "aarch64"]


def sh(*a):
    return subprocess.run(a, capture_output=True, text=True, errors="replace").stdout


# ---------------------------------------------------------------- reTS side (pahole)
class Layout:
    def __init__(self, so):
        self.so, self.cache = so, {}

    def cls(self, name):
        """{member: (offset, type)} and the class size, from `pahole -C`. None if not in DWARF."""
        if name in self.cache:
            return self.cache[name]
        out = sh("pahole", "-C", name, self.so)
        res = None
        if out.strip():
            members, depth = {}, 0
            for ln in out.splitlines():
                if depth == 1:
                    m = re.match(r"^\t([^\t].*?)\s+(\w+)((?:\[\d+\])*);\s*/\*\s*(\d+)\s+(\d+)", ln)
                    if m:
                        count = 1
                        for d in re.findall(r"\[(\d+)\]", m.group(3)):
                            count *= int(d)
                        elem = int(m.group(5)) // count if count else int(m.group(5))
                        members.setdefault(m.group(2), (int(m.group(4)), m.group(1).strip(), elem))
                depth += ln.count("{") - ln.count("}")
            sizes = re.findall(r"/\* size: (\d+)", out)
            res = (members, int(sizes[-1]) if sizes else None)
        self.cache[name] = res
        return res

    def member(self, cls, path):
        """Offset of a member path (m_TSAnim.m_iLastActivity, targets[2]), following embedded classes;
        an [i] index on a one-dimensional array adds i * element size."""
        off, cur = 0, cls
        for part in path.split("."):
            m = re.fullmatch(r"(\w+)(?:\[(\d+)\])?", part)
            info = self.cls(cur)
            if not m or not info or m.group(1) not in info[0]:
                return None
            o, typ, elem = info[0][m.group(1)]
            off += o + (int(m.group(2)) * elem if m.group(2) else 0)
            cur = re.sub(r"^(class|struct|union)\s+", "", typ)
        return off


def has_dwarf(so):
    return ".debug_info" in sh("readelf", "-S", "-W", so)


# ---------------------------------------------------------------- original side (binary)
def orig_disasm():
    return sh("objdump", "-d", "--no-show-raw-insn", ORIG).splitlines()


def gcc2_class(cls):
    return f"__{len(cls)}{cls}"


def orig_sizes(dis):
    """{class: object size}: GetClassPtr__H1Z<len><Class> pushes the size it allocates."""
    sizes, cur = {}, None
    for ln in dis:
        h = re.match(r"^[0-9a-f]+ <GetClassPtr__H1Z(\d+)(\w+?)_PX01_PX01>:", ln)
        if h:
            n = int(h.group(1)); cur = h.group(2)[:n] if len(h.group(2)) >= n else None
            continue
        if ln and not ln.startswith(" "):
            cur = None
            continue
        if cur and cur not in sizes:
            m = re.search(r"push\s+\$0x([0-9a-f]+)$", ln)
            if m and int(m.group(1), 16) >= 0x80:
                sizes[cur] = int(m.group(1), 16)
    return sizes


def orig_access(dis, cls, offs):
    """{offset: number of the class's own original methods that access it}."""
    tag, fns, cur = gcc2_class(cls), defaultdict(set), None
    pats = {o: re.compile(rf"[\s,]0x{o:x}\(%e[a-z]{{2}}\)") for o in offs}
    for ln in dis:
        h = re.match(r"^[0-9a-f]+ <([^>]+)>:", ln)
        if h:
            cur = h.group(1) if tag in h.group(1) else None
            continue
        if cur:
            for o, p in pats.items():
                if p.search(ln):
                    fns[o].add(cur)
    return {o: len(fns[o]) for o in offs}


# ---------------------------------------------------------------- tables
def load_spec():
    rows = []
    with open(os.path.join(COV, "offset_map.tsv")) as f:
        head = f.readline().rstrip("\n").split("\t")
        for ln in f:
            p = ln.rstrip("\n").split("\t")
            if len(p) >= len(head):
                r = dict(zip(head, p)); r["orig"] = int(r["orig"], 16); rows.append(r)
    return rows


def load_named_members():
    """(class, orig offset, name) from the save tables and the curated RE names."""
    out = {}
    for fn in ("members.tsv", "members_re.tsv"):
        path = os.path.join(COV, fn)
        if not os.path.exists(path):
            continue
        with open(path) as f:
            f.readline()
            for ln in f:
                p = ln.rstrip("\n").split("\t")
                if len(p) >= 5 and not p[4].startswith("field_0x"):
                    out.setdefault((p[0], int(p[1], 16)), p[4])
    return [(c, o, n) for (c, o), n in sorted(out.items())]


def hx(v):
    return f"0x{v:x}" if isinstance(v, int) else "-"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", default=ROOT, help="tree holding build-<arch>/dlls/hl.so")
    ap.add_argument("--so", action="append", default=[], help="arch=path override (repeatable)")
    ap.add_argument("--all", action="store_true", help="every named original member, joined by name")
    ap.add_argument("--tsv", help="also write the table as TSV")
    a = ap.parse_args()

    sos = {x: os.path.join(a.root, f"build-{x}", "dlls", "hl.so") for x in ARCHES}
    for o in a.so:
        k, v = o.split("=", 1); sos[k] = v
    lay = {}
    for x, so in sos.items():
        if not os.path.exists(so):
            continue
        if not has_dwarf(so):
            sys.exit(f"{so} has no DWARF: build it with BUILDTYPE=Debug scripts/build.sh {x}")
        lay[x] = Layout(so)
    if not lay:
        sys.exit(f"no hl.so found under {a.root}/build-<arch>/dlls (or pass --so arch=path)")
    arches = [x for x in ARCHES if x in lay] + [x for x in lay if x not in ARCHES]

    dis = orig_disasm()
    if a.all:
        rows = [{"key": f"{c}::{n}", "class": c, "orig": o, "rets_member": n, "kind": "field",
                 "used_by": "", "evidence": "members.tsv / members_re.tsv"} for c, o, n in load_named_members()]
    else:
        rows = load_spec()
    classes = sorted({r["class"] for r in rows})
    access = {}
    for c in classes:
        access.update({(c, o): n for o, n in orig_access(dis, c, [r["orig"] for r in rows if r["class"] == c]).items()})
    osize = orig_sizes(dis)

    hdr = ["key", "class", "orig", "orig fns"] + arches + ["kind", "used_by"]
    table, missing = [], 0
    for r in rows:
        vals = []
        for x in arches:
            v = None if r["rets_member"] == "-" else lay[x].member(r["class"], r["rets_member"])
            vals.append(v)
        if r["rets_member"] != "-" and all(v is None for v in vals):
            missing += 1
        table.append([r["key"], r["class"], hx(r["orig"]), str(access.get((r["class"], r["orig"]), 0))]
                     + [hx(v) for v in vals] + [r["kind"], r["used_by"]])
    for c in classes:
        sz = [lay[x].cls(c)[1] if lay[x].cls(c) else None for x in arches]
        table.append([f"(size of {c})", c, hx(osize.get(c)), ""] + [hx(v) for v in sz] + ["size", ""])

    print("| " + " | ".join(hdr) + " |")
    print("|" + "---|" * len(hdr))
    for t in table:
        print("| " + " | ".join(t) + " |")
    stale = [t[0] for t in table if t[3] == "0" and not t[0].startswith("(size")]
    print(f"\n{len(rows)} fields, {len(arches)} arches ({', '.join(arches)}). "
          f"reTS member not found on any arch: {missing}. Original offset never accessed by its class's "
          f"own methods: {len(stale)}" + (f" ({', '.join(stale[:12])}{' ...' if len(stale) > 12 else ''})" if stale else "") + ".")
    if a.tsv:
        with open(a.tsv, "w") as f:
            f.write("\t".join(hdr) + "\n")
            for t in table:
                f.write("\t".join(t) + "\n")


if __name__ == "__main__":
    main()
