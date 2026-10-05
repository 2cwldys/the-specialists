#!/usr/bin/env python3
"""
baseline.py — the SDK BASELINE / exception list: what the base SDK (halflife-updated) itself produces,
subtracted from the original TS binary so the TS-specific work-list is DATA-DRIVEN, not a hand-coded
class-name regex.

The original ts_i386.so contains BOTH stock HL SDK classes (CBaseEntity, CBreakable, doors, …) and the
TS-specific code. Our base SDK produces the stock set; anything in ts_i386.so NOT produced by the base
SDK is the genuine TS reconstruction target. Comparison is METHOD-level, so it also catches methods TS
*added* to a stock class (e.g. CBreakable::BreakableRespawn) while ignoring the inherited ones.

Caveats it surfaces (intended): our base SDK is NEWER than the 2007 SDK TS used, so a stock method
renamed/removed between them shows up here too — that's the "how does our SDK differ from theirs" signal.

Outputs:
  reference/coverage/SDK_BASELINE.md       — human report (pure-TS classes, stock classes TS extended)
  reference/coverage/sdk_baseline.txt      — machine-readable ignore list: one `Class::method` per line
                                             (load this in layout.py/reconstruction.py to drop stock SDK)

Self-contained (stdlib + nm/c++filt). Base path: argv[1] or $HLU_BASE or the default.

  python3 scripts/coverage/baseline.py
"""
import os, re, sys, subprocess
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SO   = os.path.join(ROOT, "reference", "ts_i386.so")
BASE = (sys.argv[1] if len(sys.argv) > 1 else
        os.environ.get("HLU_BASE",
        os.path.expanduser("~/Projects/goldsrc.net/hlds64-buildchain/halflife-updated")))
BASE_SOS = ["build-amd64/dlls/hl.so", "build-amd64/cl_dll/client.so"]
OUT_MD   = os.path.join(ROOT, "reference", "coverage", "SDK_BASELINE.md")
OUT_TXT  = os.path.join(ROOT, "reference", "coverage", "sdk_baseline.txt")
OUT_WORK = os.path.join(ROOT, "reference", "coverage", "ts_specific.txt")


def _classname(mangled):
    m = re.match(r"^[A-Za-z0-9_]+__(\d+)([A-Za-z0-9_]+)", mangled)
    return m.group(2)[: int(m.group(1))] if m else None


def ts_methods():
    """{class: set(method)} for ALL classes in ts_i386.so (gnu-v2 mangled, parsed directly)."""
    nm = subprocess.run(f"nm '{SO}'", shell=True, capture_output=True, text=True).stdout
    cls = defaultdict(set)
    for ln in nm.splitlines():
        p = ln.split()
        if len(p) < 2 or p[-2] not in ("T", "t", "W"):
            continue
        sym = p[-1]
        if "__" not in sym:
            continue
        head = sym.partition("__")[0]
        c = _classname(sym)
        if c and re.match(r"^[A-Za-z]\w*$", head):  # skip ctor/dtor/operator/template
            cls[c].add(head)
    return cls


def base_methods_from_symbols():
    """{class: set(method)} the base SDK COMPILES (Itanium-mangled, c++filt demangles)."""
    cls = defaultdict(set)
    found = False
    for rel in BASE_SOS:
        so = os.path.join(BASE, rel)
        if not os.path.exists(so):
            continue
        found = True
        nm = subprocess.run(f"nm '{so}' | c++filt", shell=True, capture_output=True, text=True).stdout
        for ln in nm.splitlines():
            m = re.search(r"\b([A-Z]\w+)::~?(\w+)\s*\(", ln)
            if m and m.group(2) != m.group(1):  # drop ctor (method==class)
                cls[m.group(1)].add(m.group(2))
    if not found:
        sys.exit(f"no base SDK build under {BASE}/{{{','.join(BASE_SOS)}}} — build it first")
    return cls


_KW = {"if", "for", "while", "switch", "return", "else", "do", "sizeof", "case", "new", "delete",
       "void", "int", "char", "float", "bool", "const", "static", "virtual", "unsigned", "short",
       "long", "double", "inline", "explicit", "friend", "typedef", "struct", "union", "enum"}
_CLASS_RE = re.compile(r"^\s*class\s+([A-Za-z_]\w*)\b")
_MEM_RE = re.compile(r"\b([A-Za-z_]\w*)\s*\(")


