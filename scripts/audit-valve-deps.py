#!/usr/bin/env python3
"""
audit-valve-deps.py — find every asset a GoldSrc mod gamedir *references* but does
NOT ship, i.e. the assets that fall through to the `valve` (Half-Life) base.

Why: goldsrc.net serves mod content to browsers. Anything the mod references that
isn't inside its own gamedir resolves from Valve's base game -> that's the IP we'd
be redistributing. This script enumerates exactly those assets so they can be
replaced, removed, or licensed.

What it inspects:
  1. BSP maps (v30): worldspawn `wad` list, embedded vs external textures, and
     entity asset references (model/sprite/sound/skyname).
  2. WAD3 files shipped in the gamedir: builds a texture-name index so external
     BSP textures can be resolved against them.
  3. Mod source code: string literals that name assets (models/sprites/sounds/...).
  4. HUD sprite configs (sprites/*.txt).

For each referenced asset it decides: satisfied by the mod gamedir (OK), or not —
and if a `--valve` base is given, whether that base actually provides it (confirmed
Half-Life asset) vs a dangling reference that resolves to nothing.

Pure stdlib. Read-only. Usage:
  ./audit-valve-deps.py --ts <ts_gamedir> [--valve <valve_gamedir>] \
      [--code <src_root> ...] [--report out.txt]
"""

import argparse
import os
import re
import struct
import sys
from collections import defaultdict

# ---- texture names that are engine/compiler tools, not real content. They live
# ---- in halflife.wad/zhlt but render invisibly; bucket them apart so genuine
# ---- content-texture dependencies stand out.
TOOL_TEXTURES = {
    "aaatrigger", "clip", "clipbevel", "clipbevelbrush", "cliphull1", "cliphull2",
    "cliphull3", "contentempty", "contentwater", "hint", "skip", "null", "bevel",
    "origin", "sky", "boundingbox", "translucent", "noclip", "black_hidden",
    "solidhint", "splitface",
}

ASSET_EXT = ("mdl", "spr", "wav", "bmp", "tga", "bsp", "wad", "txt", "png")


def norm(p):
    """Lowercase, forward-slash, strip leading ./ and slashes."""
    p = p.replace("\\", "/").lower().strip()
    while p.startswith("./"):
        p = p[2:]
    return p.lstrip("/")


# --------------------------------------------------------------------------- #
# gamedir indexing
# --------------------------------------------------------------------------- #
def index_files(root):
    """relpath(normalized) -> absolute path, for every file under root."""
    out = {}
    if not root or not os.path.isdir(root):
        return out
    for dirpath, _dirs, files in os.walk(root):
        for f in files:
            ap = os.path.join(dirpath, f)
            rp = norm(os.path.relpath(ap, root))
            out[rp] = ap
    return out


def wad3_texture_names(path):
    """Return the set of lump (texture) names inside a WAD3 file (lowercased)."""
    names = set()
    try:
        with open(path, "rb") as fh:
            data = fh.read()
    except OSError:
        return names
    if len(data) < 12 or data[:4] != b"WAD3":
        return names
    numlumps, diroffset = struct.unpack_from("<ii", data, 4)
    ENTRY = 32  # filepos,disksize,size (3*4) + type,comp,pad[2] (4) + name[16]
    for i in range(numlumps):
        off = diroffset + i * ENTRY
        if off + ENTRY > len(data):
            break
        raw_name = data[off + 16 : off + 32]
        name = raw_name.split(b"\x00", 1)[0].decode("latin-1", "replace").lower()
        if name:
            names.add(name)
    return names


def gamedir_wads(files):
    """basename(lower) -> set(texture names) for every .wad in the file index."""
    out = {}
    for rp, ap in files.items():
        if rp.endswith(".wad"):
            out[os.path.basename(rp)] = wad3_texture_names(ap)
    return out


# --------------------------------------------------------------------------- #
# BSP parsing (GoldSrc v30)
# --------------------------------------------------------------------------- #
LUMP_ENTITIES = 0
LUMP_TEXTURES = 2
KV_RE = re.compile(rb'"([^"]*)"\s+"([^"]*)"')
ASSET_IN_VALUE_RE = re.compile(r'([^\s";]+\.(?:mdl|spr|wav|bsp))', re.I)


