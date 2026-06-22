#!/usr/bin/env python3
"""
coverage.py — drop-in INTERFACE coverage: diff our compiled client+server against the
original TS binaries on the EXTERNAL contract (the only thing that must match for a drop-in
swap — see mod-ts/CLAUDE.md "compatibility target"):

  * server **entity factories** (LINK_ENTITY_TO_CLASS C exports — the engine spawns by these)
  * client **cldll exports** (HUD_*/Initialize/ClientFactory — the engine→client ABI spine)
  * **console commands** the client/server accept (server ClientCommand dispatch + client
    pfnAddCommand) — catches commands we INVENTED, ones we MISSED, and HL leftovers we should
    have removed
  * **cvars** (names; defaults where the original default.cfg pins them)
  * a **default.cfg cross-check**: every command the original binds must be handled by our
    client OR server (end-to-end: press the key → something answers)

NOT checked (deliberately — not the drop-in contract): the internal C++ ABI (vtable layout,
member offsets, name mangling). The engine never sees our vtables; only the C interface +
the by-name protocol cross the boundary.

Pure static tools (objdump/nm + the original default.cfg + our source) — no Ghidra, so no
/tmp dependency; fully reproducible.

  python3 scripts/coverage/coverage.py all        # extract both + write the report
  python3 scripts/coverage/coverage.py original    # -> reference/coverage/original.json
  python3 scripts/coverage/coverage.py ours         # -> reference/coverage/ours.json
  python3 scripts/coverage/coverage.py diff          # -> reference/coverage/REPORT.md
"""
import json, os, re, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
REF = os.path.join(ROOT, "reference")
OUT = os.path.join(REF, "coverage")
os.makedirs(OUT, exist_ok=True)

ORIG_SERVER = os.path.join(REF, "ts_i386.so")
ORIG_CLIENT = os.path.join(REF, "client.dll")
ORIG_CFG = os.path.expanduser("~/Containers/hlds/mods/ts/default.cfg")   # binds (input contract)
ORIG_CFG_CVARS = os.path.expanduser("~/Containers/hlds/mods/ts/config.cfg")  # archived cvars+defaults
ORIG_FGD = os.path.join(REF, "ts_fgd.fgd")
OUR_SERVER = os.path.join(ROOT, "build-amd64", "dlls", "hl.so")
OUR_CLIENT = os.path.join(ROOT, "build-amd64", "cl_dll", "client.so")

GOT_BASE = 0x165E10  # _GLOBAL_OFFSET_TABLE_ in ts_i386.so (verified via nm)

# The engine-facing API entry points (NOT entity factories) to exclude from the factory set.
SERVER_API = {
    "GiveFnptrsToDll", "GetEntityAPI", "GetEntityAPI2", "GetNewDLLFunctions",
    "Server_GetPhysicsInterface", "_init", "_fini",
}

def sh(*args):
    return subprocess.run(args, capture_output=True, text=True, errors="replace").stdout

# ---------------------------------------------------------------- ELF (our .so + orig .so)
def elf_text_funcs(path):
    """Undecorated global text symbols (LINK_ENTITY_TO_CLASS C factories live here)."""
    names = set()
    for ln in sh("nm", "-D", "--defined-only", path).splitlines():
        p = ln.split()
        if len(p) >= 3 and p[1] in ("T", "t"):
            names.add(p[2])
    # also static nm for non-exported (our build may not export all)
    for ln in sh("nm", "--defined-only", path).splitlines():
        p = ln.split()
        if len(p) >= 3 and p[1] in ("T", "t"):
            names.add(p[2])
    return names

# gcc/glibc/CRT internals that look like lowercase factories but aren't entities.
TOOLCHAIN_RE = re.compile(
    r"(fdes|gmon|copy_reg|register_frame|deregister|frame_dummy|anglemod|^call_|^count_|"
    r"^done_|^init_|^fini_|^data_start|^register_tm|^do_global|^_?start|^atexit|^cxa_)")

