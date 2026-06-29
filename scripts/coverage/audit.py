#!/usr/bin/env python3
"""
audit.py — check OUR reconstruction against the RE'd original structure (ts_inventory.json), BOTH directions,
at FILE / CLASS / METHOD level. Answers "did we invent something they don't have, or miss something they do?"

  MISSING  = in the original (json) but NOT in our build  -> reconstruction work-list.
  INVENTED = in OUR build but NOT in the original          -> flag for review (a helper we added, a wrong
             file/class placement, or a genuine fabrication vs the RE). Per the mission, drop-in fidelity
             means our structure should map onto the original's; invented symbols are where we drifted.

Scope: TS-SPECIFIC classes only (the reconstruction). Stock SDK classes are skipped — we build on the newer
Itanium SDK whose stock method set legitimately differs from the original's 2007 gcc2 SDK, so diffing those
is noise (that's what divergence.py is for). The json marks ts_specific.

Sources: ORIGINAL from reference/coverage/ts_inventory.json (file->class->methods/members, authoritative — run
run.py/export.py first). OURS from `nm build-amd64/{dlls/hl.so,cl_dll/client.so} | c++filt` (methods) + a source
scan of dlls/ + cl_dll/ (class->our file). Methods matched by NAME (overloads collapse), ctors/dtors ignored.

  python3 scripts/coverage/audit.py            # full report -> reference/coverage/AUDIT.md (+ summary to stdout)
  python3 scripts/coverage/audit.py <Class>    # one class's bidirectional diff to stdout
  python3 scripts/coverage/audit.py --strict   # exit 1 if any INVENTED class/method on a TS class (CI gate)
"""
import json, os, re, subprocess, sys
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
JSON = os.path.join(ROOT, "reference", "coverage", "ts_inventory.json")
HLSO = os.path.join(ROOT, "build-amd64", "dlls", "hl.so")
CLSO = os.path.join(ROOT, "build-amd64", "cl_dll", "client.so")
SRC_DIRS = [os.path.join(ROOT, "dlls"), os.path.join(ROOT, "cl_dll")]
OUT = os.path.join(ROOT, "reference", "coverage", "AUDIT.md")


def norm_file(f):
    """normalize a source filename for cross-naming comparison: TS_gun.cpp ~ ts_gun.cpp; drop dir + ext."""
    return os.path.basename(f).lower().rsplit(".", 1)[0].lstrip("ts_").replace("manager", "")


def load_original():
    """from the json: {class: {file, ts_specific, methods:set, members:set}} + {file: set(class)} for TS files.
    Also folds in the CLIENT classes (json `client.members`) so HUD classes aren't mistaken for invented — they
    live in client.dll, not the server `files` tree."""
    d = json.load(open(JSON))
    classes, file_classes = {}, defaultdict(set)
    for fn, fe in d.get("files", {}).items():
        for c, ci in fe.get("classes", {}).items():
            mem = ci.get("members", [])
            classes[c] = {
                "file": fn,
                "ts_specific": ci.get("ts_specific", False),
                "methods": set(ci.get("methods", {}).keys()),
                "member_count": len(mem),
                # REAL names only (saved/curated) — the `auto` skeleton is field_0xNN placeholders, not name-matchable
                "named_members": {m["name"] for m in mem
                                  if m.get("source") in ("saved", "curated") and not m["name"].startswith("field_0x")},
            }
            if ci.get("ts_specific"):
                file_classes[fn].add(c)
    for c, cm in d.get("client", {}).get("members", {}).items():   # HUD/prediction classes recovered client-side
        classes.setdefault(c, {"file": "client.dll", "ts_specific": c.startswith(("CHudTS", "CTS")),
                               "methods": set(), "member_count": len(cm),
                               "named_members": {m["name"] for m in cm if not m["name"].startswith("field_0x")}})
    return classes, file_classes


def our_methods():
    """{class: set(method)} from our built .so symbols (demangled). No filter — we need all to look up."""
    cls = defaultdict(set)
    pat = re.compile(r"\b([A-Z]\w+)::~?(\w+)\s*\(")
    for so in (HLSO, CLSO):
        if not os.path.exists(so):
            continue
        nm = subprocess.run(f"nm '{so}' | c++filt", shell=True, capture_output=True, text=True).stdout
        for ln in nm.splitlines():
            m = pat.search(ln)
            if m:
                cls[m.group(1)].add(m.group(2))
    return cls


