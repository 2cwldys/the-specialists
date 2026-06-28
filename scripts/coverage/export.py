#!/usr/bin/env python3
"""
export.py — the OMNI export: compose EVERYTHING the coverage toolchain recovers into one
ts_inventory.json (the complete machine-readable TS source mapping).

It imports the other tools' functions (inventory/baseline/divergence) rather than re-deriving — the
"omni" pattern. The JSON is a COMPLETE dump (not just the TS work-list): every class the binary defines,
tagged ts_specific + category (pure_ts / extended / stock), each with base/ancestry, original+our file,
functions (typed signature + addr + impl + our-extra/renamed), members (name/offset/type), statics, and
vtable; plus all globals, the SDK baseline/stripped/trimmed analysis, and the divergence/pollution map.
This is what a web viz or a Ghidra-feedback script consumes.

Run the chain first (scripts/coverage/run.py), then:
  python3 scripts/coverage/export.py        -> reference/coverage/ts_inventory.json
"""
import json, os, re, subprocess, sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import inventory as I            # class_file/func_addr/load_sigs/base/members (binary-side loaders)
import baseline as B            # ts_methods/base_methods (SDK subtraction — for the category tag)
import coverage as C            # extract_original (commands/cvars/usermsgs/events/entities)

COV = os.path.join(I.ROOT, "reference", "coverage")
OUT = os.path.join(COV, "ts_inventory.json")


def all_class_methods():
    """{class: {method: addr}} for EVERY class in ts_i386.so (complete, not just the work-list)."""
    nm = subprocess.run(f"nm '{I.SO}'", shell=True, capture_output=True, text=True).stdout
    out = {}
    for ln in nm.splitlines():
        p = ln.split()
        if len(p) < 3 or p[-2] not in ("T", "t", "W"):
            continue
        sym = p[-1]
        if "__" not in sym:
            continue
        head = sym.partition("__")[0]
        c = I.classname(sym)
        if c and re.match(r"^[A-Za-z]\w*$", head):
            out.setdefault(c, {}).setdefault(head, int(p[0], 16))
    return out


def sig_rows():
    rows = {}
    f = os.path.join(COV, "signatures.tsv")
    if os.path.exists(f):
        for ln in open(f).read().splitlines()[1:]:
            p = ln.split("\t")
            if len(p) == 7:
                rows[(p[0], p[1])] = {"return": p[2], "params": p[3], "qual": p[4],
                                      "virtual": p[5] == "1", "source": p[6]}
    return rows


def load_precache():
    """({(class,method): [asset]}, {free_name: [asset]}) — assets at FUNCTION granularity."""
    f = os.path.join(COV, "precache.tsv")
    by_method, by_free = defaultdict(list), defaultdict(list)
    if os.path.exists(f):
        for ln in open(f).read().splitlines()[1:]:
            p = ln.split("\t")
            if len(p) != 3:
                continue
            func, kind, value = p
            a = (kind, value)
            c = I.classname(func)
            head = func.partition("__")[0]
            if c and re.match(r"^[A-Za-z]\w*$", head):
                by_method[(c, head)].append(a)
            elif "__F" in func:
                by_free[func.split("__F")[0]].append(a)
            elif "__" not in func:
                by_free[func].append(a)
    return by_method, by_free


def split_refs(refs):
    """[(kind,value)] -> {assets:[{kind,asset}], cvars:[...], commands:[...]}."""
    return {
        "assets": [{"kind": k, "asset": v} for k, v in refs if k not in ("cvar", "command")],
        "cvars": sorted({v for k, v in refs if k == "cvar"}),
        "commands": sorted({v for k, v in refs if k == "command"}),
    }


def addr_to_file():
    """vaddr -> original .cpp (per-TU gcc2_compiled. ranges) — for attributing FREE functions to files."""
    out = subprocess.check_output(["readelf", "-sW", I.SO], text=True)
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
    ranges = [(a, (starts[i + 1][0] if i + 1 < len(starts) else 1 << 32), f)
              for i, (a, f) in enumerate(starts)]
    fa = {}
    for ln in subprocess.run(f"nm '{I.SO}'", shell=True, capture_output=True, text=True).stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[1] in "TtWw":
            try:
                fa[p[2]] = int(p[0], 16)
            except ValueError:
                pass

    def of(fnname):
        a = fa.get(fnname)
        return next((f for s, e, f in ranges if a is not None and s <= a < e), None)
    return of


