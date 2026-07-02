# gmsgTSWires — wire name `"Wires"`, size 3

## WIRE FORMAT

```
REG_USER_MSG("Wires", 3)          // server global: gmsgTSWires (in .dynstr of ts_i386.so)
MSG_ONE (targeted at one player)
  BYTE  wire0 state
  BYTE  wire1 state
  BYTE  wire2 state
```

Fixed size 3, three consecutive BYTEs — confirmed on the READ side by the client handler
(FUN_1004fb00: exactly three `READ_BYTE` calls, `FUN_10029a10`, after `BEGIN_READ`
`FUN_100299b0(pbuf, iSize)`), and on the registration side by `HUD_MESSAGES.md:39`
(`Wires | 3 | gmsgTSWires | CBasePlayer::UpdateClientData | BYTE,BYTE,BYTE`).

Byte SEMANTICS (per-wire display state — e.g. 0 = hidden/cut vs nonzero = intact/shown,
or a small enum) are **VERIFY**: the client stores them opaquely; the exact values come
from the server encoder (`CBasePlayer::UpdateClientData` / `CBasePlayer::CheckForWires`)
which needs the re-project decompile. Note the server must NOT leak which wire is the
defuse/boobytrap wire, so the bytes are almost certainly visibility/cut state, not the
`WIRE_DEFUSE/WIRE_BOOBYTRAP/WIRE_NEUTRAL` roles.

## PURPOSE

Bomb wire-cut HUD state. When a player can interact with a planted `ts_bomb` that has
`cutwires` enabled, the server pushes the 3-wire state to that player; the client then
**replaces the dynamic crosshair with the bomb-wires display** (pick wire 1/2/3 →
`cutwire_<n>` command back to the server). When the wires state clears, the normal
crosshair returns.

## SERVER SEND SITES (+VERIFY)

- **Sole sender: `CBasePlayer::UpdateClientData`** (HUD_MESSAGES.md:39; scout confirmed
  3x WriteByte there). The `CTSGameRules::CheckVotes` "hit" at 0x1828 is a
  `this+0x1828` member access — false positive, not a send site.
- Supporting original symbols in `ts_i386.so` (rz-bin):
  - `gmsgTSWires` (.dynstr — the registered message id global)
  - `CheckForWires__11CBasePlayer` — **VERIFY**: almost certainly the helper that scans
    for a nearby active/cuttable bomb and computes the 3 bytes (we have NO equivalent).
  - `CutWire__11CBasePlayeri`, `CutWire__7CTSBombiP11CBasePlayer`,
    `SetRandomWires__7CTSBomb` — the cut path (we have all three).
  - .rodata strings: `"Wires"` (@0xfe533, the REG_USER_MSG name), `"cutwires"`
    (CTSBomb keyvalue), `"cancutwire"` (CTSBomb save field @0x84, members.tsv:269),
    `"cutwire_"` (console-command prefix, BUY.md:15).
- **VERIFY for the re-project pass:** (a) exact trigger/condition in UpdateClientData
  (proximity? `cancutwire`? bomb active?), (b) byte value encoding, (c) change-detection
  (does it cache last-sent bytes and only send on change — the usual UpdateClientData
  pattern), (d) whether a "clear" (0,0,0) message is sent when the player walks away.
  Also heed the map-change gotcha: any change-detect cache must reset on level change.

## CLIENT HANDLER BEHAVIOR (ground truth: gmsg_TSWires.c)

Forwarder `0x1004c8a0` → handler `FUN_1004fb00(this=0x100d0ca0, name, iSize, pbuf)`.
The receiving object `0x100d0ca0` is the big **weapon-grid/crosshair HUD object** — the
SAME object that handles `WStatus` (handler 0x1004f990; our equivalent class is
`CHudTSWeaponBar`).

```c
BEGIN_READ(pbuf, iSize);
this->0x36fc = (byte)READ_BYTE();   // wire0
this->0x36fd = (byte)READ_BYTE();   // wire1
this->0x36fe = (byte)READ_BYTE();   // wire2
this->0x10  |= 1;                   // m_iFlags |= HUD_ACTIVE
```

