#!/usr/bin/env python3
"""
hamdata.py — Ham Sandwich gamedata (virtual.games) for reTS, from reTS's own vtables.

Ham hooks a virtual by slot index. Stock TS's slots are in AMX Mod X's
gamedata/common.games/virtual.games/ts/offsets-common.txt (gcc2 layout: vptr at +0x60, two header
words before the first virtual). reTS is built by a modern compiler (vptr at +0, Itanium layout) and
carries the halflife-updated extra virtuals, so its slots differ. This tool:

  1. reads each stock key's linux slot and names the original virtual at that slot from
     reference/coverage/vtables_original.json (player_* -> CBasePlayer, item_* -> CBasePlayerItem,
     weapon_* -> CBasePlayerWeapon, the rest -> CBaseEntity, or CBaseMonster past its end);
  2. finds the same virtual (name, then arguments for overloads) in reTS's _ZTV vtable of that class;
  3. writes the reTS gamedata file with that slot (index from the address the vptr points at).

Every given hl.so must yield the same slots (Itanium slot numbering does not depend on the arch);
the tool fails otherwise, and fails if a stock key has no reTS counterpart.

  python3 scripts/coverage/hamdata.py --so i386=build-i386/dlls/hl.so --so amd64=build-amd64/dlls/hl.so \\
      [--stock <amxmodx>/gamedata/common.games/virtual.games/ts/offsets-common.txt] [-o out.txt]
"""
import argparse, json, os, re, struct, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
COV = os.path.join(ROOT, "reference", "coverage")
STOCK = os.path.join(ROOT, "..", "hlds64-buildchain", "amxmodx", "gamedata", "common.games",
                     "virtual.games", "ts", "offsets-common.txt")


def sh(*a):
    return subprocess.run(a, capture_output=True, text=True, errors="replace").stdout


# ---------------------------------------------------------------- stock gamedata
def stock_keys(path):
    """[(key, windows, linux)] in file order."""
    text = open(path).read()
    return [(k, w, l) for k, w, l in
            re.findall(r'"(\w+)"\s*\{\s*"windows"\s*"(\w+)"\s*"linux"\s*"(\w+)"\s*\}', text)]


def key_class(key, slot, orig):
    if key.startswith("player_"):
        return "CBasePlayer"
    if key.startswith("item_"):
        return "CBasePlayerItem"
    if key.startswith("weapon_") or key.startswith("ts_weapon_"):
        return "CBasePlayerWeapon"
    return "CBaseEntity" if "%#x" % (slot * 4) in orig["CBaseEntity"] else "CBaseMonster"


# ---------------------------------------------------------------- reTS vtables
class Elf:
    def __init__(self, path):
        hdr = open(path, "rb").read(5)
        self.ptr = 8 if hdr[4] == 2 else 4
        self.syms, self.funcs = {}, {}
        for l in sh("readelf", "-sW", path).splitlines():
            p = l.split()
            if len(p) >= 8 and re.match(r"^[0-9a-f]+:$", p[0]) and re.match(r"^[0-9a-f]{8,16}$", p[1]):
                a, sz, typ, name = int(p[1], 16), int(p[2], 0), p[3], p[7]
                self.syms.setdefault(name, (a, sz))
                if typ == "FUNC" and a:
                    self.funcs.setdefault(a, name)
        self.rel = {}
        for l in sh("readelf", "-rW", path).splitlines():
            p = l.split()
            if len(p) < 3 or not re.match(r"^[0-9a-f]{8,16}$", p[0]):
                continue
            off, typ = int(p[0], 16), p[2]
            if typ.endswith("RELATIVE"):
                # REL (i386) keeps the target in the data word; RELA (64-bit) puts it in the addend
                self.rel[off] = ("addr", int(p[3], 16)) if len(p) > 3 else ("data", None)
            elif len(p) >= 5:
                self.rel[off] = ("sym", p[4].split("@")[0])
        self.secs = []
        for l in sh("readelf", "-SW", path).splitlines():
            m = re.match(r"\s*\[\s*\d+\]\s+\S+\s+\S+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)", l)
            if m:
                self.secs.append(tuple(int(x, 16) for x in m.groups()))
        self.data = open(path, "rb").read()

    def word(self, a):
        for va, off, sz in self.secs:
            if va and va <= a < va + sz:
                return struct.unpack_from("<Q" if self.ptr == 8 else "<I", self.data, off + a - va)[0]
        return None

    def entry(self, a):
        r = self.rel.get(a)
        if r and r[0] == "sym":
            return r[1]
        v = r[1] if r and r[0] == "addr" else self.word(a)
        return self.funcs.get(v) if v else None


def demangle(names):
    out = sh("c++filt", *names).splitlines() if names else []
    return dict(zip(names, out))