def load_commands(func_file):
    f = os.path.join(COV, "commands.tsv")
    out = []
    if os.path.exists(f):
        for ln in open(f).read().splitlines()[1:]:
            p = ln.split("\t")
            if len(p) >= 3:
                name, kind, home = p[0], p[1], p[2]
                handler = p[3] if len(p) > 3 and p[3] else None
                out.append({"name": name, "kind": kind, "home_function": home,
                            "home_file": func_file(home), "handler": handler})
    return out


def free_function_sigs():
    """{name: demangled signature} for FREE (non-class) functions, from signatures-gcc2.txt."""
    out = {}
    g = os.path.join(I.ROOT, "reference", "signatures-gcc2.txt")
    if os.path.exists(g):
        for ln in open(g, errors="replace"):
            ln = ln.strip()
            if "::" in ln:
                continue
            m = re.match(r"^([A-Za-z_]\w*)\((.*)\)$", ln)
            if m:
                out.setdefault(m.group(1), ln)
    return out


def free_functions(func_file):
    """{file: [{name, addr, signature}]} for every NON-class function in ts_i386.so — needed for a
    full reconstruction (UTIL_*, Write_<weapon>, TSExplosion, ApplyMultiDamage, the C entry points…)."""
    sigs = free_function_sigs()
    out = defaultdict(list)
    seen = set()
    nm = subprocess.run(f"nm '{I.SO}'", shell=True, capture_output=True, text=True).stdout
    for line in nm.splitlines():
        p = line.split()
        if len(p) != 3 or p[1] not in "TtW":
            continue
        addr, sym = p[0], p[2]
        if re.match(r"^[A-Za-z]\w*__F", sym):          # gnu-v2 free function: name__F<args>
            name = sym.split("__F")[0]
        elif "__" not in sym and re.match(r"^[A-Za-z]\w*$", sym):   # extern "C" (engine entry points)
            name = sym
        else:
            continue
        if name in seen or name == "gcc2_compiled":
            continue
        seen.add(name)
        out[func_file(sym)].append({"name": name, "addr": f"0x{int(addr, 16):x}",
                                    "signature": sigs.get(name)})
    return out


def global_files(func_file):
    """{global_name: home_file} — data symbols carry no file, so we attribute each global to the file
    of the function that references it most (via its GOT slot). Reference-derived (approximate): a
    widely-read global resolves to its heaviest user, which is usually—not always—its defining TU."""
    from collections import Counter
    glob = {}   # GOT slot vaddr -> global name
    for ln in subprocess.run(f"readelf -rW '{I.SO}'", shell=True, capture_output=True, text=True).stdout.splitlines():
        if "GLOB_DAT" in ln:
            p = ln.split()
            try:
                glob[int(p[0], 16)] = p[-1]
            except ValueError:
                pass
    GOT = 0x165e10
    refs = defaultdict(Counter)
    dis = subprocess.run(["objdump", "-d", "--no-show-raw-insn", I.SO], capture_output=True, text=True).stdout
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
            if off > 0x7fffffff:
                off -= 0x100000000
            slot = GOT + off
            if slot in glob:
                f = func_file(cur)
                if f:
                    refs[glob[slot]][f] += 1
    return {g: c.most_common(1)[0][0] for g, c in refs.items() if c}


def load_globals():
    f = os.path.join(COV, "globals.tsv")
    statics, frees = {}, []
    if os.path.exists(f):
        for ln in open(f).read().splitlines()[1:]:
            p = ln.split("\t")
            if len(p) != 5:
                continue
            scope, name, addr, size, sect = p
            row = {"name": name.split(".", 1)[-1], "addr": addr, "size": int(size), "section": sect}
            if scope == "global":
                frees.append({**row, "name": name})
            else:
                statics.setdefault(name.split(".", 1)[0], []).append(row)
    return statics, frees


def ancestry(c, base):
    out, seen = [c], {c}
    while c in base and base[c] not in seen:
        c = base[c]; out.append(c); seen.add(c)
    return out