def entity_factories_elf(path):
    out = set()
    for n in elf_text_funcs(path):
        if n.startswith("_Z") or "__" in n or n in SERVER_API:
            continue
        if not re.fullmatch(r"[a-z][a-z0-9_]+", n):  # entity classnames are lowercase_snake
            continue
        if TOOLCHAIN_RE.search(n):
            continue
        # Real HL/TS entity classnames either contain '_' (weapon_*, func_*, ts_*, info_*, …)
        # or are one of a small set of single-word entities. This drops bare C helpers.
        if "_" in n or n in SINGLE_WORD_ENTITIES:
            out.add(n)
    return out

SINGLE_WORD_ENTITIES = {
    "worldspawn", "gib", "grenade", "cycler", "beam", "bodyque", "laser", "spark_shower",
    "hornet", "nihilanth", "controller", "bigmomma", "gargantua", "houndeye", "ichthyosaur",
    "leech", "tentacle", "barnacle", "bullsquid", "headcrab", "zombie", "scientist", "barney",
}

def elf_dynexports(path):
    """Undecorated C text exports (the cldll engine callbacks); drops C++-mangled symbols so
    the set is name-comparable with client.dll's C exports."""
    out = set()
    for ln in sh("nm", "-D", "--defined-only", path).splitlines():
        p = ln.split()
        if len(p) >= 3 and p[1] in ("T", "t") and not p[2].startswith("_Z") and re.fullmatch(r"[A-Za-z_]\w*", p[2]):
            out.add(p[2])
    return out

# ---------------------------------------------------------------- PE (orig client.dll)
def pe_exports(path):
    """The C cldll engine-callback exports of client.dll (Initialize/HUD_*/ClientFactory/…).
    Drops the MSVC-mangled VGUI/C++ exports (?...@...@@) — those aren't the C engine contract
    and can't be name-compared against ELF mangling."""
    out, intable = set(), False
    for ln in sh("objdump", "-p", path).splitlines():
        if "[Ordinal/Name Pointer] Table" in ln:
            intable = True; continue
        if intable:
            m = re.match(r"\s*\[\s*\d+\]\s+(\S+)", ln)
            if m:
                n = m.group(1)
                if "?" not in n and "@" not in n and re.fullmatch(r"[A-Za-z_]\w*", n):
                    out.add(n)
            elif ln.strip() == "": intable = False
    return out

def pe_read_cstr(data, vaddr):
    off = vaddr - 0x10000000   # client.dll: file_off == vaddr - imagebase (flat mapping)
    if off < 0 or off >= len(data): return None
    end = data.find(b"\x00", off)
    s = data[off:end]
    try: return s.decode("ascii")
    except UnicodeDecodeError: return None

def client_usermsgs_orig(path):
    """User-message names the original CLIENT hooks via pfnHookUserMsg (ds:0x100d5b70) —
    the consumer side of the client<->server protocol (`push handler; push name; call`)."""
    data = open(path, "rb").read()
    dis = sh("objdump", "-d", "-M", "intel", path).splitlines()
    msgs, last = set(), None
    for ln in dis:
        mp = re.search(r"push\s+0x([0-9a-f]+)", ln)
        if mp:
            s = pe_read_cstr(data, int(mp.group(1), 16))
            last = s if (s and re.fullmatch(r"[A-Za-z_][\w]{1,30}", s)) else None
            continue
        if "call" in ln and "ds:0x100d5b70" in ln:   # pfnHookUserMsg
            if last: msgs.add(last)
            last = None
    return msgs

def events_in(path):
    """events/*.sc script names referenced by a binary (PRECACHE_EVENT/event playback)."""
    out = set()
    for ln in sh("strings", path).splitlines():
        out.update(re.findall(r"events/[A-Za-z0-9_]+\.sc", ln))
    return out