def rets_vtables(path, classes):
    """{class: [(name, args), ...]} slot order, primary vtable only, from the vptr target on."""
    e = Elf(path)
    raw = {}
    for c in classes:
        sym = "_ZTV%d%s" % (len(c), c)
        if sym not in e.syms:
            sys.exit(f"{path}: no {sym}")
        a, sz = e.syms[sym]
        ents = [e.entry(a + e.ptr * i) for i in range(2, sz // e.ptr)]
        raw[c] = ents
    dm = demangle(sorted({x for v in raw.values() for x in v if x}))
    out = {}
    for c, ents in raw.items():
        slots = []
        for x in ents:
            if x is None or x.startswith("_ZTI"):
                break  # end of the primary vtable
            if x == "__cxa_pure_virtual":
                slots.append(("<pure>", ""))
                continue
            d = dm.get(x, x)
            m = re.match(r"^(.*?)::(~?\w+)\((.*)\)( const)?$", d)
            slots.append((m.group(2), norm(m.group(3))) if m else (d, ""))
        out[c] = slots
    return out


def norm(args):
    a = re.sub(r"\b(struct|class|const)\b", "", args)
    a = a.replace("entvars_t", "entvars_s").replace("edict_t", "edict_s").replace("KeyValueData", "KeyValueData_s")
    a = a.replace("KeyValueData_s_s", "KeyValueData_s")
    return re.sub(r"\s+", "", a)


def orig_method(sig):
    m = re.match(r"^(.*?)::(~?\w+)\((.*)\)( const)?$", sig)
    return (m.group(2), norm(m.group(3))) if m else (sig, "")


# halflife-updated shapes reTS keeps on purpose (CLAUDE.md): the original virtual is not virtual in
# reTS and the slot holds the virtual it hands off to.
ALIASES = {
    # RetireWeapon() only schedules a think; the virtual is DoRetireWeapon(), which does the retiring
    "RetireWeapon": "DoRetireWeapon",
}

# Stock keys whose stock slot is not the virtual the key names. AMX Mod X's ts file gives
# ts_onfreeentprivatedata / ts_shouldcollide linux 12 / 13 (windows 10 / 11), which are Save / Restore in
# ts_i386.so; the original has OnFreeEntPrivateData and ShouldCollide at 16 / 17. Map them by name.
STOCK_FIXES = {
    "ts_onfreeentprivatedata": "CBaseEntity::OnFreeEntPrivateData()",
    "ts_shouldcollide": "CBaseEntity::ShouldCollide(CBaseEntity *)",
}

# Ham key names that are not the method name lowercased (after the player_/item_/weapon_/ts_ prefix).
NAME_DIFFS = {
    "fvecvisible": "FVisible", "fvecinviewcone": "FInViewCone", "item_preframe": "ItemPreFrame",
    "item_postframe": "ItemPostFrame", "item_itemslot": "iItemSlot", "weapon_isusable": "IsUseable",
    "ts_canusedthroughwalls": "CanUsedTroughWalls",
}


# ---------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--so", action="append", required=True, help="arch=path to a reTS hl.so")
    ap.add_argument("--stock", default=STOCK, help="stock TS virtual.games offsets-common.txt")
    ap.add_argument("-o", "--out", help="write the gamedata file here (default: stdout)")
    a = ap.parse_args()

    orig = json.load(open(os.path.join(COV, "vtables_original.json")))
    keys = stock_keys(a.stock)
    classes = ["CBaseEntity", "CBaseMonster", "CBasePlayer", "CBasePlayerItem", "CBasePlayerWeapon"]
    sos = dict(x.split("=", 1) for x in a.so)
    tables = {arch: rets_vtables(p, classes) for arch, p in sos.items()}

    rows, errors = [], []
    for key, win, lin in keys:
        if key in ("pev", "base"):
            continue
        slot = int(lin, 0)
        cls = key_class(key, slot, orig)
        sig = STOCK_FIXES.get(key) or orig[cls].get("%#x" % (slot * 4))
        if not sig or sig.startswith("("):
            errors.append(f"{key}: original {cls} has no virtual at linux slot {slot}")
            continue
        name, args = orig_method(sig)
        bare = re.sub(r"^(ts_weapon_|ts_|player_|item_|weapon_)", "", key)
        if NAME_DIFFS.get(key, bare).lower() != name.lower():
            errors.append(f"{key}: stock slot {slot} is {sig}, not the virtual the key names")
            continue
        name = ALIASES.get(name, name)
        found = {}
        for arch, t in tables.items():
            cands = [i for i, s in enumerate(t[cls]) if s[0] == name]
            if len(cands) > 1:
                cands = [i for i in cands if t[cls][i][1] == args]
            if len(cands) != 1:
                errors.append(f"{key}: {sig} -> {len(cands)} matches in reTS {cls} ({arch})")
                continue
            found[arch] = cands[0]
        if len(set(found.values())) > 1:
            errors.append(f"{key}: slot differs by arch {found}")
            continue
        if found:
            rows.append((key, cls, sig, slot, next(iter(found.values()))))

    if errors:
        sys.exit("\n".join(errors))

    out = [HEADER]
    for key, cls, sig, oslot, rslot in rows:
        out.append('            "%s"  // %s, stock linux %d\n            {\n                "linux"  "%d"\n            }\n\n' % (key, sig, oslot, rslot))
    text = "".join(out).rstrip("\n") + "\n" + FOOTER
    if a.out:
        open(a.out, "w").write(text)
    else:
        sys.stdout.write(text)
    print(f"{len(rows)} keys, slots identical on {', '.join(sorted(sos))}", file=sys.stderr)


HEADER = '''/**
 * Do not edit this file.  Any changes will be overwritten by the gamedata
 * updater or by upgrading your AMX Mod X install.
 *
 * To override data in this file, create a subdirectory named "custom" and
 * place your own gamedata file(s) inside of it.  Such files will be parsed
 * after AMXX's own.
 *
 * For more information, see http://wiki.alliedmods.net/Gamedata_Updating_(AMX_Mod_X)
 */

/**
 * reTS, the source rebuild of The Specialists 3.0. It shares the "ts" game directory with the original,
 * so this file is selected by the export only reTS's game library has. Generated by reTS's
 * scripts/coverage/hamdata.py from the reTS vtables; same slots on i386, amd64 and aarch64.
 */

"Games"
{
    "#default"
    {
        "#supported"
        {
            "gamedll_export"  "ReTS_GetInterface"
        }

        "Offsets"
        {
            "pev"  // 8 on 64-bit; Ham only prints it
            {
                "linux"  "4"
            }

            "base"
            {
                "linux"  "0x0"
            }

'''

FOOTER = '''        }
    }
}
'''

if __name__ == "__main__":
    main()