def client_surface():
    """The recoverable client.dll surface. client.dll is EXPORT-ONLY: the TS-specific internal HUD classes
    are stripped (not in its symbols), so only (a) the extern-C engine interface (HUD_*/Initialize — the
    cldll spine we DO reimplement) and (b) the VGUI library classes it links (vgui::*, NOT TS source) are
    recoverable. Commands/cvars are in `contract`."""
    exports = sorted(C.pe_exports(C.ORIG_CLIENT)) if os.path.exists(C.ORIG_CLIENT) else []
    interface = sorted(e for e in exports if not e.startswith("?"))    # extern "C" = the cldll interface
    libcls = set()
    p = os.path.join(I.ROOT, "reference", "signatures-client.txt")
    if os.path.exists(p):
        for ln in open(p, errors="replace"):
            ln = ln.strip()
            if ln.startswith("?") or "::" not in ln:
                continue
            m = re.search(r"(\S+)::~?\w+\s*\(", ln)   # the qualified class token before ::method(
            if m:
                cls = m.group(1)
                if cls and (cls[0].isalpha() or cls[0] == "_"):
                    libcls.add(cls)
    return interface, sorted(libcls)


def main():
    allm = all_class_methods()
    work, cf = I.load_worklist(), I.class_file()
    sigs, base, mem = I.load_sigs(), I.load_base(), I.load_members()
    addr = I.func_addr()
    srows = sig_rows()
    statics, frees = load_globals()
    by_method, by_free = load_precache()
    func_file = addr_to_file()
    commands = load_commands(func_file)
    gfile = global_files(func_file)             # global name -> home file (reference-derived)
    for g in frees:
        g["file"] = gfile.get(g["name"])        # file -> global becomes navigable
    vts = json.load(open(os.path.join(COV, "vtables_original.json"))) \
        if os.path.exists(os.path.join(COV, "vtables_original.json")) else {}

    # SDK subtraction (both directions) — reuse baseline's computation
    ts_b, base_b = B.ts_methods(), B.base_methods()
    pure_ts = {c for c in ts_b if c not in base_b}
    extended = {c for c in ts_b if c in base_b and (ts_b[c] - base_b[c])}
    realcls = lambda c: c.startswith("C") and not c.startswith("__")
    stripped = sorted(c for c in base_b if c not in ts_b and realcls(c))
    trimmed = {c: sorted(base_b[c] - ts_b[c]) for c in (set(base_b) & set(ts_b))
               if (base_b[c] - ts_b[c]) and realcls(c)}

    def category(c):
        return "pure_ts" if c in pure_ts else "extended" if c in extended else "stock"

    ff = free_functions(func_file)   # {file: [{name,addr,signature}]} — non-class functions

    # ---- build the NESTED tree: file -> {classes -> {methods -> {assets,...}}, free_functions, globals}
    universe = sorted(set(allm) | set(mem) | set(vts) | set(base) | set(work))
    tree = {}

    def node(f):
        return tree.setdefault(f, {"classes": {}, "free_functions": {}, "globals": []})

    class_index, tot_methods, n_classes = {}, 0, 0
    for c in universe:
        methods = allm.get(c, {})
        ofile = cf.get(c, "?")
        class_index[c] = ofile
        ts_specific = c in work
        n_classes += 1
        mdict = {}
        for m in sorted(methods, key=lambda x: methods[x]):
            sr = srows.get((c, m), {})
            mdict[m] = {
                "addr": f"0x{methods[m]:x}", "signature": sigs.get((c, m)),
                "return": sr.get("return"), "params": sr.get("params"),
                "virtual": sr.get("virtual", False), "qual": sr.get("qual", ""),
                "sig_source": sr.get("source"),
                "ts_specific": (not ts_specific) or (m in work.get(c, set())),
                # precached/played assets + cvar/command refs for THIS method
                **split_refs(by_method.get((c, m), [])),
            }
        tot_methods += len(mdict)
        node(ofile)["classes"][c] = {
            "ts_specific": ts_specific, "category": category(c),
            "base": base.get(c), "ancestry": ancestry(c, base),
            "members": [{"name": nm, "offset": off, "cpptype": cpp, "count": cnt}
                        for off, cpp, nm, cnt in mem.get(c, [])],
            "statics": statics.get(c, []),
            "vtable": vts.get(c),
            "methods": mdict,
        }
    tot_free = 0
    for f, items in ff.items():
        for fn in items:
            node(f)["free_functions"][fn["name"]] = {
                "addr": fn["addr"], "signature": fn["signature"],
                **split_refs(by_free.get(fn["name"], [])),
            }
            tot_free += 1
    for g in frees:
        if g.get("file"):
            node(g["file"])["globals"].append(g)

    doc = {
        "meta": {
            "source": "ts_i386.so + mp.dll/client.dll (the ORIGINAL TS) vs halflife-updated (base SDK, "
                      "for the category tag only)",
            "scope": "PURE reference dump of the original TS — NO information about our codebase. "
                     "Generate a same-schema snapshot of our build separately and json-diff the two.",
            "layout": "files -> {classes -> {methods -> {assets,...}}, free_functions, globals}, fully "
                      "nested. class_index maps a class name to its file for quick lookup.",
            "files": len(tree), "classes": n_classes, "ts_specific_classes": len(work),
            "class_methods": tot_methods, "free_functions": tot_free,
            "members": sum(len(cl["members"]) for n in tree.values() for cl in n["classes"].values()),
            "globals": len(frees),
            "note": "ts_specific flags the TS work-list; offsets are original-gcc2 provenance.",
        },
        "files": {f: tree[f] for f in sorted(tree)},
        "class_index": class_index,
        "globals": frees,
        "sdk": {
            "baseline_methods": sum(len(v) for v in base_b.values()),
            "baseline_classes": len(base_b),
            "pure_ts_classes": sorted(pure_ts),
            "extended_classes": {c: sorted(ts_b[c] - base_b[c]) for c in sorted(extended)},
            "stripped_classes": stripped,
            "trimmed_methods": trimmed,
        },
    }
    # external contract of the ORIGINAL TS: entities, commands, cvars(+defaults), usermsgs(+sizes),
    # events, FGD keyvalues. cvars = cvar_t STRUCT scan (robust — not the registration chain);
    # commands/usermsgs = from their registration sites. ORIGINAL only (no "ours"). See coverage.py.
    try:
        doc["contract"] = C.extract_original()
    except Exception as e:
        doc["contract"] = {"error": str(e)}
    doc["commands"] = commands   # name -> kind/home_function/home_file/handler

    # client.dll surface. The `files` tree above is the SERVER (ts_i386.so, unstripped). client.dll is
    # export-only (symbols = VGUI library + extern-C engine interface; the TS HUD classes aren't exported and
    # have no RTTI) — BUT their MEMBER LAYOUTS are recovered by binmap-matching our built client.so -> client.dll
    # (scripts/coverage/client_members.py -> members_auto_client.tsv). Fold those in here.
    interface, libcls = client_surface()
    client_members = {}
    cmf = os.path.join(I.ROOT, "reference", "coverage", "members_auto_client.tsv")
    if os.path.exists(cmf):
        with open(cmf) as f:
            next(f, None)
            for ln in f:
                p = ln.rstrip("\n").split("\t")
                if len(p) >= 5:
                    client_members.setdefault(p[0], []).append({"offset": p[1], "type": p[3], "name": p[4]})
    doc["client"] = {
        "note": "client.dll is export-only (its symbols are the VGUI library + the extern-C engine interface; "
                "the TS HUD classes are NOT exported and have NO RTTI). The HUD/prediction class MEMBER LAYOUTS "
                "are nonetheless recovered (`recovered_members` below) by binmap-matching our built client.so "
                "(full symbols) -> client.dll and reading each function's this+offset accesses — see "
                "scripts/coverage/client_members.py. Client commands/cvars are in `contract`; the server "
                "(ts_i386.so, unstripped) is the full `files` tree.",
        "interface_exports": interface,
        "vgui_library_classes": libcls,
        "recovered_member_classes": len(client_members),
        "recovered_members": sum(len(v) for v in client_members.values()),
        "members": client_members,
    }

    json.dump(doc, open(OUT, "w"), indent=1)
    m = doc["meta"]
    print(f"wrote {OUT}: {m['classes']} classes ({m['ts_specific_classes']} TS) · {m['class_methods']}+{m['free_functions']} fns · "
          f"{m['members']} members · {m['globals']} globals · {len(stripped)} stripped · "
          f"{sum(len(v) for v in trimmed.values())} trimmed")


if __name__ == "__main__":
    main()