def base_methods_from_headers():
    """{class: set(method)} DECLARED in a base SDK class body (.h AND .cpp — many HL entity classes,
    e.g. CShake/CGameScore, are defined inline in .cpp). Catches inline accessors the compiler inlines
    away (CBeam::GetBrightness, CBasePlayerItem::iFlags) that ts_i386.so emitted as real symbols —
    without this they'd masquerade as 'TS-added'; really they're our-SDK-vs-theirs differences."""
    cls = defaultdict(set)
    for root, dirs, files in os.walk(BASE):
        dirs[:] = [d for d in dirs if not d.startswith("build")]
        for fn in files:
            if not fn.endswith((".h", ".cpp")):
                continue
            txt = open(os.path.join(root, fn), errors="replace").read()
            txt = re.sub(r"/\*.*?\*/", "", txt, flags=re.S)   # strip block comments
            cur, cls_brace, depth, entered = None, 0, 0, False
            for raw in txt.splitlines():
                line = re.sub(r"//.*$", "", raw)
                if cur is None:
                    cm = _CLASS_RE.match(line)
                    if cm and not re.search(r";\s*$", line):   # not a forward decl
                        cur, cls_brace, entered = cm.group(1), depth, False
                elif depth == cls_brace + 1:                   # at the class-member level
                    for m in _MEM_RE.finditer(line):
                        name = m.group(1)
                        if (name not in _KW and name != cur and not name.startswith("operator")
                                and not name.isupper()):     # skip ALL_CAPS macros (STRING, ENT, …)
                            cls[cur].add(name)
                depth += line.count("{") - line.count("}")
                if cur is not None:
                    if depth > cls_brace:
                        entered = True                         # body opened
                    elif entered:                              # body closed → class done
                        cur = None
    return cls


def base_methods():
    """Union of what the base SDK compiles AND what its headers declare (the full ignore baseline)."""
    cls = base_methods_from_symbols()
    for c, ms in base_methods_from_headers().items():
        cls[c] |= ms
    return cls


