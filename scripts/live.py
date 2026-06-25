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
  scripts/live.py readabs 0xADDR [f|d|u|p]    # read at an absolute live address
  scripts/live.py vmethod 0xOBJ <slot>        # this->vtable[slot] -> resolve the virtual fn (e.g. slot 14)
  scripts/live.py watch client.dll 0xRVA [--go "fire now"]   # value-watch (fires on CHANGE); auto-resolve bt
  scripts/live.py watch abs 0x2d8aec4 --go "fire, no mouse"  # ...or watch an ABSOLUTE addr (e.g. engine data)
  scripts/live.py break mp.dll 0xRVA [--go "..."]            # hardware bp -> regs + stack + resolved bt
        # 1st positional is a module name OR the literal "abs" (then the 2nd arg is the absolute live addr).
        # optional: [f|d|u] watch type, --timeout SECS (default 180).

Run watch/break BACKGROUNDED (harness run_in_background): it prints ">>> ARMED — <go>" once set, blocks until
it trips, then prints the resolved backtrace. Tell the user the --go message, wait for the trip. A value-watch
only fires when the value CHANGES — so e.g. watching cl.viewangles pitch with no mouse isolates the recoil.

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
    return readabs(pid, module_base(pid, module) + rva, fmt)

def readabs(pid, addr, fmt):  # read at an ABSOLUTE live address (not module+rva)
    mem = open(f"/proc/{pid}/mem", "rb"); mem.seek(addr)
    if fmt == "d": return struct.unpack("<d", mem.read(8))[0]
    if fmt == "u": return struct.unpack("<I", mem.read(4))[0]
    if fmt == "p": return hex(struct.unpack("<I", mem.read(4))[0])
    return struct.unpack("<f", mem.read(4))[0]

def vmethod(pid, objaddr, slot):
    # this->vtable[slot]: vtable = *obj; fn = *(vtable + slot*4); resolve it
    try:
        vt = readabs(pid, objaddr, "u")
        fn = readabs(pid, vt + slot * 4, "u")
    except OSError:
        return f"obj 0x{objaddr:x}: unreadable — not a valid object pointer (need a live `this` from a break)"
    return f"obj 0x{objaddr:x}  vtable 0x{vt:x}  slot {slot} (off 0x{slot*4:x}) -> " + resolve(pid, fn)

def resolve_text(pid, text):
    # resolve every hex address in `text` that lands in a mapped module region (for gdb backtraces)
    rng = [(s, e) for s, e, _, p in maps(pid) if p]
    seen, out = set(), []
    for x in re.findall(r"0x[0-9a-f]{6,}", text):
        v = int(x, 16)
        if v not in seen and any(s <= v < e for s, e in rng):
            seen.add(v); out.append("  " + resolve(pid, v))
    return out

def _run_gdb(pid, gdb_script, go_msg, timeout):
    """Run a gdb batch script streaming its output; print >>> ARMED (then go_msg) once the bp/watchpoint is
    set, block until it trips (or timeout), then print the capture + the resolved backtrace."""
    f = "/tmp/_live_gdb.txt"
    open(f, "w").write(gdb_script)
    cmd = ["timeout", str(timeout), "gdb", "-p", str(pid), "-batch", "-nx", "-x", f]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
    cap, armed = [], False
    for line in proc.stdout:
        cap.append(line)
        if not armed and ("Hardware watchpoint" in line or "Hardware assisted breakpoint" in line):
            armed = True
            print(f">>> ARMED — {go_msg}", flush=True)
    proc.wait()
    text = "".join(cap)
    print("\n=== gdb capture ===")
    # keep the interesting lines (skip the LWP spam)
    for ln in text.splitlines():
        if not ln.startswith("[New LWP") and ln.strip():
            print(ln)
    print("\n=== resolved frames ===")
    for r in resolve_text(pid, text):
        print(r)
    if not armed:
        print("(!) never armed — check the address / that the original is running", file=sys.stderr)

def _addr_of(pid, where, rva):
    # where == "abs" -> rva IS the absolute live address; else where is a module + rva
    return rva if where == "abs" else module_base(pid, where) + rva

def watch(pid, where, rva, go_msg, fmt, timeout):
    addr = _addr_of(pid, where, rva)
    ctype = {"f": "float", "d": "double", "u": "int"}.get(fmt, "float")
    print(f"watching {where}+0x{rva:x} = 0x{addr:x} (*{ctype}*, current={readabs(pid,addr,fmt)})")
    script = (
        "set pagination off\nset confirm off\nset height 0\nset width 0\n"
        f"watch *({ctype}*)0x{addr:x}\n"
        'commands\n  printf "\\n===WATCH HIT===\\n"\n  bt 24\n  detach\n  quit\nend\ncontinue\n')
    _run_gdb(pid, script, go_msg, timeout)

def brk(pid, where, rva, go_msg, timeout):  # hardware breakpoint -> dump regs + stack + bt (read this/args)
    addr = _addr_of(pid, where, rva)
    print(f"hbreak {where}+0x{rva:x} = 0x{addr:x}")
    script = (
        "set pagination off\nset confirm off\nset height 0\nset width 0\n"
        f"hbreak *0x{addr:x}\n"
        'commands\n  printf "\\n===BREAK HIT===\\n"\n'
        '  printf "eax=%x ecx=%x edx=%x esi=%x edi=%x esp=%x ebp=%x\\n",$eax,$ecx,$edx,$esi,$edi,$esp,$ebp\n'
        "  x/24xw $esp\n  bt 24\n  detach\n  quit\nend\ncontinue\n")
    _run_gdb(pid, script, go_msg, timeout)

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
    elif cmd == "readabs":
        print(readabs(pid, int(a[1], 16), a[2] if len(a) > 2 else "f"))
    elif cmd == "vmethod":
        print(vmethod(pid, int(a[1], 16), int(a[2])))
    elif cmd in ("watch", "break"):
        go, tmo = "GO — perform the action now", 180
        if "--go" in a:
            i = a.index("--go"); go = a[i + 1]; a = a[:i] + a[i + 2:]
        if "--timeout" in a:
            i = a.index("--timeout"); tmo = int(a[i + 1]); a = a[:i] + a[i + 2:]
        fmt = a[4] if len(a) > 4 and not a[4].startswith("-") else "f"
        (watch(pid, a[1], int(a[2], 16), go, fmt, tmo) if cmd == "watch"
         else brk(pid, a[1], int(a[2], 16), go, tmo))
    else:
        print(__doc__)

if __name__ == "__main__":
    main()
