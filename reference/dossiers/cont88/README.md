# cont.88 dossier — ReHLDS interop rig, vestigial removal, merged delta.lst, scope rework

Preserved scratchpads + the reasoning artifacts from the cont.88 session. See the ledger
`## 2026-07-05 (cont. 88)` entry and HANDOFF "START HERE (cont.88)" for the full narrative.

## Files here
- `delta_seed.lst` / `delta_ts.lst` / `delta_stock.lst` — the **3 inputs to the delta.lst 3-way merge**.
  - `delta_seed.lst` = halflife-updated initial-seed commit `de70fad` (the ~1998 HL1 SDK base = common ancestor).
  - `delta_ts.lst` = original TS `delta.lst` (`recovered/ts3-linux/ts/delta.lst`, md5 19af2487, = what Steam ships).
  - `delta_stock.lst` = current halflife-updated `network/delta.lst` (md5 1aaf1652, = anniversary branch).
  - The merge = seed + TS-branch changes + anniversary-branch changes; result committed as `network/delta.lst`.
- `hosterr_break.gdb` — the gdb batch script that breaks on the engine's `Host_Error` on the ReHLDS rig
  and dumps `x/NN $rsp` to resolve the gamedll caller (this caught the w_weaponbox faults). Usage:
  `cd <rig> && timeout 90 env LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH gdb -batch -nx -x hosterr_break.gdb --args ./hlds_linux -game ts +sv_lan 1 +maxplayers 8 +map ts_bikini`

## Regenerable (NOT committed — too big / trivially reproduced)
- `client_disasm.txt` (~8 MB) = `objdump -d -M intel reference/client.dll`. Regenerate on demand; used
  for the legacy-vgui-menu dead-code xref scan and the scope-draw RE (`quarterscope` @ `0x10054516`).

## Key method used this session (for the next agent)
- **Field-level 3-way merge** for config tables (delta.lst): parse to `{struct:{field:(type,bits,div)}}`,
  diff each branch vs the seed ancestor, apply both, resolve overlaps (incl. add/add — same field added by
  both at different widths → take the wider). Beats line-diff (whitespace/comment/order noise).
- **Verify a wire table against the CODE**: (1) every `DEFINE_DELTA` field must be a real struct member
  (grep the SDK headers `common/entity_state.h` etc.); (2) each field's bits must hold the values the
  gamedll actually *writes* (grep the write sites, check range) — too-few bits silently truncate.
