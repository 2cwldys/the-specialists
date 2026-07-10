#!/usr/bin/env python3
"""
dead-functions.py — find PROVABLY-dead code in the mod's COMPILED sources, so
stock-Half-Life scaffolding that TS never runs can be removed (cleaner recon).

Two reachability signals, high-confidence first:

  [A] Uncalled free functions.  A file-scope (non-member) function whose name
      appears nowhere in the compiled corpus except its own definition and
      prototype declarations is never called and never address-taken -> dead.
      (This is exactly how ev_hldm.cpp's unhooked EV_* handlers read.)

  [B] Unregistered entity classes.  A CBaseEntity-derived class with no
      LINK_ENTITY_TO_CLASS and not a base of any registered class is never
      spawned -> its Precache/Spawn are dead.  LOWER confidence: a class can be
      live via `new`/a global (client-predicted weapons, gamerules) — VERIFY.

Only compiled sources count (parsed from dlls/ + cl_dll/ CMakeLists target_sources);
a function referenced only from a non-compiled file is still dead.  Headers are
scanned for references (a call can live in an inline/header). Read-only.

Usage:
  ./dead-functions.py --code dlls cl_dll [--ts <gamedir>] [--valve <gamedir>]
"""
import argparse
import os
import re
import sys
from collections import defaultdict

FUNC_NAME_RE = re.compile(r'([A-Za-z_~][A-Za-z0-9_:]*)\s*\(')
_SKIP = {"if", "for", "while", "switch", "return", "sizeof", "defined", "else",
         "do", "case", "static_cast", "reinterpret_cast", "const_cast", "dynamic_cast"}
ASSET_RE = re.compile(r'"([A-Za-z0-9_][A-Za-z0-9_\-/\.]*\.(?:mdl|spr|wav|bmp|tga|bsp))"', re.I)
LINK_RE = re.compile(r'LINK_ENTITY_TO_CLASS\s*\(\s*[A-Za-z0-9_]+\s*,\s*([A-Za-z_]\w*)')
CLASS_RE = re.compile(r'\bclass\s+([A-Za-z_]\w*)\s*:\s*public\s+([A-Za-z_]\w*)')
# free functions the ENGINE or a macro registers even though no literal call
# appears in source -> not statically-detectable, exclude to avoid false deads.
#   HUD_/Game_/Sys_ : engine client/server callbacks
#   __MsgFunc_/__CmdFunc_ : HOOK_MESSAGE/HOOK_COMMAND macro-paste the name
ENGINE_PREFIX = ("HUD_", "Game_", "Sys_", "DLLEXPORT", "__MsgFunc_", "__CmdFunc_")
ENGINE_EXACT = {
    "GiveFnptrsToDll", "GetEntityAPI", "GetEntityAPI2", "GetNewDLLFunctions",
    "Initialize", "ClientFactory", "InitVGUISupportAPI", "F", "GetStudioModelInterface",
    "Draw", "Init", "VidInit", "Shutdown", "Reset",  # common HUD element engine hooks
    "GetClientColor", "ClientMoveEvent", "ClientLookEvent",
    # engine input PUSH callbacks (inputw32.cpp) — the engine calls these
    "IN_ClientMoveEvent", "IN_ClientLookEvent",
}
# a TS-reconstruction symbol: uncalled here usually means PENDING WIRING, not dead —
# never propose deleting these (that would remove reconstruction work, not clean it).
TS_SYMBOL_RE = re.compile(r'(?i)(^|_)ts|^CTS|weaponid|slowmotion|specialist')


def blank_strings_comments(lines):
    out, in_block = [], False
    for raw in lines:
        res, i, n, in_str = [], 0, len(raw), None
        while i < n:
            c, two = raw[i], raw[i:i + 2]
            if in_block:
                if two == "*/":
                    in_block = False; i += 2; continue
                i += 1; continue
            if in_str:
                if c == "\\":
                    i += 2; continue
                if c == in_str:
                    in_str = None
                i += 1; continue
            if two == "/*":
                in_block = True; i += 2; continue
            if two == "//":
                break
            if c in '"\'':
                in_str = c; i += 1; continue
            res.append(c); i += 1
        out.append("".join(res))
    return out