def our_class_files():
    """{class: set(our_file)} from out-of-line method defs in our sources."""
    cf = defaultdict(set)
    defre = re.compile(r"\b(C[A-Za-z0-9_]+)::~?\w+\s*\(")
    for d in SRC_DIRS:
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not fn.endswith(".cpp"):
                continue
            txt = open(os.path.join(d, fn), errors="replace").read()
            for m in defre.finditer(txt):
                cf[m.group(1)].add(os.path.relpath(os.path.join(d, fn), ROOT))
    return {c: sorted(fs) for c, fs in cf.items()}


    # Server TS class-name pattern. We restrict the INVENTED-class flag to these: the SERVER inventory (json
    # files tree) is complete, so "TS-named class we have but the original doesn't" is a real fabrication. We
    # do NOT flag invented CHud*/client classes — the client inventory (binmap-matched) is incomplete, so a
    # client class we have that wasn't matched is a false positive, not an invention.
TS_SERVER = re.compile(r"^(CTS|CM61|CVip|CBulletManager|CInfoBreakable|CBaseTS)")


def our_globals():
    """global/static DATA symbol names from our build (demangled), normalized with '.' (statics: Class.member)."""
    names = set()
    for so in (HLSO, CLSO):
        if not os.path.exists(so):
            continue
        nm = subprocess.run(f"nm '{so}' | c++filt", shell=True, capture_output=True, text=True).stdout
        for ln in nm.splitlines():
            p = ln.split()
            if len(p) >= 3 and p[-2] in ("D", "d", "B", "b", "V", "v", "G", "g", "R", "r"):
                names.add(p[-1].split("(")[0].replace("::", "."))
    return names


def our_members():
    """{class: set(member_name)} from our dlls/ + cl_dll/ headers (m_* / lowercase member declarations)."""
    cls = defaultdict(set)
    classre = re.compile(r"^\s*(?:class|struct)\s+([A-Za-z_]\w*)")
    # member declarations: a type then the member name then =/;/{. Capture m_-prefixed OR a
    # lowercase-leading name (the original's FGD/save members are non-prefixed, e.g. tsweaponid).
    declre = re.compile(r"^\s*(?:const\s+)?[A-Za-z_][\w:<>,\s\*&]*?\b(m_[A-Za-z0-9_]+|[a-z][a-z0-9_]+)\s*(?:\[[^\]]*\])?\s*(?:=|;|\{)")
    # type keywords that can be the LAST token before ; (forward decls / bare types) — not members
    typeword = {"int","bool","char","float","double","void","short","long","unsigned","string_t","vec_t","byte","entvars_t"}
    for d in SRC_DIRS:
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not fn.endswith(".h"):
                continue
            cur = None
            for ln in open(os.path.join(d, fn), errors="replace"):
                cm = classre.match(ln)
                if cm:
                    cur = cm.group(1)
                if cur and "(" not in ln.split("//")[0]:        # skip method decls
                    dm = declre.match(ln)
                    if dm and dm.group(1) not in typeword:
                        cls[cur].add(dm.group(1))
    return cls