def parse_bsp(path):
    """
    Returns dict:
      wads:      list of wad basenames from worldspawn (lower)
      ext_tex:   set of external (WAD-resident) texture names (lower)
      emb_tex:   count of embedded textures (informational)
      refs:      set of asset path references from entities (mdl/spr/wav/bsp)
      skies:     set of skyname strings
      error:     str or None
    """
    res = {"wads": [], "ext_tex": set(), "emb_tex": 0, "refs": set(),
           "skies": set(), "error": None}
    try:
        with open(path, "rb") as fh:
            data = fh.read()
    except OSError as e:
        res["error"] = str(e)
        return res
    if len(data) < 4 + 15 * 8:
        res["error"] = "too small"
        return res
    version = struct.unpack_from("<i", data, 0)[0]
    if version != 30:
        res["error"] = f"unexpected bsp version {version}"
        return res

    lumps = []
    for i in range(15):
        ofs, ln = struct.unpack_from("<ii", data, 4 + i * 8)
        lumps.append((ofs, ln))

    # --- entities lump (text) ---
    eofs, elen = lumps[LUMP_ENTITIES]
    ent = data[eofs : eofs + elen]
    cur_class = None
    for m in KV_RE.finditer(ent):
        key = m.group(1).decode("latin-1", "replace").lower()
        val = m.group(2).decode("latin-1", "replace")
        if key == "classname":
            cur_class = val.lower()
        if key == "wad":
            for w in val.split(";"):
                w = w.strip()
                if w:
                    res["wads"].append(os.path.basename(norm(w)))
        if key == "skyname" and val.strip():
            res["skies"].add(val.strip().lower())
        # any value that names a model/sprite/sound file
        for am in ASSET_IN_VALUE_RE.finditer(val):
            ref = norm(am.group(1))
            if not ref.startswith("*"):
                res["refs"].add(ref)

    # --- textures lump ---
    tofs, tlen = lumps[LUMP_TEXTURES]
    if tlen >= 4:
        nummip = struct.unpack_from("<i", data, tofs)[0]
        for i in range(nummip):
            dofs = struct.unpack_from("<i", data, tofs + 4 + i * 4)[0]
            if dofs < 0:
                continue
            base = tofs + dofs
            if base + 40 > len(data):
                continue
            raw = data[base : base + 16]
            name = raw.split(b"\x00", 1)[0].decode("latin-1", "replace").lower()
            # miptex: name[16], w(4), h(4), offsets[4](16); offsets[0]==0 => external
            off0 = struct.unpack_from("<I", data, base + 24)[0]
            if off0 == 0:
                if name:
                    res["ext_tex"].add(name)
            else:
                res["emb_tex"] += 1
    return res


# --------------------------------------------------------------------------- #
# code scanning
# --------------------------------------------------------------------------- #
CODE_EXTS = (".cpp", ".c", ".h", ".hpp", ".cc", ".cxx")
# string literal that looks like an asset path
CODE_ASSET_RE = re.compile(
    r'"([A-Za-z0-9_][A-Za-z0-9_\-/\.]*\.(?:mdl|spr|wav|bmp|tga|bsp))"', re.I
)
# identifier immediately preceding a '(' — used to name the enclosing function
FUNC_NAME_RE = re.compile(r'([A-Za-z_~][A-Za-z0-9_:]*)\s*\(')
_FUNC_SKIP = {"if", "for", "while", "switch", "return", "sizeof", "defined",
              "else", "do", "case", "static_cast", "reinterpret_cast"}
# enclosing-symbol names that mark a stock-HL weapon TS doesn't have -> DEAD hint.
# (shotgun/grenade deliberately OMITTED — TS has its own, so they're ambiguous.)
HL_WEAPON_RE = re.compile(
    r'(?i)(glock|python|\bmp5\b|mp52|gauss|egon|crossbow|\brpg\b|hornet|\bhgun\b|'
    r'satchel|tripmine|snark|squeak|crowbar|\bbolt\b|9mmar|\b357\b|hldm)')
# asset paths that are SHARED effects/world-entity audio TS genuinely plays ->
# LIVE (replace with generic, do NOT delete): player movement, gibs, breakable
# debris, and the door/button/platform/fan sounds every GoldSrc map triggers.
LIVE_SHARED_RE = re.compile(
    r'(?i)(player/pl_|/pl_|_step|step\d|npc_step|gibs?\.mdl|debris|bullet_hit|'
    r'ambience/|\bric\d|doors?/|buttons?/|fans?/|wheels?/|plats?/|/bodysplat)')


