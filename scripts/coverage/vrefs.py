#!/usr/bin/env python3
"""
vrefs.py — REVERSE virtual-call finder: given a method, find every function that CALLS it through a
vtable. This is the inverse of vtcall.py (which is slot->method) and fills the gap that defeats both
Ghidra's get_function_callers AND DecompRefs.java: a virtual call goes through `this->vtbl[slot]` and
NEVER references the method's address, so address-xref tools find nothing (this is what happened
tracing CTSGun::PrimaryAttack — "No callers found", because every call is `call *0x17c(vptr)`).

How it works:
  1. Invert reference/coverage/vtables_original.json -> method name -> {slot: set(classes)}.
  2. Scan the .so disassembly (objdump -d) for `call *0xSLOT(%reg)` sites, bucketed by the enclosing
     function (the gcc2 old-ABI virtual dispatch shape: load vptr from this+0x60, then call *slot(vptr)).
  3. A call site at slot 0xNN dispatches to whatever class's vtable[0xNN] the RECEIVER has — which we
     can't know statically without dataflow. So we RANK by self-dispatch: if the CALLING function's own
     class has the queried method at that slot (the common `this->Method()` case), it's HIGH confidence;
     otherwise it's a POSSIBLE cross-object/other-class hit at the same slot.

Usage:
  python3 scripts/coverage/vrefs.py PrimaryAttack       # bare name: every class's PrimaryAttack slot
  python3 scripts/coverage/vrefs.py CTSGun::Deploy       # one class's slot only
  python3 scripts/coverage/vrefs.py 0x17c                # a raw slot offset
  python3 scripts/coverage/vrefs.py PrimaryAttack --all  # don't split high/possible — list everything
"""
import json, os, re, subprocess, sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SO = os.path.join(ROOT, "reference", "ts_i386.so")
COV = os.path.join(ROOT, "reference", "coverage")
VT = os.path.join(COV, "vtables_original.json")
MAP = os.path.join(ROOT, "reference", "signatures-gcc2.map")
FUNCFILE = os.path.join(COV, "funcfile.tsv")


def load_vtables():
    if not os.path.exists(VT):
        sys.exit("missing vtables_original.json — run scripts/coverage/extract_vtables.py first")
    return json.load(open(VT))


def method_name(demangled):
    """'CTSGun::PrimaryAttack()' -> 'PrimaryAttack'; 'CTSGun::CTSGun()' stays a ctor name."""
    s = demangled.split("(", 1)[0]
    return s.split("::")[-1] if "::" in s else s


def class_of(demangled):
    """'CTSGun::PrimaryAttack()' -> 'CTSGun'  (None if free function)."""
    s = demangled.split("(", 1)[0]
    return s.rsplit("::", 1)[0] if "::" in s else None


def parse_mangled_class(sym):
    """Best-effort gnu-v2 class extraction from a raw mangled symbol when it's not in the .map.
    e.g. ItemPostFrame__17CBasePlayerWeapon -> CBasePlayerWeapon ; __12CTSAnimation -> CTSAnimation."""
    i = sym.rfind("__")
    if i < 0:
        return None
    suf = sym[i + 2:]
    if suf.startswith("Q"):  # Q<n><parts...> nested — grab the last length-prefixed part
        m = list(re.finditer(r"(\d+)", suf))
        if not m:
            return None
        last = m[-1]
        return suf[last.end(): last.end() + int(last.group(1))] or None
    m = re.match(r"(\d+)", suf)
    if not m:
        return None
    n = int(m.group(1))
    return suf[m.end(): m.end() + n] or None


def demangle_map():
    """mangled symbol -> demangled (from signatures-gcc2.map: 'mangled  =>  demangled')."""
    d = {}
    if os.path.exists(MAP):
        for ln in open(MAP, errors="replace"):
            if "=>" in ln:
                a, b = ln.split("=>", 1)
                d[a.strip()] = b.strip()
    return d


def funcfiles():
    d = {}
    if os.path.exists(FUNCFILE):
        for ln in open(FUNCFILE, errors="replace"):
            p = ln.rstrip("\n").split("\t")
            if len(p) >= 2:
                d[p[0]] = p[1]
    return d