def our_client_usermsgs():
    return grep_set(r'HOOK_MESSAGE\(\s*\w+\s*,\s*(\w+)\s*\)', "cl_dll", group=1) | \
           grep_set(r'HOOK_MESSAGE\(\s*(\w+)\s*\)', "cl_dll", group=1)

def our_server_usermsgs():
    return grep_set(r'REG_USER_MSG\(\s*"([^"]+)"', "dlls")

def elf_sections(path):
    secs = []
    for l in sh("objdump", "-h", path).splitlines():
        p = l.split()
        if len(p) >= 7 and p[0].isdigit():
            try: secs.append((p[1], int(p[3], 16), int(p[2], 16), int(p[5], 16)))  # name,vma,size,off
            except ValueError: pass
    return secs

def server_cvars_orig(path):
    """Server cvars REGISTERED BY THE BINARY: scan .data for cvar_t {char* name; char* string
    (default); int flags; float value; cvar_t* next}. The name/default pointers are filled by
    R_386_RELATIVE relocs whose addend (the .rodata string vaddr) is stored in the file, so we
    read them directly. Returns {name: default}. (Closes the server-cvar coverage gap.)"""
    import struct
    data = open(path, "rb").read()
    secs = elf_sections(path)
    def cstr(va):
        for nm, vma, size, off in secs:
            if nm in (".rodata", ".data") and vma <= va < vma + size:
                e = data.find(b"\x00", off + (va - vma))
                try: return data[off + (va - vma):e].decode("ascii")
                except UnicodeDecodeError: return None
        return None
    dsec = [s for s in secs if s[0] == ".data"]
    if not dsec: return {}
    _, dvma, dsize, doff = dsec[0]
    cv = {}
    for o in range(0, dsize - 20, 4):
        p_name, p_str = struct.unpack_from("<II", data, doff + o)
        if not p_name or not p_str: continue
        name, dflt = cstr(p_name), cstr(p_str)
        if not name or dflt is None: continue
        if not re.fullmatch(r"[a-z_][a-z0-9_]{2,30}", name): continue
        # cvar defaults are short values; drop the weapon/ammo classname-pair false positives.
        if len(dflt) > 12 or re.match(r"(weapon_|ammo_|item_|func_|info_)", dflt): continue
        if re.match(r"(weapon_|ammo_)", name): continue
        cv[name] = dflt
    return cv

def our_server_cvars():
    """Our server cvars: cvar_t definitions in dlls/ ( cvar_t X = {"name", "default", ... } )."""
    cv = {}
    for name, dflt in re_pairs(r'cvar_t\s+\w+\s*=\s*\{\s*"([^"]+)"\s*,\s*"([^"]*)"', "dlls"):
        cv[name] = dflt
    return cv

def client_cvars_orig(path):
    """Cvars the original CLIENT registers: pfnRegisterVariable(name, default, flags) at
    ds:0x100d5b60 — `push flags; push default; push name; call`. Returns {name: default}.
    This is the AUTHORITATIVE original client-cvar set (config.cfg only holds the archived
    subset that was written out, so it MISSES stock cvars like zoom_sensitivity_ratio)."""
    data = open(path, "rb").read()
    dis = sh("objdump", "-d", "-M", "intel", path).splitlines()
    cv, str_pushes = {}, []
    for ln in dis:
        mpush = re.search(r"push\s+0x([0-9a-f]+)", ln)
        if mpush:
            s = pe_read_cstr(data, int(mpush.group(1), 16))
            str_pushes.append(s)               # may be None for non-string pushes
            if len(str_pushes) > 3: str_pushes.pop(0)
            continue
        if "call" in ln and "ds:0x100d5b60" in ln:   # pfnRegisterVariable
            # name = last string push, default = the one before it
            strs = [s for s in str_pushes if s is not None]
            if strs:
                name = strs[-1]
                dflt = strs[-2] if len(strs) >= 2 else None
                if re.fullmatch(r"[a-zA-Z_][\w]{1,40}", name):
                    cv[name] = dflt
            str_pushes = []
        elif "call" in ln:
            str_pushes = []
    return cv

