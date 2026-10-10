# gdb script: print the engine's sound, model and generic precache tables and flag bad entries.
# Run inside gdb stopped at SV_ActivateServer (scripts/precache/check-live.sh does this). Writes to
# $PRECACHE_OUT (default precache-dump.txt). A BAD entry other than [0] (the engine's empty first slot)
# is a name whose memory changed after it was precached: the engine keeps the pointer, not a copy.
import os

import gdb

out = []
bad = 0
for kind in ("sound_precache", "model_precache", "generic_precache"):
    arr = gdb.parse_and_eval("sv." + kind)
    n = 0
    for i in range(arr.type.range()[1] + 1):
        p = int(arr[i])
        if p == 0:
            continue
        n += 1
        try:
            s = bytes(gdb.selected_inferior().read_memory(p, 96)).split(b"\0")[0]
        except gdb.MemoryError as e:
            s = ("<" + str(e) + ">").encode()
        ok = s and all(32 <= c < 127 for c in s) and (
            s.lower().endswith((b".wav", b".mdl", b".spr", b".bsp", b".txt", b".wad", b".tga", b".res", b".bmp"))
            or s.startswith(b"*"))
        if not ok and i != 0:
            bad += 1
        out.append(f"{'BAD ' if not ok and i != 0 else ''}{kind}[{i}] ptr={p:#x} {s!r}")
    out.append(f"{kind} entries {n}")
out.append(f"bad entries {bad}")
with open(os.environ.get("PRECACHE_OUT", "precache-dump.txt"), "w") as f:
    f.write("\n".join(out) + "\n")
