#!/usr/bin/env python3
"""
curate_members.py — PRODUCER for reference/coverage/members_re.tsv (which was an orphan: no script made
it, so it sat empty and every weapon decompile read `this->field_0xNN` instead of `this->m_pActiveItem`).

The names of UNSAVED members aren't in the binary — but WE'VE ALREADY RE'd many of them, and recorded the
offset in our reconstruction source as a provenance comment, e.g. in dlls/ts_gun.h:
    int m_weaponType = TSW_MELEE; // ACTIVE tsweapon_t (gun+0xc8) ...
    float m_flStuntTime = 0;      // +0x00 main stunt timer ...        (struct-relative, in dlls/ts_stunt.h)
This scrapes those (class, offset, name) facts back out and writes members_re.tsv, so ApplyInventory renders
them as real names.

FAITHFULNESS GUARD (the important bit): a source comment could mis-state the base (an `item+0xNN` /
`player+0xNN` offset belongs to a DIFFERENT struct, not this class). So every scraped (class, offset) is
CROSS-CHECKED against members_auto.tsv — the BINARY-derived offset+type skeleton (DumpMembers). We only emit
a row when the binary actually has a field at that class+offset (corroborated); the type comes from the
binary, the NAME from our source. Anything our source claims that the binary doesn't corroborate is reported
as UNVERIFIED and NOT written (so we never invent a faithful-looking-but-wrong label — see the
[[ghidra-correctness-gotchas]] memory). Existing hand-curated rows in members_re.tsv are preserved.

  python3 scripts/coverage/curate_members.py            # write members_re.tsv + print a report
  python3 scripts/coverage/curate_members.py --dry      # report only, don't write
"""
import os, re, sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
COV = os.path.join(ROOT, "reference", "coverage")
AUTO = os.path.join(COV, "members_auto.tsv")
SAVED = os.path.join(COV, "members.tsv")
OUT = os.path.join(COV, "members_re.tsv")
SRC_DIRS = [os.path.join(ROOT, "dlls"), os.path.join(ROOT, "cl_dll")]

# offset-comment bases that are NOT this class (a different struct/object) — never attribute to the class.
FOREIGN_BASE = re.compile(r"\b(item|player|pev|gpGlobals|CBasePlayer|CBaseEntity|CBaseAnimating)\b", re.I)
# a member declaration:  <type> <name> [= init];   // ... comment
DECL = re.compile(r"^\s*(?:const\s+)?([A-Za-z_][\w:]*\**)\s+(m_[A-Za-z0-9_]+|[a-z][A-Za-z0-9_]*)\s*"
                  r"(?:\[[^\]]*\])?\s*(?:=|;)")
CLASS = re.compile(r"^\s*(?:class|struct)\s+([A-Za-z_]\w*)")
# offset forms in the trailing comment: `+0xNN` (struct-relative or <word>+0xNN) and `name@0xNN`
OFF_REL = re.compile(r"(?:^|[^>\w])\+?(0x[0-9a-fA-F]+)\b")        # a bare/relative +0xNN
OFF_AT = re.compile(r"@(0x[0-9a-fA-F]+)\b")                       # name@0xNN


def load_tsv(path):
    """class -> {offset_int: (fieldtype, cpptype, count)}  from a members*.tsv."""
    d = defaultdict(dict)
    if not os.path.exists(path):
        return d
    with open(path, errors="replace") as f:
        next(f, None)
        for ln in f:
            p = ln.rstrip("\n").split("\t")
            if len(p) >= 5:
                try:
                    d[p[0]][int(p[1], 16)] = (p[2], p[3], p[5] if len(p) > 5 else "1")
                except ValueError:
                    pass
    return d


def existing_re_rows():
    """preserve hand-curated rows already in members_re.tsv -> {(class,off): full_line}."""
    rows = {}
    if os.path.exists(OUT):
        with open(OUT, errors="replace") as f:
            next(f, None)
            for ln in f:
                p = ln.rstrip("\n").split("\t")
                if len(p) >= 5:
                    try:
                        rows[(p[0], int(p[1], 16))] = ln.rstrip("\n")
                    except ValueError:
                        pass
    return rows