def client_commands_orig(path):
    """Every pfnAddCommand(name, handler) the original client registers (ds:0x100d5b6c)."""
    data = open(path, "rb").read()
    dis = sh("objdump", "-d", "-M", "intel", path).splitlines()
    cmds, last_push_str = set(), None
    for ln in dis:
        mpush = re.search(r"push\s+0x([0-9a-f]+)", ln)
        if mpush:
            va = int(mpush.group(1), 16)
            s = pe_read_cstr(data, va)
            if s and re.fullmatch(r"[A-Za-z_+][\w+. %]{0,30}", s):
                last_push_str = s
            else:
                last_push_str = None
            continue
        if "call" in ln and "ds:0x100d5b6c" in ln:   # pfnAddCommand
            if last_push_str:
                cmds.add(last_push_str.split()[0] if " " in last_push_str else last_push_str)
            last_push_str = None
    return cmds

# ---------------------------------------------------------------- orig server commands
def fn_range(path, mangled_substr):
    """(start,next) addr of the function whose symbol contains mangled_substr."""
    syms = []
    for ln in sh("nm", path).splitlines():
        p = ln.split()
        if len(p) >= 3 and p[1] in ("T", "t"):
            try: syms.append((int(p[0], 16), p[2]))
            except ValueError: pass
    syms.sort()
    for i, (a, n) in enumerate(syms):
        if mangled_substr in n:
            nxt = syms[i + 1][0] if i + 1 < len(syms) else a + 0x2000
            return a, nxt
    return None, None

def server_client_commands_orig(path):
    """The strcmp(argv0, "...") dispatch in ClientCommand — the commands the original server
    accepts from a client. Resolves the PIC `lea -off(%ebx)` string operands via the GOT."""
    start, end = fn_range(path, "ClientCommand__FP7edict_s")
    if start is None: return set()
    data = open(path, "rb").read()
    dis = sh("objdump", "-d", f"--start-address=0x{start:x}", f"--stop-address=0x{end:x}", path).splitlines()
    cmds, last_lea = set(), None
    for ln in dis:
        m = re.search(r"lea\s+-0x([0-9a-f]+)\(%ebx\)", ln)
        if not m:
            m = re.search(r"lea\s+0x([0-9a-f]+)\(%ebx\)", ln)  # rare +off
            off = -int(m.group(1), 16) if m else None
        else:
            off = int(m.group(1), 16)
        if off is not None:
            va = GOT_BASE - off
            end_s = data.find(b"\x00", va)
            try: last_lea = data[va:end_s].decode("ascii")
            except (UnicodeDecodeError, ValueError): last_lea = None
            continue
        if "strcmp@plt" in ln and last_lea:
            if re.fullmatch(r"[A-Za-z_+][\w+]{0,30}", last_lea):
                cmds.add(last_lea)
            last_lea = None
    return cmds

# ---------------------------------------------------------------- ours (from source)
def grep_set(pattern, *globs, group=1):
    out = set()
    for g in globs:
        d = os.path.join(ROOT, g)
        if not os.path.isdir(d): continue
        for root, _, files in os.walk(d):
            for f in files:
                if not f.endswith((".cpp", ".h")): continue
                try: txt = open(os.path.join(root, f), errors="replace").read()
                except OSError: continue
                for m in re.finditer(pattern, txt):
                    out.add(m.group(group))
    return out

def our_server_commands():
    cmds = grep_set(r'FStrEq\(\s*pcmd\s*,\s*"([^"]+)"', "dlls")
    if grep_set(r'strncmp\(\s*pcmd\s*,\s*"weapon_"', "dlls", group=0):
        cmds.add("weapon_<N>")
    return cmds

def our_client_commands():
    return (grep_set(r'HOOK_COMMAND\(\s*"([^"]+)"', "cl_dll") |
            grep_set(r'pfnAddCommand\(\s*"([^"]+)"', "cl_dll"))