def _blank_strings_comments(lines):
    """Return each line with string/char-literal contents and comments blanked
    (so brace counting isn't corrupted by `{`/`}` inside them). Tracks block
    comments across lines."""
    out = []
    in_block = False
    for raw in lines:
        res = []
        i, n, in_str = 0, len(raw), None
        while i < n:
            c = raw[i]
            two = raw[i:i + 2]
            if in_block:
                if two == "*/":
                    in_block = False
                    i += 2
                    continue
                i += 1
                continue
            if in_str:
                if c == "\\":
                    i += 2
                    continue
                if c == in_str:
                    in_str = None
                i += 1
                continue
            if two == "/*":
                in_block = True
                i += 2
                continue
            if two == "//":
                break
            if c == '"' or c == "'":
                in_str = c
                i += 1
                continue
            res.append(c)
            i += 1
        out.append("".join(res))
    return out


def enclosing_functions(lines):
    """Approximate the enclosing top-level function name for each line (brace
    depth walk over string/comment-stripped code; good enough for HL-SDK
    column-0 function style). Returns a list parallel to `lines` of name-or-None."""
    code = _blank_strings_comments(lines)
    out = [None] * len(lines)
    depth = 0
    cur = None
    sig = []
    for i, cl in enumerate(code):
        if depth == 0:
            s = cl.strip()
            if s and not s.startswith(("#", "}")):
                sig.append(s)
        before = depth
        depth += cl.count("{") - cl.count("}")
        if before == 0 and depth > 0 and cur is None:
            joined = " ".join(sig)
            if "(" in joined:
                cand = [m for m in FUNC_NAME_RE.findall(joined) if m not in _FUNC_SKIP]
                cur = cand[0] if cand else None
        out[i] = cur
        if depth < 0:
            depth = 0
        if before > 0 and depth == 0:
            # a top-level block just closed -> we left the function
            cur, sig = None, []
        elif before == 0 and depth == 0 and sig and sig[-1].rstrip().endswith(";"):
            # a complete file-scope statement (prototype, LINK_ENTITY_TO_CLASS,
            # global) — not a pending function signature; drop the accumulation
            sig = []
    return out


def scan_code(roots):
    """Return {ref(normalized): [(relpath, lineno, func), ...]} for concrete asset
    literals (every reference SITE, with its enclosing function), and a separate
    set of dynamic (format-string) literals for eyeballing."""
    refs = defaultdict(list)
    dynamic = set()
    for root in roots:
        if not os.path.isdir(root):
            continue
        for dirpath, _dirs, files in os.walk(root):
            for f in files:
                if not f.lower().endswith(CODE_EXTS):
                    continue
                ap = os.path.join(dirpath, f)
                try:
                    with open(ap, "r", encoding="latin-1") as fh:
                        lines = fh.readlines()
                except OSError:
                    continue
                rel = os.path.relpath(ap)
                func_at = enclosing_functions(lines)
                for lineno, text in enumerate(lines, 1):
                    for m in CODE_ASSET_RE.finditer(text):
                        lit = m.group(1)
                        ref = norm(lit)
                        if "%" in lit:
                            dynamic.add(ref)
                        else:
                            refs[ref].append((rel, lineno, func_at[lineno - 1]))
    return refs, dynamic


# --------------------------------------------------------------------------- #
# HUD sprite config (sprites/*.txt)
# --------------------------------------------------------------------------- #
def scan_hud_sprites(ts_root):
    """Parse sprites/*.txt -> set of referenced sprites/<name>.spr."""
    refs = set()
    sdir = os.path.join(ts_root, "sprites")
    if not os.path.isdir(sdir):
        return refs
    for f in os.listdir(sdir):
        if not f.lower().endswith(".txt"):
            continue
        try:
            with open(os.path.join(sdir, f), "r", encoding="latin-1") as fh:
                lines = fh.readlines()
        except OSError:
            continue
        for ln in lines:
            ln = ln.strip()
            if not ln or ln.startswith("//"):
                continue
            cols = ln.split()
            # format: name res spritefile x y w h  (>=3 cols, col[2] = sprite)
            if len(cols) >= 3 and cols[1].isdigit():
                refs.add(norm("sprites/%s.spr" % cols[2]))
    return refs


# --------------------------------------------------------------------------- #
# resolution helpers
# --------------------------------------------------------------------------- #
def has_file(ref, files):
    """Is `ref` (a gamedir-relative asset path) present in the file index?
    Handles the engine's implicit sound/ prefix for .wav."""
    if ref in files:
        return True
    if ref.endswith(".wav") and not ref.startswith("sound/"):
        if ("sound/" + ref) in files:
            return True
    return False