def analyze():
    orig, orig_file_classes = load_original()
    ours, ourcf = our_methods(), our_class_files()
    ourmem = our_members()
    ts_orig = {c for c, i in orig.items() if i["ts_specific"]}
    orig_all = set(orig)

    # CLASS level
    missing_classes = sorted(c for c in ts_orig if c not in ours and c.startswith("C"))   # orig TS, not built
    invented_classes = sorted(c for c in ourcf                                            # ours, NOT in original
                              if c not in orig_all and TS_SERVER.match(c))

    # METHOD level (per class present in BOTH)
    per_class = {}
    for c in sorted(ts_orig & set(ours)):
        o, u = orig[c]["methods"], ours[c]
        missing = sorted(o - u)
        invented = sorted(m for m in (u - o) if m != c and not m.startswith("~"))
        ofile = orig[c]["file"]
        our_files = ourcf.get(c, [])
        file_ok = any(norm_file(of) == norm_file(ofile) for of in our_files) if our_files else False
        # MEMBER diff: original REAL-named members (saved/curated) not present in our source headers, by name.
        # (Only the named ones — the auto skeleton is field_0xNN placeholders; and our modern layout uses
        # different OFFSETS by design, so we match on NAME, not offset.)
        missing_members = sorted(orig[c]["named_members"] - ourmem.get(c, set()))
        per_class[c] = {"missing": missing, "invented": invented, "orig_file": ofile,
                        "our_files": our_files, "file_ok": file_ok, "impl": len(o & u), "total": len(o),
                        "missing_members": missing_members, "orig_member_count": orig[c]["member_count"],
                        "our_member_count": len(ourmem.get(c, set()))}

    # FILE level (derived from classes): original TS files w/ no class built, our files holding invented classes
    orig_ts_files = sorted({orig[c]["file"] for c in ts_orig})
    missing_files = sorted(f for f in orig_ts_files
                           if not any(c in ours for c in orig_file_classes.get(f, ())))

    # GLOBALS level: the json's recovered globals (TS-relevant) vs our build's data symbols.
    d = json.load(open(JSON))
    orig_glob = {g["name"].replace("::", "."): g.get("file") for g in d.get("globals", [])}
    ourg = our_globals()
    missing_globals = sorted(n for n in orig_glob if n not in ourg)            # original global we don't define
    tsg = re.compile(r"^(gmsg|g_ts|g_TS|TS[A-Z]|g_pTS)")
    invented_globals = sorted(n for n in (ourg - set(orig_glob)) if tsg.match(n))   # TS-pattern, not in original

    return {"missing_classes": missing_classes, "invented_classes": invented_classes,
            "per_class": per_class, "missing_files": missing_files, "orig_ts_files": orig_ts_files,
            "missing_globals": missing_globals, "invented_globals": invented_globals, "orig_glob": orig_glob,
            "ourcf": ourcf, "orig": orig}