def drop_release_halves(cmds):
    """The engine auto-registers a '-x' for every '+x' command, so '-x' is never separately
    registered — drop the '-' halves to avoid false 'extra' noise when comparing."""
    return {c for c in cmds if not c.startswith("-")}

def our_link_entities():
    """Our entity classnames = the first arg of LINK_ENTITY_TO_CLASS(classname, CppClass)."""
    return grep_set(r'LINK_ENTITY_TO_CLASS\(\s*(\w+)\s*,', "dlls", group=1)

def fgd_entities(path):
    """The original's authoritative entity classnames (mapper-facing) from the TS FGD."""
    out = set()
    if not os.path.exists(path): return out
    for ln in open(path, errors="replace"):
        m = re.search(r'@(?:Point|Solid|Keyframe|Move)Class\b[^=]*=\s*([A-Za-z0-9_]+)', ln)
        if m: out.add(m.group(1))
    return out

def cfg_cvars_with_defaults(path):
    """Archived cvars + their default values from the original config.cfg ( name "value" )."""
    cv = {}
    if not os.path.exists(path): return cv
    for ln in open(path, errors="replace"):
        m = re.match(r'\s*([a-zA-Z_][\w]*)\s+"([^"]*)"\s*$', ln)
        if m and m.group(1) not in ("bind", "unbindall", "exec", "echo"):
            cv[m.group(1)] = m.group(2)
    return cv

def our_cvars_with_defaults():
    """Our cvar names -> default (2nd arg of CVAR_CREATE; CVAR_REGISTER cvar_t defaults are
    harder so those names carry None)."""
    cv = {}
    for name, dflt in re_pairs(r'CVAR_CREATE\(\s*"([^"]+)"\s*,\s*"([^"]*)"', "cl_dll", "dlls"):
        cv[name] = dflt
    for name in our_cvars():
        cv.setdefault(name, None)
    return cv

def re_pairs(pattern, *globs):
    out = []
    for g in globs:
        d = os.path.join(ROOT, g)
        if not os.path.isdir(d): continue
        for root, _, files in os.walk(d):
            for f in files:
                if not f.endswith((".cpp", ".h")): continue
                try: txt = open(os.path.join(root, f), errors="replace").read()
                except OSError: continue
                for m in re.finditer(pattern, txt):
                    out.append((m.group(1), m.group(2)))
    return out

def our_cvars():
    return (grep_set(r'CVAR_CREATE\(\s*"([^"]+)"', "cl_dll", "dlls") |
            grep_set(r'CVAR_REGISTER\(\s*&?\w*,?\s*"([^"]+)"', "cl_dll", "dlls") |
            grep_set(r'pfnRegisterVariable\(\s*"([^"]+)"', "cl_dll", "dlls"))

# ---------------------------------------------------------------- default.cfg ground truth
def cfg_binds(path):
    cmds = set()
    if not os.path.exists(path): return cmds
    for ln in open(path, errors="replace"):
        m = re.match(r'\s*bind\s+"[^"]*"\s+"([^"]+)"', ln)
        if m:
            cmd = m.group(1).strip().split()[0]  # "save quick"->save; keep +/- (engine builtins)
            cmds.add(cmd)
    return cmds

def cfg_cvars(path):
    cv = {}
    if not os.path.exists(path): return cv
    for ln in open(path, errors="replace"):
        m = re.match(r'\s*([a-zA-Z_][\w]*)\s+"([^"]*)"\s*$', ln)
        if m and m.group(1) not in ("bind", "unbindall"):
            cv[m.group(1)] = m.group(2)
    return cv