def in_valve(ref, valve_files):
    if not valve_files:
        return None
    return has_file(ref, valve_files)


def resolve_path(ref, files):
    """Return the actual relpath in `files` that satisfies `ref` (handling the
    implicit sound/ prefix for .wav), or None."""
    if ref in files:
        return ref
    if ref.endswith(".wav") and not ref.startswith("sound/"):
        cand = "sound/" + ref
        if cand in files:
            return cand
    return None


def _safe_size(path):
    try:
        return os.path.getsize(path)
    except OSError:
        return 0


def human_size(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024 or unit == "GB":
            return f"{n:.1f} {unit}" if unit != "B" else f"{n} B"
        n /= 1024


# Top-level valve dirs the ENGINE may load without any map/code naming them —
# don't auto-strip; flag for a human + a test-load. (Conservative; err toward keep.)
ENGINE_ESSENTIAL_HINT = {"gfx", "resource", "sprites"}


# --------------------------------------------------------------------------- #
# main
# --------------------------------------------------------------------------- #
def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ts", required=True, help="mod gamedir to audit (e.g. .../ts)")
    ap.add_argument("--valve", help="valve base gamedir, to confirm deps are real HL assets")
    ap.add_argument("--code", nargs="*", default=[],
                    help="source roots to scan for asset literals (e.g. dlls cl_dll)")
    ap.add_argument("--report", help="write full detailed report to this file")
    ap.add_argument("--strip-plan", action="store_true",
                    help="classify every valve/ file KEEP/STRIP (needs --valve). "
                         "Read-only: prints the plan + optional --strip-list file; "
                         "deletes NOTHING.")
    ap.add_argument("--strip-list",
                    help="with --strip-plan: write the newline-delimited STRIP file "
                         "relpaths here (feed to a reviewed delete step)")
    args = ap.parse_args()

    ts_root = args.ts.rstrip("/")
    if not os.path.isdir(ts_root):
        sys.exit(f"ts gamedir not found: {ts_root}")

    ts_files = index_files(ts_root)
    ts_wads = gamedir_wads(ts_files)
    ts_wad_textures = set()
    for names in ts_wads.values():
        ts_wad_textures |= names
    valve_files = index_files(args.valve) if args.valve else {}
    valve_wads = gamedir_wads(valve_files) if args.valve else {}
    valve_wad_textures = set()
    for names in valve_wads.values():
        valve_wad_textures |= names

    out = []
    def emit(s=""):
        out.append(s)

    emit("=" * 78)
    emit(f"VALVE-DEPENDENCY AUDIT  —  {ts_root}")
    emit("=" * 78)
    emit(f"gamedir files: {len(ts_files)}   wads: {sorted(ts_wads)}   "
         f"wad textures: {len(ts_wad_textures)}")
    if args.valve:
        emit(f"valve base:    {len(valve_files)} files, {len(valve_wad_textures)} wad textures")
    emit()

    # ---- BSP audit --------------------------------------------------------- #
    missing_wads = defaultdict(list)       # wad basename -> [maps]
    missing_tex = defaultdict(list)        # texname -> [maps]
    missing_tool_tex = defaultdict(list)
    missing_ent_refs = defaultdict(list)   # ref -> [maps]
    missing_sky = defaultdict(list)
    bsp_errors = []

    bsps = sorted(rp for rp in ts_files if rp.startswith("maps/") and rp.endswith(".bsp"))
    for rp in bsps:
        info = parse_bsp(ts_files[rp])
        mapname = os.path.basename(rp)
        if info["error"]:
            bsp_errors.append((mapname, info["error"]))
            continue
        for w in info["wads"]:
            if w not in ts_wads:
                missing_wads[w].append(mapname)
        for t in info["ext_tex"]:
            if t in ts_wad_textures:
                continue
            bucket = missing_tool_tex if t in TOOL_TEXTURES else missing_tex
            bucket[t].append(mapname)
        for ref in info["refs"]:
            if not has_file(ref, ts_files):
                missing_ent_refs[ref].append(mapname)
        for sky in info["skies"]:
            # sky images: gfx/env/<sky>{up,dn,lf,rt,ft,bk}.tga
            found = any(("gfx/env/%s%s.tga" % (sky, s)) in ts_files
                        for s in ("up", "dn", "lf", "rt", "ft", "bk"))
            if not found:
                missing_sky[sky].append(mapname)

    emit("-" * 78)
    emit(f"[1] BSP MAPS  ({len(bsps)} maps, {len(bsp_errors)} unparseable)")
    emit("-" * 78)

    def dep_line(key, maps, valve_ok):
        tag = ""
        if valve_ok is True:
            tag = "  [in valve ✓]"
        elif valve_ok is False:
            tag = "  [NOT in valve ✗ — dangling]"
        n = len(set(maps))
        return f"  {key:<34} used by {n:>2} map(s){tag}"

    emit(f"\n  WADs referenced but NOT shipped in the mod ({len(missing_wads)}):")
    if not missing_wads:
        emit("    (none — all referenced wads ship in the gamedir)")
    for w in sorted(missing_wads):
        present = (w in valve_wads) if args.valve else None
        emit(dep_line(w, missing_wads[w], present))

    emit(f"\n  External CONTENT textures not in the mod's wads ({len(missing_tex)}):")
    if not missing_tex:
        emit("    (none)")
    for t in sorted(missing_tex):
        ok = (t in valve_wad_textures) if args.valve else None
        emit(dep_line(t, missing_tex[t], ok))

    emit(f"\n  External TOOL/system textures (invisible; halflife.wad/zhlt) "
         f"({len(missing_tool_tex)}):")
    if missing_tool_tex:
        emit("    " + ", ".join(sorted(missing_tool_tex)))

    emit(f"\n  Entity asset refs (model/sprite/sound) not in the mod ({len(missing_ent_refs)}):")
    if not missing_ent_refs:
        emit("    (none)")
    for r in sorted(missing_ent_refs):
        ok = in_valve(r, valve_files)
        emit(dep_line(r, missing_ent_refs[r], ok))

    emit(f"\n  Sky sets not in the mod ({len(missing_sky)}):")
    if not missing_sky:
        emit("    (none)")
    for s in sorted(missing_sky):
        ok = None
        if args.valve:
            ok = any(("gfx/env/%s%s.tga" % (s, sfx)) in valve_files
                     for sfx in ("up", "dn", "lf", "rt", "ft", "bk"))
        emit(dep_line(s + " (env/*.tga x6)", missing_sky[s], ok))
    if bsp_errors:
        emit("\n  Unparseable maps:")
        for m, e in bsp_errors:
            emit(f"    {m}: {e}")

    # ---- code audit -------------------------------------------------------- #
    # The point of interest is OUR reconstruction, not the original: any asset our
    # code names that we DON'T ship in ts/ is a dead-path candidate to remove
    # (or, rarely, an asset we forgot to ship). Whether the ORIGINAL used it is
    # secondary — the valve tag only classifies WHY it's dead:
    #   [HL-asset] the ref is a stock Half-Life file  -> HL-SDK leftover, remove
    #   [dangling] the ref exists nowhere (not ts, not valve) -> dead code / typo
    emit()
    emit("-" * 78)
    emit(f"[2] CODE ASSET LITERALS  (roots: {args.code or 'none given'})")
    emit("-" * 78)
    code_missing = {}          # ref -> [(file, line), ...]
    dynamic = set()
    if args.code:
        code_refs, dynamic = scan_code(args.code)
        for ref, locs in code_refs.items():
            if not has_file(ref, ts_files):
                code_missing[ref] = locs
        emit(f"\n  Distinct asset literals our code references but ts/ does NOT ship "
             f"({len(code_missing)}):")
        if not code_missing:
            emit("    (none — every concrete literal resolves inside the gamedir)")
        for ref in sorted(code_missing):
            ok = in_valve(ref, valve_files)
            tag = "[HL-asset]" if ok else ("[dangling]" if ok is False else "[?]")
            locs = code_missing[ref]
            where = locs[0][0] + (f" +{len(locs)-1}" if len(locs) > 1 else "")
            emit(f"    {tag:<10} {ref:<40} ({where})")
        if dynamic:
            emit(f"\n  Dynamic/format-string literals (manual review — {len(dynamic)}):")
            for d in sorted(dynamic):
                emit(f"    {d}")
    else:
        emit("  (skipped — pass --code <dir> ... to scan source)")

    # ---- [2b] dead-path removal work-list, grouped by source file ---------- #
    if args.code:
        by_file = defaultdict(list)   # file -> [(line, ref, cls)]
        for ref, locs in code_missing.items():
            ok = in_valve(ref, valve_files)
            cls = "HL-asset" if ok else ("dangling" if ok is False else "?")
            for (fl, ln, _fn) in locs:
                by_file[fl].append((ln, ref, cls))
        n_sites = sum(len(v) for v in by_file.values())
        emit()
        emit("-" * 78)
        emit(f"[2b] DEAD-PATH REMOVAL WORK-LIST  ({n_sites} reference sites in "
             f"{len(by_file)} files)")
        emit("-" * 78)
        emit("  Each site names an asset ts/ does not ship -> remove the code path")
        emit("  (or ship the asset, if it turns out to be genuinely-TS). Grouped by file,")
        emit("  ordered by line, so it can be worked file-by-file.")
        for fl in sorted(by_file):
            rows = sorted(by_file[fl])
            emit(f"\n  {fl}  ({len(rows)} sites)")
            for ln, ref, cls in rows:
                emit(f"    L{ln:<6} [{cls:<8}] {ref}")

    # ---- [2c] CHARACTERIZED: grouped by enclosing function, DEAD vs LIVE ---- #
    # Collapse the loose sites into their enclosing functions and tag each group:
    #   DELETE?  the function is a stock-HL weapon TS doesn't have (name match)
    #   REPLACE? every asset is a shared HL effect TS actually plays (footstep/
    #            gib/debris/ambience) -> swap for generic art, do NOT delete
    #   REVIEW   neither signal fired -> read the function, decide by hand
    # Heuristic triage only: the actual cut still goes through a per-subsystem
    # verify + build (like ev_hldm), so a mis-tag can prioritize wrong but never
    # cause a bad cut.
    if args.code:
        groups = defaultdict(list)    # (file, func) -> [(line, ref), ...]
        for ref, locs in code_missing.items():
            for (fl, ln, fn) in locs:
                groups[(fl, fn or "<file-scope>")].append((ln, ref))

        def classify(func, refs_in_group):
            # DELETE?: the enclosing fn is a stock-HL weapon, OR every asset in the
            # group is an HL-weapon asset (v_egon.mdl, weapons/gauss2.wav, …).
            if func and HL_WEAPON_RE.search(func):
                return "DELETE?"
            if refs_in_group and all(HL_WEAPON_RE.search(r) for r in refs_in_group):
                return "DELETE?"
            if refs_in_group and all(LIVE_SHARED_RE.search(r) for r in refs_in_group):
                return "REPLACE?"
            return "REVIEW"

        tagged = []
        for (fl, fn), rows in groups.items():
            refs_in = [r for _ln, r in rows]
            tagged.append((classify(fn, refs_in), fl, fn, sorted(rows)))
        order = {"DELETE?": 0, "REVIEW": 1, "REPLACE?": 2}
        tagged.sort(key=lambda t: (order[t[0]], t[1], t[2]))

        counts = defaultdict(lambda: [0, 0])   # tag -> [funcs, sites]
        for tag, _fl, _fn, rows in tagged:
            counts[tag][0] += 1
            counts[tag][1] += len(rows)
        emit()
        emit("-" * 78)
        emit(f"[2c] CHARACTERIZED BY ENCLOSING FUNCTION  ({len(tagged)} functions)")
        emit("-" * 78)
        for tag in ("DELETE?", "REVIEW", "REPLACE?"):
            f, s = counts[tag]
            emit(f"    {tag:<9} {f:>3} functions, {s:>3} sites")
        emit("  DELETE? = enclosing fn is a stock-HL weapon TS lacks (gut it, like ev_hldm).")
        emit("  REPLACE? = shared HL effect TS plays (footstep/gib/debris/ambience) -> swap")
        emit("             for generic assets; do NOT delete. REVIEW = read + decide.")
        for tag in ("DELETE?", "REVIEW", "REPLACE?"):
            members = [t for t in tagged if t[0] == tag]
            if not members:
                continue
            emit(f"\n  === {tag} ===")
            for _tag, fl, fn, rows in members:
                assets = sorted({r for _ln, r in rows})
                shown = ", ".join(assets[:4]) + (" …" if len(assets) > 4 else "")
                emit(f"    {fl} :: {fn}  ({len(rows)} sites)")
                emit(f"        {shown}")

    # ---- HUD sprites ------------------------------------------------------- #
    emit()
    emit("-" * 78)
    emit("[3] HUD SPRITE CONFIGS  (sprites/*.txt)  — MANIFEST-LEVEL, verify at runtime")
    emit("-" * 78)
    emit("  NOTE: hud.txt is often the near-stock HL manifest. A listed sprite is only")
    emit("  actually loaded if the client still draws that HUD element — a mod that")
    emit("  reimplements the HUD (e.g. TS draws its own crosshair via ts_reticle, so")
    emit("  'crosshairs.spr'/'cross' here are dead HL leftovers) won't load all of it.")
    emit("  Treat this like the code bucket: references, not proof of runtime load.")
    hud_refs = scan_hud_sprites(ts_root)
    hud_missing = [r for r in sorted(hud_refs) if not has_file(r, ts_files)]
    emit(f"\n  HUD sprite files referenced but missing ({len(hud_missing)}):")
    if not hud_missing:
        emit("    (none)")
    for r in hud_missing:
        ok = in_valve(r, valve_files)
        tag = "  [in valve ✓]" if ok else ("  [NOT in valve ✗]" if ok is False else "")
        emit(f"    {r}{tag}")

    # ---- [4] VALVE STRIP PLAN --------------------------------------------- #
    # Classify every file in the valve base: KEEP (ts references it, or it's a wad
    # / expected engine base) vs STRIP (nothing ts uses names it). The KEEP set is
    # the "replace with generic assets later" surface; the STRIP set shrinks the
    # shipped base and drops unused Valve IP. Read-only — deletes nothing.
    strip_out_list = []
    if args.strip_plan:
        if not args.valve:
            sys.exit("--strip-plan requires --valve")
        keep_reason = {}   # valve relpath -> reason
        def keep(relpath, reason):
            if relpath and relpath not in keep_reason:
                keep_reason[relpath] = reason

        # wads: expected to be present (engine + maps reference them by name)
        for r in valve_files:
            if r.endswith(".wad"):
                keep(r, "wad")
        # map-referenced model/sprite/sound (the assets a shipped ts map pulls)
        for ref in missing_ent_refs:
            keep(resolve_path(ref, valve_files), "map")
        # map skies
        for s in missing_sky:
            for sfx in ("up", "dn", "lf", "rt", "ft", "bk"):
                keep(("gfx/env/%s%s.tga" % (s, sfx)) if
                     ("gfx/env/%s%s.tga" % (s, sfx)) in valve_files else None, "map-sky")
        # code-named assets (sounds/models TS plays from live code — kept per the
        # maps+code rule; dead-path refs fall away naturally once [2b] code is cut)
        for ref in code_missing:
            keep(resolve_path(ref, valve_files), "code")
        for ref in hud_missing:
            keep(resolve_path(ref, valve_files), "hud")

        strip = [r for r in valve_files if r not in keep_reason]
        strip_out_list = sorted(strip)

        def group_by_topdir(paths):
            g = defaultdict(lambda: [0, 0])  # dir -> [count, bytes]
            for r in paths:
                top = r.split("/", 1)[0] if "/" in r else "(root)"
                try:
                    sz = os.path.getsize(valve_files[r])
                except OSError:
                    sz = 0
                g[top][0] += 1
                g[top][1] += sz
            return g

        keep_sz = sum(_safe_size(valve_files[r]) for r in keep_reason)
        strip_sz = sum(_safe_size(valve_files[r]) for r in strip)

        emit()
        emit("-" * 78)
        emit(f"[4] VALVE STRIP PLAN  (read-only; classifies {len(valve_files)} valve files)")
        emit("-" * 78)
        emit(f"  KEEP  {len(keep_reason):>5} files  {human_size(keep_sz):>10}   "
             f"(ts references them, or wad/engine-expected)")
        emit(f"  STRIP {len(strip):>5} files  {human_size(strip_sz):>10}   "
             f"(nothing ts uses names them)")

        # KEEP breakdown by reason — this IS the replace-with-generic surface
        by_reason = defaultdict(list)
        for r, why in keep_reason.items():
            by_reason[why].append(r)
        emit("\n  KEEP by reason (the 'replace-with-generic' work surface):")
        for why in ("wad", "map", "map-sky", "code", "hud"):
            items = sorted(by_reason.get(why, []))
            if not items:
                continue
            emit(f"    [{why}] {len(items)} files:")
            # wads: list all (few). others: list, but cap noise.
            show = items if (why != "code" or len(items) <= 60) else items[:60]
            for r in show:
                emit(f"        {r}")
            if len(show) < len(items):
                emit(f"        … +{len(items) - len(show)} more")

        emit("\n  STRIP candidates by top-level dir "
             "(⚠ = may hold engine-essential base — review + test-load before delete):")
        g = group_by_topdir(strip)
        for top in sorted(g, key=lambda k: -g[k][1]):
            cnt, sz = g[top]
            flag = "  ⚠ review" if top in ENGINE_ESSENTIAL_HINT else ""
            emit(f"    {top:<14} {cnt:>5} files  {human_size(sz):>10}{flag}")
        emit("\n  NOTE: texture-level replacement is finer than file-level — the kept")
        emit("  wads still contain the 33 external HL textures (§[1]); replacing those")
        emit("  means editing the wad, not deleting a file. Sequence: cut dead code")
        emit("  ([2b]) → re-run → strip again → the code-only sounds drop out too.")
        if args.strip_list:
            with open(args.strip_list, "w") as fh:
                fh.write("\n".join(strip_out_list) + "\n")
            emit(f"\n  [STRIP relpaths written to {args.strip_list} — review before any delete]")

    # ---- summary ----------------------------------------------------------- #
    # When a valve base is given, the HONEST count is "confirmed in valve" only.
    # Two big sources of noise otherwise: (a) BSP worldspawn `wad` lists name the
    # mapper's editor wads even though the textures are embedded in the .bsp at
    # compile — those show as dangling; (b) the mod code is derived from the HL
    # SDK and references HL assets on dead code paths (weapons TS doesn't use), so
    # a code literal referencing a valve asset is NOT proof it's loaded at runtime.
    def confirmed(d, valve_present):
        """count of keys in d that valve actually provides (real HL deps)."""
        if not args.valve:
            return None
        return sum(1 for k in d if valve_present(k))

    c_wads = confirmed(missing_wads, lambda w: w in valve_wads)
    c_tex = confirmed(missing_tex, lambda t: t in valve_wad_textures)
    c_ent = confirmed(missing_ent_refs, lambda r: in_valve(r, valve_files))
    c_sky = confirmed(missing_sky, lambda s: any(
        ("gfx/env/%s%s.tga" % (s, sfx)) in valve_files
        for sfx in ("up", "dn", "lf", "rt", "ft", "bk")))
    c_code = confirmed(code_missing, lambda r: in_valve(r, valve_files))
    c_hud = confirmed({r: 1 for r in hud_missing}, lambda r: in_valve(r, valve_files))

    def line(label, raw, conf):
        if conf is None:
            return f"  {label} {raw}"
        return f"  {label} {conf:>3} confirmed  ({raw} raw refs)"

    emit()
    emit("=" * 78)
    emit("SUMMARY" + ("  — 'confirmed' = present in the valve base = real HL asset"
                      if args.valve else ""))
    emit("=" * 78)
    emit("  TIER 1 — engine-FORCED by shipped maps (trust these; the engine loads them")
    emit("           whenever the map loads):")
    emit(line("    content textures (external, in a valve wad) .....", len(missing_tex), c_tex))
    emit(line("    entity model/sprite/sound refs (maps) ...........", len(missing_ent_refs), c_ent))
    emit(line("    sky sets ........................................", len(missing_sky), c_sky))
    if args.valve:
        tier1 = sum(x for x in (c_tex, c_ent, c_sky) if x)
        emit(f"    -> TIER 1 confirmed HL assets .................... {tier1} groups")
    emit()
    emit("  TIER 2 — OUR-CODE references to assets we DON'T ship = DEAD-PATH REMOVAL")
    emit("           targets (see [2b]); not deps to satisfy but code to delete:")
    n_code_sites = sum(len(v) for v in code_missing.values())
    emit(line(f"    code literals ({n_code_sites} sites, see [2b]) ..........", len(code_missing), c_code))
    emit(line("    HUD sprites (sprites/*.txt manifest) ............", len(hud_missing), c_hud))
    emit()
    emit("  INFORMATIONAL (not real deps):")
    emit(line("    WAD editor lists (textures embedded in .bsp) ....", len(missing_wads), c_wads))
    emit(f"    tool/system textures (invisible) ................ {len(missing_tool_tex)}")
    if args.valve:
        emit("\n  Notes:")
        emit("   • 'in valve ✓' = a real Half-Life asset the mod could pull from the base.")
        emit("   • 'NOT in valve ✗' = dangling ref (editor wad list / dead code) — ignore.")
        emit("   • TIER 1 is the honest floor of HL content actually rendered. TIER 2 needs")
        emit("     a runtime/RE check (PRECACHE_* on live paths; which HUD elements the")
        emit("     client still draws) before any of it counts as a real dependency.")

    text = "\n".join(out)
    print(text)
    if args.report:
        with open(args.report, "w") as fh:
            fh.write(text + "\n")
        print(f"\n[written to {args.report}]")


if __name__ == "__main__":
    main()