def call_sites():
    """{enclosing_function_symbol: set(slot_int)} for every virtual call.

    Calls whose base register was loaded through the PIC GOT base (`mov X(%ebx),%r`, e.g. &g_engfuncs) are
    global function tables, not vtables, and are dropped: `call *0x58(%esi)` is REMOVE_ENTITY, not
    TakeDamage, and `call *0x17c(%esi)` in SENTENCEG_Init is not PrimaryAttack. Anything else is kept: a
    positive `mov 0x60(%r),%r` vptr load is not required, because objdump's linear sweep sometimes
    misdecodes it (CheckTimeBasedDamage shows `8b 47 60` as or/inc/pusha). Both dispatch shapes count:
    `call *0xNN(%r)` and `mov 0xNN(%vptr),%r` + `call *%r`."""
    dis = subprocess.run(["objdump", "-d", "--no-show-raw-insn", SO],
                         capture_output=True, text=True).stdout
    sites = defaultdict(set)
    cur = None
    reg = {}   # register -> ('vptr', None) | ('slot', off)
    ins = re.compile(r"^\s*[0-9a-f]+:\s+(\w+)\s*(.*)$")
    for ln in dis.splitlines():
        h = re.match(r"^[0-9a-f]+ <([^>]+)>:", ln)
        if h:
            cur, reg = h.group(1), {}
            continue
        if cur is None:
            continue
        m = ins.match(ln)
        if not m:
            continue
        op, args = m.group(1), m.group(2).split("#")[0].strip()
        if op == "call":
            c = re.fullmatch(r"\*(0x[0-9a-f]+)?\((%e[a-z]{2})\)", args)
            if c and reg.get(c.group(2), (None,))[0] != "got":
                sites[cur].add(int(c.group(1), 16) if c.group(1) else 0)
            c = re.fullmatch(r"\*(%e[a-z]{2})", args)
            if c and reg.get(c.group(1), (None,))[0] == "slot":
                sites[cur].add(reg[c.group(1)][1])
            for r in ("%eax", "%ecx", "%edx"):   # caller-saved, clobbered by the call
                reg.pop(r, None)
            continue
        dst = re.search(r",(%e[a-z]{2})$", args) or (re.fullmatch(r"(%e[a-z]{2})", args) if op == "pop" else None)
        if not dst:
            continue
        d = dst.group(1)
        src = args[: dst.start()] if op != "pop" else ""
        if op == "mov":
            v = re.fullmatch(r"0x60\((%e[a-z]{2})\)", src)
            s = re.fullmatch(r"(0x[0-9a-f]+)?\((%e[a-z]{2})\)", src)
            r = re.fullmatch(r"(%e[a-z]{2})", src)
            g = re.fullmatch(r"-?(0x[0-9a-f]+)?\(%ebx\)", src)
            if g:
                reg[d] = ("got", None)
            elif v:
                reg[d] = ("vptr", None)
            elif s and reg.get(s.group(2), (None,))[0] == "vptr":
                reg[d] = ("slot", int(s.group(1), 16) if s.group(1) else 0)
            elif r and r.group(1) in reg:
                reg[d] = reg[r.group(1)]
            else:
                reg.pop(d, None)
        else:
            reg.pop(d, None)
    return sites


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    show_all = "--all" in sys.argv
    if not args:
        sys.exit(__doc__)
    query = args[0]
    vts = load_vtables()

    # slot -> {class: method_name_at_that_slot}, for confidence checks
    slot_class_method = defaultdict(dict)
    # method_name -> {slot: set(classes that have it there)}
    by_name = defaultdict(lambda: defaultdict(set))
    for cls, slots in vts.items():
        for slot, dem in slots.items():
            off = int(slot, 16)
            nm = method_name(dem)
            slot_class_method[off][cls] = nm
            by_name[nm][off].add(cls)

    # resolve the query -> target slots (+ a label)
    target_slots = {}  # slot -> set(classes) that have the queried method there
    if re.fullmatch(r"0x[0-9a-fA-F]+", query):
        off = int(query, 16)
        target_slots[off] = {c for c, nm in slot_class_method[off].items()}
        label = f"slot {query}"
        want_name = None
    elif "::" in query:
        cls, meth = query.split("::", 1)
        want_name = meth.split("(")[0]
        if cls not in vts:
            cand = [c for c in vts if cls.lower() in c.lower()]
            sys.exit(f"unknown class '{cls}'" + (f"; did you mean {', '.join(cand[:8])}" if cand else ""))
        for slot, dem in vts[cls].items():
            if method_name(dem) == want_name:
                target_slots[int(slot, 16)] = {cls}
        if not target_slots:
            sys.exit(f"{cls} has no virtual method '{want_name}' (not in its vtable — maybe non-virtual)")
        label = query
    else:
        want_name = query
        if query not in by_name:
            cand = sorted({nm for nm in by_name if query.lower() in nm.lower()})
            sys.exit(f"no virtual method named '{query}' (not in any vtable — it's non-virtual; for a "
                     f"direct/non-virtual method use ordinary address xrefs: the MCP get_xrefs_to / "
                     f"re.sh refs / DecompRefs.java)"
                     + (f"\nclose virtual names: {', '.join(cand[:8])}" if cand else ""))
        for slot, classes in by_name[query].items():
            target_slots[slot] = classes
        label = query

    dmap = demangle_map()
    ffiles = funcfiles()

    def caller_class(sym):
        if sym in dmap:
            return class_of(dmap[sym])
        return parse_mangled_class(sym)

    def pretty(sym):
        return dmap.get(sym, sym)

    sites = call_sites()
    # collect callers: (sym, slot, confidence)
    high, possible = [], []
    for sym, slots in sites.items():
        for off in slots:
            if off not in target_slots:
                continue
            cc = caller_class(sym)
            # high confidence: the calling function's own class has the queried method at this slot
            hit = cc in vts and off in [int(s, 16) for s in vts.get(cc, {})] and \
                  (want_name is None or slot_class_method[off].get(cc) == want_name)
            row = (pretty(sym), f"0x{off:x}", ffiles.get(sym, "?"))
            (high if hit else possible).append(row)

    slotdesc = ", ".join(f"0x{s:x} ({', '.join(sorted(c)[:4])})" for s, c in sorted(target_slots.items()))
    print(f"# vrefs: {label}")
    print(f"# slot(s): {slotdesc}\n")

    def dump(rows, title):
        rows = sorted(set(rows))
        print(f"## {title} ({len(rows)})")
        for fn, slot, cpp in rows:
            print(f"  {slot}  {fn:55} [{cpp}]")
        print()

    if show_all:
        dump(high + possible, "callers (all)")
    else:
        dump(high, "callers — self-dispatch (high confidence)")
        dump(possible, "callers — same slot, other/unknown receiver class (possible)")
    if not high and not possible:
        print("  (no virtual call sites found at these slots — the method may be called only directly,")
        print("   or only via base-class pointers whose receiver type isn't this class)")


if __name__ == "__main__":
    main()