# ---------------------------------------------------------------- manifests
def extract_original():
    man = {
        "server_bin": ORIG_SERVER, "client_bin": ORIG_CLIENT,
        "entities": sorted(fgd_entities(ORIG_FGD)),  # FGD = authoritative mapper-facing set
        "client_exports": sorted(pe_exports(ORIG_CLIENT)) if os.path.exists(ORIG_CLIENT) else [],
        "server_commands": sorted(server_client_commands_orig(ORIG_SERVER)) if os.path.exists(ORIG_SERVER) else [],
        "client_commands": sorted(drop_release_halves(client_commands_orig(ORIG_CLIENT))) if os.path.exists(ORIG_CLIENT) else [],
        "client_cvars": client_cvars_orig(ORIG_CLIENT) if os.path.exists(ORIG_CLIENT) else {},  # binary-registered (authoritative)
        "server_cvars": server_cvars_orig(ORIG_SERVER) if os.path.exists(ORIG_SERVER) else {},  # cvar_t scan (authoritative)
        "usermsgs": sorted(client_usermsgs_orig(ORIG_CLIENT)) if os.path.exists(ORIG_CLIENT) else [],  # client-hooked names
        "events": sorted(events_in(ORIG_SERVER)) if os.path.exists(ORIG_SERVER) else [],
        "cfg_binds": sorted(cfg_binds(ORIG_CFG)),
        "cfg_cvars_archived": cfg_cvars_with_defaults(ORIG_CFG_CVARS),  # archived subset only (NOT a full set)
    }
    json.dump(man, open(os.path.join(OUT, "original.json"), "w"), indent=1, sort_keys=True)
    return man

def extract_ours():
    man = {
        "server_bin": OUR_SERVER, "client_bin": OUR_CLIENT,
        "entities": sorted(our_link_entities()),  # LINK_ENTITY_TO_CLASS classnames
        "client_exports": sorted(elf_dynexports(OUR_CLIENT)) if os.path.exists(OUR_CLIENT) else [],
        "server_commands": sorted(our_server_commands()),
        "client_commands": sorted(drop_release_halves(our_client_commands())),
        "cvars": our_cvars_with_defaults(),
        "server_cvars": our_server_cvars(),
        "usermsgs": sorted(our_client_usermsgs()),
        "server_usermsgs": sorted(our_server_usermsgs()),
        "events": sorted(events_in(OUR_SERVER)) if os.path.exists(OUR_SERVER) else [],
    }
    json.dump(man, open(os.path.join(OUT, "ours.json"), "w"), indent=1, sort_keys=True)
    return man

# ---------------------------------------------------------------- diff + report
def _fmt(items):
    """Render a name list, collapsing big same-prefix families (e.g. the sk_* HL skill cvars)
    so the report stays readable: 'sk_* (211 — HL skill leftovers)' + the rest inline."""
    items = list(items)
    groups = {}
    for it in items:
        m = re.match(r"([a-z]+_[a-z]+_)", it)  # two-segment prefix, e.g. sk_zombie_
        key = re.match(r"(sk_)", it)
        if key:
            groups.setdefault("sk_", []).append(it)
    rest = [it for it in items if not it.startswith("sk_")]
    out = []
    if "sk_" in groups:
        out.append(f"`sk_*` ({len(groups['sk_'])} — stock-HL skill cvars, bulk leftover)")
    out += [f"`{x}`" for x in rest]
    return ", ".join(out)

def section(title, orig, ours, note="", ignore_extra=None):
    o, u = set(orig), set(ours)
    ignore_extra = ignore_extra or set()
    missing = sorted(o - u)   # in original, not in ours -> need to implement / RE
    extra = sorted((u - o) - ignore_extra)  # in ours, not in original -> invented / HL leftover
    both = sorted(o & u)
    lines = [f"### {title}", ""]
    if note: lines += [f"_{note}_", ""]
    lines.append(f"- matched: **{len(both)}**  |  missing (orig→not ours): **{len(missing)}**  |  extra (ours→not orig): **{len(extra)}**")
    if missing: lines += ["", "**MISSING (in original, not in ours — implement / RE):**", "", "  " + _fmt(missing)]
    if extra:   lines += ["", "**EXTRA (in ours, not in original — invented or HL-leftover, candidate to remove):**", "", "  " + _fmt(extra)]
    lines.append("")
    return lines, missing, extra