Draw effect (HUD_DRAWCHAIN.md:42): in the object's draw chain, step 4 draws either
`FUN_1004e9f0` (dynamic spread crosshair) **or `FUN_1004d550` (bomb wires) when wires
active (`this+0x36fc..fe`)** — i.e. nonzero wire state swaps the crosshair for the
wire-cut UI. The internals of `FUN_1004d550` (layout/colours/labels 1-2-3) are
**VERIFY** (decompile via `dc.sh client 0x1004d550` in the re-project pass).

## OURS

**Message: ABSENT (both sides). Mechanics: partially present (server only).**

- Server (`dlls/`): `CTSBomb` is reconstructed (`ts_bomb.cpp/.h`: `m_wires[3]`,
  `SetRandomWires`, `CutWire(int, CBasePlayer*)`, `cancutwire`/`cutwires` keyvalue) and
  the `cutwire_<n>` console command is wired (`client.cpp:686` →
  `CBasePlayer::CutWire(int)` @ ts_bomb.cpp:198, a free helper that FindEntityByClassname
  loops all `ts_bomb`s — **VERIFY vs original CheckForWires/CutWire targeting**).
  But: no `gmsgTSWires` global, no `REG_USER_MSG("Wires", 3)`, no send in our
  `CBasePlayer::UpdateClientData`, no `CBasePlayer::CheckForWires`.
- Client (`cl_dll/`): nothing — no `HOOK_MESSAGE(Wires)`, no wires state, no
  wires-crosshair draw (grep for "wire" only finds comments).
- The audit's invented globals are NOT renamed substitutes for this message:
  `gmsgStatusText`/`gmsgStatusValue` are the stock HL SDK status-bar (player.cpp
  UpdateStatusBar, WRITE_STRING/WRITE_SHORT payloads — wrong shape) and `gmsgWeapons`
  (size 8) is the stock weapon-bits message. No rename applies; this is a clean ADD.

## RECONCILE

Server side:
1. `dlls/UserMessages.h`: add `inline int gmsgTSWires = 0;`
   `dlls/UserMessages.cpp` (LinkUserMessages): `gmsgTSWires = REG_USER_MSG("Wires", 3);`
   — exact wire name `"Wires"`, exact size 3 (the drop-in contract).
2. Implement `CBasePlayer::CheckForWires()` (name-faithful, per
   match-original-names-for-coverage) from the re-project decompile — computes the
   3 wire bytes for the bomb the player can currently cut (or zeros).
3. Send from `CBasePlayer::UpdateClientData()` (the sole original send site), on
   change, `MSG_ONE` to `pev`:
   `MESSAGE_BEGIN(MSG_ONE, gmsgTSWires, NULL, pev); WRITE_BYTE(w0); WRITE_BYTE(w1);
   WRITE_BYTE(w2); MESSAGE_END();`
   Cache the last-sent triple in CBasePlayer; reset the cache on level change
   (map-change-state-reset gotcha) so the new map re-broadcasts.
4. Cross-check `CBasePlayer::CutWire`'s bomb-targeting against the original (our
   all-bombs loop is a stand-in).

Client side:
5. Handler lives on the weapon-bar/crosshair object → ours is **`CHudTSWeaponBar`**
   (`cl_dll/ts_weaponbar.cpp`, already hooks `WStatus` on the same original object
   0x100d0ca0): add `DECLARE_MESSAGE(m_TSWeaponBar, Wires)` + `HOOK_MESSAGE(Wires)` in
   Init; `MsgFunc_Wires` reads 3 BYTEs into `byte m_wires[3]` (provenance: obj+0x36fc..
   0x36fe) and sets `m_iFlags |= HUD_ACTIVE` — byte-faithful mirror of FUN_1004fb00.
6. Draw: in the crosshair path (ours: `cl_dll/ts_reticle.cpp` draws the dynamic
   crosshair), branch to the wires-crosshair draw when any `m_wires[i]` is active,
   reconstructing `FUN_1004d550` (decompile first — VERIFY). Since our code splits the
   original monolith object across files, keep the STATE + handler in CHudTSWeaponBar
   (name/offset parity for the audit) and let the reticle draw read it, or move the
   crosshair branch wholesale into the weapon-bar class to match the original draw
   chain — decide in the restructure pass.
