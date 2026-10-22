#!/usr/bin/env python3
"""
spawnflags.py — which spawnflag bits the original's entity code tests that reTS's doesn't (and the
reverse), for every entity TS 3.0's FGD gives flags. coverage.py checks each FGD key has a KeyValue
handler; a flag has no handler to find, so a bit TS added (env_fade's 8, "Restrict Area") could go
unported with every report clean.

For each FGD entity: its class (the factory export calls GetClassPtr<Class>), that class and its bases
(reference/coverage/hierarchy.tsv), and every bit their methods test against pev->spawnflags (0x1a0 on
i386: `testb $m,0x1a0(r)`, or a load of 0x1a0..0x1a3 then a test/and of the register), in the original
ts_i386.so and in reTS's i386 build. Only the FGD's own bits are reported, each with its label.

It reads machine code, so it can be wrong both ways: a this-relative member at 0x1a0 reads as a flag
test, and a bit tested through a helper or a copied value is missed. A MISSING row is a lead to read in
the decompile (scripts/ghidra/re.sh <Class>), not a verdict.

  python3 scripts/coverage/spawnflags.py [--so build-i386/dlls/hl.so]   # -> reference/coverage/SPAWNFLAGS.md

A row read and found handled goes in reference/coverage/spawnflags_checked.tsv (entity, bit, verdict,
evidence) and is listed apart from the leads.
"""
import os, re, subprocess, sys
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
REF = os.path.join(ROOT, "reference")
ORIG = os.path.join(REF, "ts_i386.so")
FGD = os.path.join(REF, "ts_fgd.fgd")
HIER = os.path.join(REF, "coverage", "hierarchy.tsv")
OUT = os.path.join(REF, "coverage", "SPAWNFLAGS.md")
# rows someone has read in the source and the decompile: entity, bit, verdict, evidence
CHECKED = os.path.join(REF, "coverage", "spawnflags_checked.tsv")
OURS = os.path.join(ROOT, sys.argv[sys.argv.index("--so") + 1] if "--so" in sys.argv else "build-i386/dlls/hl.so")
SPAWNFLAGS = 0x1A0  # entvars_t.spawnflags, i386


def fgd_flags(path):
    """{entity: {bit: label}}, each class's own flags over its bases' (by base(...))."""
    text = open(path, encoding="latin-1").read()
    text = re.sub(r"//[^\n]*", "", text)
    classes = {}
    for m in re.finditer(r"@(\w+)Class\b([^=]*)=\s*(\w+)", text):
        kind, head, name = m.group(1).lower(), m.group(2), m.group(3)
        bases = re.search(r"base\(([^)]*)\)", head)
        # the class body: from the next [ to its matching ]
        at = text.find("[", m.end())
        nxt = text.find("@", m.end())
        flags = {}
        if at >= 0 and (nxt < 0 or at < nxt):
            depth, i = 0, at
            while i < len(text):
                depth += text[i] == "["
                depth -= text[i] == "]"
                if depth == 0:
                    break
                i += 1
            body = text[at:i]
            fm = re.search(r"spawnflags\s*\(\s*flags\s*\)\s*=\s*\[(.*?)\]", body, re.S)
            if fm:
                for b in re.finditer(r"(\d+)\s*:\s*\"([^\"]*)\"", fm.group(1)):
                    flags[int(b.group(1))] = b.group(2)
        classes[name] = {"kind": kind, "bases": [b.strip() for b in bases.group(1).split(",")] if bases else [], "flags": flags}

    def resolved(name, seen=()):
        c = classes.get(name)
        if not c or name in seen:
            return {}
        out = {}
        for b in c["bases"]:
            out.update(resolved(b, seen + (name,)))
        out.update(c["flags"])
        return out

    return {n: resolved(n) for n, c in classes.items() if c["kind"] != "base" and resolved(n)}


def symbols(so):
    """[(addr, size, name)] of the defined functions, demangled where the tool can."""
    out = subprocess.run(["nm", "-S", "--defined-only", so], capture_output=True, text=True).stdout
    syms = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in "TtWw":
            syms.append((int(parts[0], 16), int(parts[1], 16), parts[3]))
    return syms


def disasm(so, start, size):
    return subprocess.run(["objdump", "-d", "--no-show-raw-insn", f"--start-address={start:#x}",
                           f"--stop-address={start + size:#x}", so], capture_output=True, text=True).stdout


# a register by its family (%eax, %ax, %al, %ah are all a), with the byte a part starts at
FAMILY = {}
for f in "abcd":
    FAMILY[f"e{f}x"] = (f, 0); FAMILY[f"{f}x"] = (f, 0); FAMILY[f"{f}l"] = (f, 0); FAMILY[f"{f}h"] = (f, 8)
for r in ("si", "di", "bp"):
    FAMILY[f"e{r}"] = (r, 0); FAMILY[r] = (r, 0)


def bits_tested(text):
    """Spawnflag bits a function's code tests."""
    bits = set()
    loaded = {}  # register family -> bit shift of the spawnflags byte it holds at its bottom
    for line in text.splitlines():
        m = re.search(r"\t(\w+)\s+(.*)$", line)
        if not m:
            continue
        op, args = m.group(1), m.group(2)
        mem = re.search(r"0x(1a[0-3])\(%\w+\)", args)
        imm = re.search(r"\$0x([0-9a-f]+)", args)
        dest = re.search(r",%(\w+)\s*$", args)
        if op.startswith("test") or op.startswith("and"):
            if mem and imm:
                mask = int(imm.group(1), 16) << (int(mem.group(1), 16) - SPAWNFLAGS) * 8
            elif imm and dest and dest.group(1) in FAMILY:
                fam, part = FAMILY[dest.group(1)]
                if fam not in loaded:
                    continue
                mask = int(imm.group(1), 16) << (loaded[fam] + part)
            else:
                continue
            if mask & 0xFFFFFFFF != 0xFFFFFFFF:
                for b in range(32):
                    if mask & (1 << b):
                        bits.add(1 << b)
        elif op.startswith("mov") and dest and dest.group(1) in FAMILY:
            fam = FAMILY[dest.group(1)][0]
            if mem:
                loaded[fam] = (int(mem.group(1), 16) - SPAWNFLAGS) * 8
            else:
                loaded.pop(fam, None)
    return bits