def main():
    if not os.path.exists(JSON):
        sys.exit("missing ts_inventory.json — run scripts/coverage/run.py (or export.py) first")
    a = analyze()

    if len(sys.argv) > 1 and not sys.argv[1].startswith("-"):
        c = sys.argv[1]
        pc = a["per_class"].get(c)
        o = a["orig"].get(c)
        print(f"# {c}")
        if not o:
            print("  NOT in the original (json) — INVENTED class" if c in a["ourcf"] else "  unknown class")
            return
        print(f"  original file : {o['file']}   ts_specific={o['ts_specific']}")
        print(f"  our file(s)   : {', '.join(a['ourcf'].get(c, ['(none)']))}"
              + ("" if not pc else ("   ✓ file matches" if pc["file_ok"] else "   ⚠ FILE MISMATCH")))
        if pc:
            print(f"  methods       : {pc['impl']}/{pc['total']} impl")
            print(f"    MISSING  (orig, not ours): {', '.join(pc['missing']) or '—'}")
            print(f"    INVENTED (ours, not orig): {', '.join(pc['invented']) or '—'}")
            print(f"  members       : {pc['our_member_count']} ours / {pc['orig_member_count']} orig")
            print(f"    MISSING named (orig real-named field we lack): {', '.join(pc['missing_members']) or '—'}")
        else:
            print("  not implemented in our build (MISSING class)")
        return

    inv_meth = {c: p["invented"] for c, p in a["per_class"].items() if p["invented"]}
    file_mismatch = {c: p for c, p in a["per_class"].items() if p["our_files"] and not p["file_ok"]}
    n_inv_meth = sum(len(v) for v in inv_meth.values())

    if "--strict" in sys.argv:
        bad = len(a["invented_classes"]) + n_inv_meth + len(a["invented_globals"])
        print(f"audit --strict: {len(a['invented_classes'])} invented classes, {n_inv_meth} invented methods, "
              f"{len(a['invented_globals'])} invented globals · {len(a['missing_classes'])} missing classes, "
              f"{len(a['missing_globals'])} missing globals.")
        sys.exit(1 if bad else 0)

    L = ["# Reconstruction audit — ours vs the original (ts_inventory.json)", "",
         "Bidirectional, TS-specific classes only. Generated by `scripts/coverage/audit.py`.",
         "MISSING = original has it, we don't (work-list). INVENTED = WE have it, the original doesn't "
         "(review: a helper, a misplacement, or drift from the RE).", "",
         f"**{len(a['missing_classes'])} missing classes · {len(a['invented_classes'])} invented classes · "
         f"{n_inv_meth} invented methods · {sum(len(p['missing']) for p in a['per_class'].values())} missing "
         f"methods · {sum(len(p['missing_members']) for p in a['per_class'].values())} missing named members · "
         f"{len(file_mismatch)} file mismatches · {len(a['missing_globals'])} missing globals · "
         f"{len(a['invented_globals'])} invented globals.**", ""]

    L += ["## INVENTED classes (we define; the original has nowhere)", ""]
    L += ([", ".join(f"`{c}`" for c in a["invented_classes"])] if a["invented_classes"] else ["_none_"])
    L += ["", "## MISSING classes (original TS class, not in our build)", ""]
    L += ([", ".join(f"`{c}`" for c in a["missing_classes"])] if a["missing_classes"] else ["_none_"])

    L += ["", "## INVENTED methods (ours, not in the original — per class)", "",
          "| class | original file | invented methods |", "|---|---|---|"]
    for c in sorted(inv_meth):
        L.append(f"| `{c}` | `{a['per_class'][c]['orig_file']}` | {', '.join(inv_meth[c])} |")
    if not inv_meth:
        L.append("| _none_ | | |")

    miss_meth = {c: p for c, p in a["per_class"].items() if p["missing"]}
    L += ["", "## MISSING methods (original has, we don't — per class)", "",
          "| class | impl/total | missing |", "|---|--:|---|"]
    for c in sorted(miss_meth):
        p = miss_meth[c]
        L.append(f"| `{c}` | {p['impl']}/{p['total']} | {', '.join(p['missing'])} |")
    if not miss_meth:
        L.append("| _none_ | | |")

    miss_mem = {c: p for c, p in a["per_class"].items() if p["missing_members"]}
    L += ["", "## MISSING members (original REAL-named field we don't have — per class; matched by NAME, our "
          "modern layout uses different offsets by design)", "",
          "| class | members ours/orig | missing named |", "|---|--:|---|"]
    for c in sorted(miss_mem):
        p = miss_mem[c]
        L.append(f"| `{c}` | {p['our_member_count']}/{p['orig_member_count']} | {', '.join(p['missing_members'])} |")
    if not miss_mem:
        L.append("| _none_ | | |")

    L += ["", "## FILE mismatches (our class lives in a file that doesn't match the original's)", "",
          "| class | original file | our file(s) |", "|---|---|---|"]
    for c, p in sorted(file_mismatch.items()):
        L.append(f"| `{c}` | `{p['orig_file']}` | {', '.join(p['our_files'])} |")
    if not file_mismatch:
        L.append("| _none_ | | |")

    L += ["", "## MISSING files (original TS file whose classes we haven't built)", "",
          (", ".join(f"`{f}`" for f in a["missing_files"]) or "_none_")]

    L += ["", "## GLOBALS — MISSING (original global/static we don't define)", "",
          "| global | original file |", "|---|---|"]
    for n in a["missing_globals"]:
        L.append(f"| `{n}` | `{a['orig_glob'].get(n) or '?'}` |")
    if not a["missing_globals"]:
        L.append("| _none_ | |")
    L += ["", "## GLOBALS — INVENTED (TS-named global we have, the original doesn't)", "",
          (", ".join(f"`{n}`" for n in a["invented_globals"]) or "_none_")]

    open(OUT, "w").write("\n".join(L) + "\n")
    print(f"wrote {OUT}: {len(a['missing_classes'])} missing / {len(a['invented_classes'])} invented classes, "
          f"{n_inv_meth} invented methods, {len(file_mismatch)} file mismatches, {len(a['missing_files'])} missing files")


if __name__ == "__main__":
    main()