def diff():
    o = json.load(open(os.path.join(OUT, "original.json")))
    u = json.load(open(os.path.join(OUT, "ours.json")))
    L = ["# Drop-in interface coverage — original TS vs ours", "",
         "Generated by `scripts/coverage/coverage.py`. Checks the EXTERNAL swap contract only",
         "(entity factories, cldll exports, console commands, cvars). Internal C++ ABI is out of",
         "scope (not a drop-in requirement — see CLAUDE.md).", ""]

    s, _, _ = section("Entity classes (original FGD vs our LINK_ENTITY_TO_CLASS)", o["entities"], u["entities"],
                      "Original = the TS FGD's mapper-facing entity set; ours = LINK_ENTITY_TO_CLASS classnames. "
                      "MISSING = an FGD entity we don't implement (the reconstruction work-list). EXTRA = we link "
                      "an entity the FGD omits (stock HL helpers / non-mapper entities — low signal).")
    L += s
    s, _, _ = section("Client cldll exports", o["client_exports"], u["client_exports"],
                      "Engine→client entry points. NB: orig client.dll exports may include MSVC/VGUI decoration; cross-check by base name.")
    L += s
    s, miss_sc, extra_sc = section("Server console commands (ClientCommand dispatch)", o["server_commands"], u["server_commands"],
                      "Commands the server accepts from a client (exact strcmp dispatch). EXTRA = the prime 'invented/HL-leftover' signal. "
                      "NB the original ALSO accepts the numeric `weapon_<N>` select + the `weapon_%d` buy args via non-strcmp paths "
                      "(strstr/sscanf), not captured here — so `weapon_<N>` is not flagged.",
                      ignore_extra={"weapon_<N>"})
    L += s
    s, _, _ = section("Client console commands (pfnAddCommand)", o["client_commands"], u["client_commands"],
                      "Commands the client registers. Many are stock HL (shared); focus on TS-specific gaps.")
    L += s

    s, _, _ = section("User messages (client-hooked names — the protocol)", o.get("usermsgs", []), u.get("usermsgs", []),
                      "Original = names hooked in client.dll (pfnHookUserMsg); ours = HOOK_MESSAGE in cl_dll. "
                      "These are the client↔server message NAMES (the wire contract). MISSING = a message the original "
                      "client reads that ours doesn't; EXTRA = ours-only (invented or inherited-HL).")
    L += s
    s, _, _ = section("Events (events/*.sc)", o.get("events", []), u.get("events", []),
                      "Predicted-event scripts referenced by each server. MISSING = a TS event we don't reference; "
                      "EXTRA = a stock-HL event we still precache that the original removed (HL leftover).")
    L += s

    # default.cfg cross-check: every bound command must be answered by ours (client or server) or be an engine builtin.
    bound = set(o["cfg_binds"])
    ours_all = set(u["server_commands"]) | set(u["client_commands"]) | {"weapon"}
    ENGINE_BUILTINS = {"+attack","+attack2","+jump","+duck","+forward","+back","+moveleft","+moveright",
        "+moveup","+movedown","+left","+right","+lookup","+lookdown","+klook","+speed","+use","+reload",
        "+showscores","+voicerecord","slot1","slot2","slot3","slot4","slot5","slot6","slot7","slot8","slot9",
        "slot10","cancelselect","invnext","invprev","messagemode","messagemode2","sizeup","sizedown","snapshot",
        "save","load","quit","pause","centerview","toggleconsole","thirdperson","impulse","unbindall","bind",
        "+alt1","+alt2"}
    unanswered = sorted(b for b in bound if b not in ours_all and b not in ENGINE_BUILTINS)
    L += ["### default.cfg input cross-check", "",
          "_Every command the original binds should be answered by our client/server (or be an engine builtin)._", "",
          f"- bound commands: **{len(bound)}**  |  unanswered by ours (non-builtin): **{len(unanswered)}**"]
    if unanswered:
        L += ["", "**UNANSWERED (original binds it, ours has no handler):**", "", "  " + ", ".join(f"`{c}`" for c in unanswered)]
    L.append("")

    # cvars: original CLIENT cvars REGISTERED BY THE BINARY (authoritative) vs ours.
    # (config.cfg is only the archived subset — it misses stock cvars like zoom_sensitivity_ratio,
    #  so it must NOT be used as the reference set.)
    ocv = o.get("client_cvars", {}); ucvd = u.get("cvars", {})
    ocv_names, ucv_names = set(ocv), set(ucvd)
    s, _, _ = section("Client cvars (original binary-registered vs ours)", ocv_names, ucv_names,
                      "Source = pfnRegisterVariable sites in client.dll (authoritative — NOT config.cfg). "
                      "MISSING = a client cvar the original registers that we don't. EXTRA = ours-only "
                      "(invented, or a SERVER cvar — server-side registrations aren't in this client list yet, TODO).")
    L += s
    # default-value mismatches on the cvars present in both
    mism = []
    for name in sorted(ocv_names & ucv_names):
        od, ud = ocv.get(name), ucvd.get(name)
        if od is not None and ud is not None and od != ud:
            mism.append(f"`{name}`: orig=`{od}` ours=`{ud}`")
    if mism:
        L += ["", "**DEFAULT MISMATCHES (cvar in both, different default):**", "", "  " + " · ".join(mism), ""]

    # server cvars (cvar_t scan, authoritative) vs ours
    oscv = o.get("server_cvars", {}); uscv = u.get("server_cvars", {})
    s, _, _ = section("Server cvars (original cvar_t scan vs ours)", set(oscv), set(uscv),
                      "Source = cvar_t structs in ts_i386.so .data (authoritative). MISSING = a gameplay/mp_ "
                      "cvar the original registers that we don't; EXTRA = ours-only (invented or HL-leftover).")
    L += s
    smism = [f"`{n}`: orig=`{oscv[n]}` ours=`{uscv[n]}`" for n in sorted(set(oscv) & set(uscv))
             if oscv[n] != uscv[n]]
    if smism:
        L += ["", "**SERVER-CVAR DEFAULT MISMATCHES:**", "", "  " + " · ".join(smism), ""]

    L += ["---", "## Coverage status / remaining gaps",
          "DONE (the by-name swap contract): entity classes, client cldll exports, server+client console commands,",
          "client cvars (binary `pfnRegisterVariable`) + **server cvars** (binary `cvar_t` scan) both with default",
          "mismatch, user messages (client-hooked), events, default.cfg input cross-check. Plus vtable layouts for all",
          "226 classes (`extract_vtables.py` → VTABLES.md).",
          "",
          "NOT yet automated (extend this harness):",
          "- **user-message wire SIZES** — the `pfnRegUserMsg(name,size)` call sites (name via GOT ptr, size imm); ",
          "  the NAMES are covered, sizes are the refinement.",
          "- **cvar call-site tracing** (where each cvar is READ) — xref pfnCVarGetFloat/Pointer to compare usage.",
          "- **engine-call usage** (which g_engfuncs we call vs original) — by construction we use the SDK's fixed",
          "  engine_t, so we can't call a nonexistent one; a usage diff is informational only.",
          "OUT OF SCOPE (not a drop-in requirement — CLAUDE.md): internal C++ ABI (vtable layout, member offsets,",
          "name mangling), per-class method inventories. The engine only sees the C interface + the by-name protocol above.", ""]

    rep = os.path.join(OUT, "REPORT.md")
    open(rep, "w").write("\n".join(L))
    print(f"wrote {rep}")
    print(f"server commands: {len(miss_sc)} missing, {len(extra_sc)} extra; {len(unanswered)} unanswered binds")

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "all"
    if cmd in ("original", "all"): extract_original(); print("wrote original.json")
    if cmd in ("ours", "all"): extract_ours(); print("wrote ours.json")
    if cmd in ("diff", "all"): diff()