def hierarchy():
    bases = {}
    for line in open(HIER):
        parts = line.rstrip("\n").split("\t")
        if len(parts) == 2 and parts[0] != "Class":
            bases[parts[0]] = parts[1]
    return bases


def chain(cls, bases):
    out = []
    while cls and cls not in out:
        out.append(cls)
        cls = bases.get(cls)
    return out


def original_classes(syms):
    """{entity: Class}, from each factory's GetClassPtr call."""
    by_name = {n: (a, s) for a, s, n in syms}
    out = {}
    for name, (addr, size) in by_name.items():
        if not re.fullmatch(r"[a-z_0-9]+", name) or size > 0x200:
            continue
        m = re.search(r"<GetClassPtr__H1Z(\d+)(\w+?)_", disasm(ORIG, addr, size))
        if m:
            out[name] = m.group(2)[: int(m.group(1))]
    return out


def gcc2_class(name):
    m = re.match(r"\w+?__(\d+)([A-Za-z_]\w*)", name)
    if not m or name.startswith("_"):
        return None
    return m.group(2)[: int(m.group(1))]


def methods_by_class(syms, so, demangle):
    by_class = defaultdict(list)
    if demangle:
        names = subprocess.run(["c++filt"], input="\n".join(n for _, _, n in syms), capture_output=True, text=True).stdout.splitlines()
        for (a, s, _), d in zip(syms, names):
            m = re.match(r"(\w+)::~?\w+\(", d)
            if m:
                by_class[m.group(1)].append((a, s))
    else:
        for a, s, n in syms:
            c = gcc2_class(n)
            if c:
                by_class[c].append((a, s))
    return by_class


def class_bits(so, by_class, cls_chain):
    bits = set()
    for c in cls_chain:
        for a, s in by_class.get(c, []):
            if s:
                bits |= bits_tested(disasm(so, a, s))
    return bits


def main():
    flags = fgd_flags(FGD)
    bases = hierarchy()
    osyms = symbols(ORIG)
    entity_class = original_classes(osyms)
    orig_methods = methods_by_class(osyms, ORIG, demangle=False)
    our_methods = methods_by_class(symbols(OURS), OURS, demangle=True)
    checked = {}
    if os.path.exists(CHECKED):
        for line in open(CHECKED):
            parts = line.rstrip("\n").split("\t")
            if len(parts) == 4 and parts[0] != "entity":
                checked[(parts[0], int(parts[1]))] = (parts[2], parts[3])
    rows = []
    for ent in sorted(flags):
        cls = entity_class.get(ent)
        if not cls:
            continue
        ch = chain(cls, bases)
        ob = class_bits(ORIG, orig_methods, ch)
        rb = class_bits(OURS, our_methods, ch)
        for bit, label in sorted(flags[ent].items()):
            o, r = bit in ob, bit in rb
            state = "ok" if o == r else ("MISSING" if o else "EXTRA")
            if state != "ok" and (ent, bit) in checked:
                state = "checked"
            rows.append((ent, cls, bit, label, o, r, state))
    lines = ["# Spawnflags: TS 3.0 against reTS", "",
             "Generated by `python3 scripts/coverage/spawnflags.py` (reTS from " + os.path.relpath(OURS, ROOT) + "). For each",
             "entity TS 3.0's FGD gives flags: the bits its class and bases test against pev->spawnflags in the original",
             "ts_i386.so and in reTS. MISSING: the original tests it, reTS doesn't. EXTRA: the reverse. Read from machine",
             "code, so a lead to check in the decompile (`scripts/ghidra/re.sh <Class>`), not a verdict; once read, a row goes",
             "in reference/coverage/spawnflags_checked.tsv and is listed apart.", "",
             "```", f"{'entity':26} {'class':20} {'bit':>6}  {'orig':4} {'reTS':4}  state    label"]
    for ent, cls, bit, label, o, r, state in rows:
        if state in ("MISSING", "EXTRA"):
            lines.append(f"{ent:26} {cls:20} {bit:6}  {'yes' if o else 'no':4} {'yes' if r else 'no':4}  {state:8} {label}")
    lines += ["```", "", "Read and found handled (reference/coverage/spawnflags_checked.tsv):", "", "```"]
    for ent, cls, bit, label, o, r, state in rows:
        if state == "checked":
            lines.append(f"{ent:26} {bit:6}  {checked[(ent, bit)][0]:8} {checked[(ent, bit)][1]}")
    lines += ["```", "", f"{len(rows)} flags on {len({r[0] for r in rows})} entities; "
              f"{sum(r[6] == 'MISSING' for r in rows)} missing, {sum(r[6] == 'EXTRA' for r in rows)} extra, "
              f"{sum(r[6] == 'checked' for r in rows)} read and found handled."]
    open(OUT, "w").write("\n".join(lines) + "\n")
    print(lines[-1], "->", os.path.relpath(OUT, ROOT))


if __name__ == "__main__":
    main()