def function_spans(lines):
    """Top-level function definitions -> list of dicts {name,start,open,end}."""
    code = blank_strings_comments(lines)
    spans = []
    depth = 0
    cur = None
    sig = []
    sigline = openline = None
    for i, cl in enumerate(code):
        if depth == 0:
            s = cl.strip()
            if s and not s.startswith(("#", "}")):
                if not sig:
                    sigline = i + 1
                sig.append(s)
        before = depth
        depth += cl.count("{") - cl.count("}")
        if before == 0 and depth > 0 and cur is None:
            joined = " ".join(sig)
            if "(" in joined:
                cand = [m for m in FUNC_NAME_RE.findall(joined) if m not in _SKIP]
                if cand:
                    cur = cand[0]
                    openline = i + 1
        if depth < 0:
            depth = 0
        if before > 0 and depth == 0:
            if cur is not None:
                spans.append({"name": cur, "start": sigline, "open": openline, "end": i + 1})
            cur, sig, sigline, openline = None, [], None, None
        elif before == 0 and depth == 0 and sig and sig[-1].rstrip().endswith(";"):
            sig, sigline = [], None
    return spans


def compiled_basenames(root):
    """Parse <root>/CMakeLists.txt target_sources for compiled .cpp basenames."""
    names = set()
    cml = os.path.join(root, "CMakeLists.txt")
    if os.path.isfile(cml):
        with open(cml, encoding="latin-1") as fh:
            txt = fh.read()
        for m in re.findall(r'([A-Za-z0-9_./-]+\.cpp)', txt):
            names.add(os.path.basename(m))
    return names


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--code", nargs="+", default=["dlls", "cl_dll"])
    ap.add_argument("--ts", help="gamedir, to tag asset refs missing from the mod")
    ap.add_argument("--valve", help="valve base, to tag refs as stock-HL assets")
    args = ap.parse_args()

    ts_files = set()
    if args.ts:
        for dp, _d, fs in os.walk(args.ts):
            for f in fs:
                ts_files.add(os.path.relpath(os.path.join(dp, f), args.ts)
                             .replace("\\", "/").lower())

    def in_ts(ref):
        r = ref.lower()
        return r in ts_files or (r.endswith(".wav") and ("sound/" + r) in ts_files)

    # ---- gather compiled .cpp + all headers under the roots -----------------
    compiled_cpp = []          # (abspath, relpath)
    header_files = []
    for root in args.code:
        wanted = compiled_basenames(root)
        for dp, _d, fs in os.walk(root):
            for f in fs:
                ap_ = os.path.join(dp, f)
                if f in wanted and f.endswith(".cpp"):
                    compiled_cpp.append((ap_, os.path.relpath(ap_)))
                elif f.lower().endswith((".h", ".hpp")):
                    header_files.append((ap_, os.path.relpath(ap_)))

    # ---- corpus of stripped lines (compiled cpp + headers) ------------------
    corpus = []                # (relpath, lineno, stripped_text)
    file_lines = {}            # relpath -> raw lines (for asset extraction)
    for ap_, rel in compiled_cpp + header_files:
        try:
            with open(ap_, encoding="latin-1") as fh:
                raw = fh.readlines()
        except OSError:
            continue
        file_lines[rel] = raw
        for ln, s in enumerate(blank_strings_comments(raw), 1):
            corpus.append((rel, ln, s))

    # ---- collect free-function definitions in compiled cpp ------------------
    free_defs = []             # dict {name,file,start,open,end}
    for ap_, rel in compiled_cpp:
        for sp in function_spans(file_lines[rel]):
            if "::" in sp["name"]:
                continue       # method — handled by [B]
            sp["file"] = rel
            free_defs.append(sp)

    # ---- use-count each free function over the corpus -----------------------
    def is_declaration(text, name):
        """True only for a forward declaration `<returntype...> NAME(args);` — a
        type token must precede NAME and NAME must not be the first token (a bare
        `NAME(args);` is a CALL statement, not a declaration; `return NAME();` is
        a use)."""
        s = text.strip()
        if not re.match(r'^[A-Za-z_][\w\s\*&:<>,]*?\b' + re.escape(name)
                        + r'\s*\([^;{]*\)\s*;$', s):
            return False
        first = re.match(r'^([A-Za-z_]\w*)', s)
        return bool(first) and first.group(1) != name and first.group(1) != "return"

    def uses_of(name, deffile, dstart, dend):
        pat = re.compile(r'\b' + re.escape(name) + r'\b')
        n = 0
        for rel, ln, s in corpus:
            if not pat.search(s):
                continue
            if rel == deffile and dstart <= ln <= dend:
                continue                      # its own definition body/signature
            if is_declaration(s, name):
                continue                      # a forward declaration
            n += 1
        return n

    def engine_hook(name):
        return name.startswith(ENGINE_PREFIX) or name in ENGINE_EXACT

    def asset_refs(rel, start, end):
        out = []
        for ln in range(start, end + 1):
            if ln - 1 < len(file_lines[rel]):
                for m in ASSET_RE.finditer(file_lines[rel][ln - 1]):
                    out.append(m.group(1))
        return out

    def is_ts(name, rel):
        return bool(TS_SYMBOL_RE.search(name)) or "ts_" in os.path.basename(rel).lower()

    dead_free = []          # stock-HL uncalled -> safe cleanup
    ts_pending = []         # TS-symbol uncalled -> pending wiring, DO NOT delete
    for sp in free_defs:
        if engine_hook(sp["name"]) or sp["name"].startswith("~"):
            continue        # engine/macro entry points + destructors (implicit calls)
        u = uses_of(sp["name"], sp["file"], sp["start"], sp["end"])
        if u == 0:
            refs = asset_refs(sp["file"], sp["open"], sp["end"])
            row = (sp["file"], sp["start"], sp["name"], sorted(set(refs)))
            (ts_pending if is_ts(sp["name"], sp["file"]) else dead_free).append(row)

    # ---- [B] entity classes: registered + hierarchy -------------------------
    registered = set()
    bases = {}                 # class -> base
    for rel, raw in file_lines.items():
        for s in blank_strings_comments(raw):
            for m in LINK_RE.finditer(s):
                registered.add(m.group(1))
            for m in CLASS_RE.finditer(s):
                bases[m.group(1)] = m.group(2)
    # reachable = registered + all ancestors of registered
    reachable = set()
    for c in registered:
        x = c
        seen = set()
        while x and x not in seen:
            reachable.add(x)
            seen.add(x)
            x = bases.get(x)
    # entity-rooted? (CBaseEntity in the ancestry)
    def rooted_at_entity(c):
        x, seen = c, set()
        while x and x not in seen:
            if x in ("CBaseEntity", "CBaseAnimating", "CBaseToggle", "CBaseDelay",
                     "CBaseMonster", "CBasePlayerItem", "CBasePlayerWeapon"):
                return True
            seen.add(x); x = bases.get(x)
        return False

    # methods referencing missing assets whose class is unreachable
    dead_classes = defaultdict(list)   # class -> [(file,line,method,ref)]
    for ap_, rel in compiled_cpp:
        for sp in function_spans(file_lines[rel]):
            if "::" not in sp["name"]:
                continue
            cls = sp["name"].split("::", 1)[0]
            if cls in reachable or not rooted_at_entity(cls):
                continue
            refs = asset_refs(rel, sp["open"], sp["end"])
            for r in refs:
                dead_classes[cls].append((rel, sp["start"], sp["name"], r))

    # ---- report -------------------------------------------------------------
    def tag(ref):
        if not args.ts:
            return ""
        return "  (missing from ts/)" if not in_ts(ref) else "  (shipped in ts/)"

    print("=" * 78)
    print("PROVABLY-DEAD CODE  —  compiled sources:", ", ".join(args.code))
    print("=" * 78)
    print(f"compiled .cpp: {len(compiled_cpp)}   free functions: {len(free_defs)}   "
          f"registered entities: {len(registered)}")

    print("\n" + "-" * 78)
    print(f"[A] UNCALLED STOCK-HL FREE FUNCTIONS  ({len(dead_free)})  — safe cleanup, "
          "gut like ev_hldm (build verifies)")
    print("-" * 78)
    if not dead_free:
        print("  (none — every compiled stock free function is called/address-taken)")
    for fl, ln, name, refs in sorted(dead_free):
        astr = ("  assets: " + ", ".join(refs)) if refs else "  (no asset refs)"
        print(f"  {fl}:{ln}  {name}(){astr}")

    print("\n" + "-" * 78)
    print(f"[A2] UNCALLED TS-RECONSTRUCTION FUNCTIONS  ({len(ts_pending)})  — do NOT delete: "
          "uncalled usually = PENDING WIRING, not dead. For the owner to triage.")
    print("-" * 78)
    if not ts_pending:
        print("  (none)")
    for fl, ln, name, refs in sorted(ts_pending):
        astr = ("  assets: " + ", ".join(refs)) if refs else ""
        print(f"  {fl}:{ln}  {name}(){astr}")

    print("\n" + "-" * 78)
    print(f"[B] UNREGISTERED ENTITY CLASSES w/ asset refs  ({len(dead_classes)})  "
          "— LOWER confidence, VERIFY (may be new'd / client-predicted)")
    print("-" * 78)
    if not dead_classes:
        print("  (none)")
    for cls in sorted(dead_classes):
        rows = dead_classes[cls]
        assets = sorted({r for _f, _l, _m, r in rows})
        loc = rows[0][0]
        print(f"  {cls}  ({loc})  {len(assets)} asset ref(s):")
        for a in assets:
            print(f"       {a}{tag(a)}")


if __name__ == "__main__":
    main()