def scrape():
    """yield (class, offset_int, name, srcfile, raw_comment) from our headers' offset annotations."""
    for d in SRC_DIRS:
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not fn.endswith(".h"):
                continue
            path = os.path.join(d, fn)
            cls = None
            for ln in open(path, errors="replace"):
                cm = CLASS.match(ln)
                if cm:
                    cls = cm.group(1)
                if cls is None or "//" not in ln:
                    continue
                code, _, comment = ln.partition("//")
                dm = DECL.match(code if code.strip() else ln)
                if not dm:
                    continue
                name = dm.group(2)
                if not name.startswith("m_") and not name[0].islower():
                    continue
                # an offset that belongs to ANOTHER object's base -> skip (faithfulness)
                if FOREIGN_BASE.search(comment):
                    continue
                off = None
                am = OFF_AT.search(comment)
                if am:
                    off = int(am.group(1), 16)
                else:
                    rm = OFF_REL.search(comment)
                    if rm:
                        off = int(rm.group(1), 16)
                if off is None:
                    continue
                yield cls, off, name, fn, comment.strip()


def main():
    dry = "--dry" in sys.argv
    auto = load_tsv(AUTO)
    saved = load_tsv(SAVED)
    keep = existing_re_rows()

    emitted = {}        # (class,off) -> row tuple
    unverified = []     # (class, off, name, file, reason)
    for cls, off, name, fn, comment in scrape():
        if (cls, off) in keep:                      # already hand-curated — leave it
            continue
        if cls in saved and off in saved[cls]:      # the save table already names it authoritatively
            continue
        if cls in auto and off in auto[cls]:
            ftype, ctype, count = auto[cls][off]
            emitted[(cls, off)] = (cls, off, ftype, ctype, name, count)
        else:
            reason = "class not in members_auto" if cls not in auto else f"no binary field at {cls}+0x{off:x}"
            unverified.append((cls, off, name, fn, reason))

    # ---- report ----
    print(f"# curate_members — scraped our source annotations, cross-checked vs members_auto.tsv (binary)")
    by_cls = defaultdict(int)
    for (cls, off) in emitted:
        by_cls[cls] += 1
    print(f"\n## corroborated (will write {len(emitted)} rows across {len(by_cls)} classes):")
    for cls in sorted(by_cls):
        names = sorted(n for (c, o, ft, ct, n, cn) in emitted.values() if c == cls)
        print(f"  {cls:22} {by_cls[cls]:3}  {', '.join(names[:8])}{' …' if len(names) > 8 else ''}")
    if unverified:
        print(f"\n## UNVERIFIED — source claims the binary does NOT corroborate ({len(unverified)}, NOT written):")
        for cls, off, name, fn, reason in sorted(unverified):
            print(f"  {cls}+0x{off:x}  {name}  ({fn}: {reason})")

    if dry:
        print("\n(--dry: nothing written)")
        return

    # ---- write: existing hand-curated rows + newly corroborated, sorted ----
    allrows = dict(keep)  # (class,off) -> raw line
    for (cls, off), (c, o, ft, ct, nm, cn) in emitted.items():
        allrows[(cls, off)] = f"{c}\t0x{o:x}\t{ft}\t{ct}\t{nm}\t{cn}"
    with open(OUT, "w") as w:
        w.write("Class\toffset\tfieldtype\tcpptype\tname\tcount\n")
        for key in sorted(allrows, key=lambda k: (k[0], k[1])):
            w.write(allrows[key] + "\n")
    print(f"\nwrote {OUT}  ({len(allrows)} rows: {len(keep)} preserved + {len(emitted)} new)")
    print("re-apply with: scripts/ghidra/re.sh setup   (so ApplyInventory picks up the new names)")


if __name__ == "__main__":
    main()