def main():
    ts = ts_methods()
    base = base_methods()

    # ---- ADDED direction: what TS has that the base SDK doesn't (reconstruction targets) ----
    pure_ts = {}      # class entirely absent from base SDK
    extended = {}     # shared class, TS-added methods only
    for c, ms in ts.items():
        if c not in base:
            pure_ts[c] = ms
        else:
            added = ms - base[c]
            if added:
                extended[c] = added

    # ---- STRIPPED direction: what the base SDK has that TS doesn't (features TS removed) ----
    # Only real C-prefixed classes; note the caveat — base − TS conflates (a) genuine TS strips with
    # (b) classes our NEWER SDK added post-2007. Whole-class absences are mostly real strips (HL
    # weapons/monsters); method-level trims are noisier (newer-SDK accessors).
    realcls = lambda c: c.startswith("C") and not c.startswith("__")
    stripped_cls = {c: base[c] for c in base if c not in ts and realcls(c)}
    trimmed = {}      # shared class, base-only methods (TS dropped or our SDK added)
    for c in set(base) & set(ts):
        only = base[c] - ts[c]
        if only and realcls(c):
            trimmed[c] = only

    # ---- machine-readable ignore list: every base (Class::method) ----
    ignore = sorted(f"{c}::{m}" for c, ms in base.items() for m in ms)
    open(OUT_TXT, "w").write("\n".join(ignore) + "\n")

    # ---- machine-readable TS-specific work-list (pure-TS + stock-extended), consumed by
    #      layout.py / reconstruction.py so every tool agrees on "what is TS" (data-driven, not TS_RE).
    worklist = sorted([f"{c}::{m}" for c, ms in pure_ts.items() for m in ms] +
                      [f"{c}::{m}" for c, ms in extended.items() for m in ms])
    open(OUT_WORK, "w").write("\n".join(worklist) + "\n")

    ts_specific = sum(len(v) for v in pure_ts.values()) + sum(len(v) for v in extended.values())
    # The base binaries come from a working tree, so record exactly what they were built from and warn
    # when that is not reTS's fork point (upstream/64bit-demo): later halflife-updated commits or local
    # edits there would show up here as TS differences.
    git = lambda *a: subprocess.run(["git"] + list(a), capture_output=True, text=True).stdout.strip()
    b_sha, b_ref = git("-C", BASE, "rev-parse", "--short", "HEAD"), git("-C", BASE, "rev-parse", "--abbrev-ref", "HEAD")
    b_dirty = len(git("-C", BASE, "status", "--porcelain", "--", "dlls", "cl_dll").splitlines())
    fork = git("-C", ROOT, "rev-parse", "--short", "upstream/64bit-demo")
    prov = f"{b_ref} @ {b_sha}" + (f", {b_dirty} locally modified file(s) in dlls/cl_dll" if b_dirty else "")
    warn = ""
    if fork and (b_sha != fork or b_dirty):
        warn = (f"**Warning:** the base is not reTS's fork point (upstream/64bit-demo @ {fork}); "
                "differences in the base after that commit are counted here as TS differences.")
        sys.stderr.write(f"baseline.py: {warn.replace('**', '')}\n")
    L = ["# SDK baseline / exception list (base SDK vs TS binary)", "",
         f"Base SDK: `{BASE}` ({prov}; {', '.join(r for r in BASE_SOS if os.path.exists(os.path.join(BASE,r)))}, "
         "+ header decls). Generated by `scripts/coverage/baseline.py`. Subtracts what the base SDK "
         "produces from ts_i386.so (both directions, method-level).", ""] + ([warn, ""] if warn else []) + [
         f"**Base SDK: {sum(len(v) for v in base.values())} methods / {len(base)} classes · "
         f"TS binary: {sum(len(v) for v in ts.values())} methods / {len(ts)} classes.**", "",
         f"- **TS-ADDED** (reconstruct): {ts_specific} methods — {len(pure_ts)} pure-TS classes + "
         f"{len(extended)} stock classes TS extended.",
         f"- **TS-STRIPPED** (features TS removed): {len(stripped_cls)} whole classes + "
         f"{sum(len(v) for v in trimmed.values())} methods trimmed from {len(trimmed)} kept classes.", "",
         "Ignore list → `reference/coverage/sdk_baseline.txt` "
         f"({len(ignore)} `Class::method`) — load it to drop stock SDK from the coverage tools.", "",
         "## 1. Stock SDK classes TS *extended* — added methods to replicate (inherited ones ignored)", "",
         "| class | TS-added methods (the real targets) |", "|---|---|"]
    for c in sorted(extended):
        L.append(f"| `{c}` | {', '.join(sorted(extended[c]))} |")

    L += ["", "## 2. Pure-TS classes (absent from the base SDK entirely) — the core work-list", "",
          f"{len(pure_ts)} classes:", "",
          ", ".join(f"`{c}`({len(pure_ts[c])})" for c in sorted(pure_ts)), "",
          "## 3. STRIPPED whole classes — in the base SDK, ABSENT from TS (features/weapons/monsters TS removed)", "",
          "_Caveat: a few may instead be classes our newer SDK added post-2007, not TS strips._ "
          "Sorted by size (bigger = more substantial feature).", "",
          "| class | base methods | |", "|---|--:|---|"]
    for c in sorted(stripped_cls, key=lambda x: (-len(stripped_cls[x]), x)):   # name breaks ties: stable output
        methods = ", ".join(sorted(stripped_cls[c]))   # full list — never truncated
        L.append(f"| `{c}` | {len(stripped_cls[c])} | {methods} |")

    L += ["", "## 4. TRIMMED methods — kept classes where the base SDK has methods TS lacks", "",
          "_Noisier: mixes methods TS genuinely dropped with accessors our newer SDK added. "
          "Use for spotting removed behaviour on shared classes._", "",
          "| class | base-only methods |", "|---|---|"]
    for c in sorted(trimmed, key=lambda x: (-len(trimmed[x]), x)):
        ms = sorted(trimmed[c])
        L.append(f"| `{c}` | {', '.join(ms)} |")   # full list — never truncated

    open(OUT_MD, "w").write("\n".join(L) + "\n")
    print(f"wrote {OUT_MD} + {OUT_TXT} + {OUT_WORK}: baseline {len(ignore)} methods; "
          f"ADDED {ts_specific} ({len(pure_ts)} pure +{len(extended)} extended); "
          f"STRIPPED {len(stripped_cls)} classes +{sum(len(v) for v in trimmed.values())} trimmed methods")


if __name__ == "__main__":
    main()
