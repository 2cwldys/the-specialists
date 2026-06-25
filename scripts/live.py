#!/usr/bin/env python3
"""live.py — drive the LIVE original TS (hl.exe under Wine/Proton) for dynamic RE.

Static RE dead-ends on virtual/indirect calls + client-side effects (e.g. the angle-recoil — see the
ledger "LIVE TRACE located the angle-recoil"). This codifies the live-gdb workflow so it's not hand-built
each time:

  scripts/live.py pid                         # the live hl.exe PID (the big-RSS one that maps mp.dll)
  scripts/live.py base mp.dll                 # live load base of a module (reloc-free prologue scan)
  scripts/live.py resolve 0xADDR [0xADDR...]  # raw live addr(s) -> module + RVA + nearest export name
  scripts/live.py bt                          # read a gdb backtrace on stdin, resolve every frame
  scripts/live.py read mp.dll 0xRVA [f|d|u|p] # read float/double/uint/ptr at module+RVA (live)

Module base = (a known reloc-free anchor function's live address found by scanning /proc/<pid>/mem for its
file prologue bytes) − its RVA.  resolve() then maps any live address to RVA via /proc/<pid>/maps and
names it from the file's export table (rz-bin -E).  RVA->Ghidra: PE imagebase 0x10000000 (mp/client.dll),
ELF +0x10000 (ts_i386.so). See CLAUDE.md §4b for the gdb recipe (hbreak/watch + bt + detach).
"""
import os, re, sys, struct, subprocess, glob

REF = os.path.join(os.path.dirname(__file__), "..", "reference")
# module -> (reference file, a RELOC-FREE anchor: (file_offset/RVA, nbytes)). PE: paddr==RVA here.
MODULES = {
    "mp.dll":     (f"{REF}/mp.dll",     0x4b9a0, 20),  # CBreakable::Die prologue (no abs operands)
    "client.dll": (f"{REF}/client.dll", 0x69b90, 14),  # V_CalcRefdef leading bytes (before the a3 abs-store)
    "ts_i386.so": (f"{REF}/ts_i386.so", None,   None),  # ELF server (mapped natively, base from maps)
}

def sh(c): return subprocess.run(c, shell=True, capture_output=True, text=True).stdout

def find_pid():
    best = None
    for p in sh("pgrep -f 'Half-Life/hl.exe'").split() + sh("pgrep -f hl.exe").split():
        try:
            if "mp.dll" not in open(f"/proc/{p}/maps").read():
                continue
            rss = int(re.search(r"VmRSS:\s+(\d+)", open(f"/proc/{p}/status").read()).group(1))
            if not best or rss > best[1]:
                best = (int(p), rss)
        except Exception:
            pass
    return best[0] if best else None

def maps(pid):
    out = []
    for ln in open(f"/proc/{pid}/maps"):
        m = re.match(r"([0-9a-f]+)-([0-9a-f]+) (\S+).*?(\S+)?$", ln)
        if not m: continue
        s, e, perm = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
        path = ln.split()[-1] if "/" in ln else ""
        out.append((s, e, perm, path))
    return out

def module_base(pid, module):
    fn, rva, n = MODULES[module]
    rx = [(s, e) for s, e, perm, path in maps(pid) if module in path and "x" in perm]
    if rva is None:  # ELF: base = lowest mapping of the file
        cands = [s for s, e, perm, path in maps(pid) if module in path]
        return min(cands) if cands else None
    pat = open(fn, "rb").read()[rva:rva + n]
    mem = open(f"/proc/{pid}/mem", "rb")
    for s, e in rx:
        mem.seek(s); buf = mem.read(e - s)
        i = buf.find(pat)
        if i >= 0:
            return s + i - rva
    return None

def exports(module):
    fn = MODULES[module][0]
    tbl = []  # (paddr/RVA, name)
    for ln in sh(f"rz-bin -E {fn} 2>/dev/null").splitlines():
        m = re.search(r"0x([0-9a-f]+)\s+0x[0-9a-f]+\s+\w+\s+FUNC\s+\d+\s+\S+\s+(.+)$", ln)
        if m: tbl.append((int(m.group(1), 16), m.group(2).strip()))
    tbl.sort()
    return tbl

def resolve(pid, addr):
    for s, e, perm, path in maps(pid):
        if s <= addr < e and path:
            mod = os.path.basename(path)
            key = next((k for k in MODULES if k == mod), None)
            base = module_base(pid, key) if key else s
            rva = addr - base
            name = "?"
            if key:
                tbl = exports(key)
                below = [(a, nm) for a, nm in tbl if a <= rva]
                if below:
                    a, nm = below[-1]; name = f"{nm}+0x{rva-a:x}"
            return f"0x{addr:x}  {mod}!{name}  (RVA 0x{rva:x}, base 0x{base:x})"
    return f"0x{addr:x}  <unmapped or anonymous>"

def read(pid, module, rva, fmt):
    base = module_base(pid, module)
    mem = open(f"/proc/{pid}/mem", "rb"); mem.seek(base + rva)
    if fmt == "d": return struct.unpack("<d", mem.read(8))[0]
    if fmt == "u": return struct.unpack("<I", mem.read(4))[0]
    if fmt == "p": return hex(struct.unpack("<I", mem.read(4))[0])
    return struct.unpack("<f", mem.read(4))[0]

def main():
    a = sys.argv[1:]
    if not a: print(__doc__); return
    pid = find_pid()
    if not pid:
        print("no live hl.exe (mapping mp.dll) — is the original TS running?", file=sys.stderr); sys.exit(1)
    cmd = a[0]
    if cmd == "pid":
        print(pid)
    elif cmd == "base":
        print(hex(module_base(pid, a[1])))
    elif cmd == "resolve":
        for x in a[1:]:
            print(resolve(pid, int(x, 16)))
    elif cmd == "bt":  # resolve a gdb backtrace piped on stdin (lines like "#3 0x0c0cb3cb in ?? ()")
        for ln in sys.stdin:
            for x in re.findall(r"0x[0-9a-f]{6,}", ln):
                v = int(x, 16)
                if any(s <= v < e for s, e, _, p in maps(pid) if p): print("  " + resolve(pid, v))
    elif cmd == "read":
        print(read(pid, a[1], int(a[2], 16), a[3] if len(a) > 3 else "f"))
    else:
        print(__doc__)

if __name__ == "__main__":
    main()
