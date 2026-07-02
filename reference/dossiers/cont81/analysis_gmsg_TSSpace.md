# gmsg TSSpace — analysis

## WIRE FORMAT

- Name (wire, exact): `TSSpace`
- Registered size: **1** (fixed) — `REG_USER_MSG("TSSpace", 1)` (server global `gmsgTSSpace`, .data, GLOBALS.md:81)
- Direction: server → client, **MSG_ONE** (per-player, unicast)
- Payload, in order:

| # | Type | Meaning |
|---|------|---------|
| 1 | BYTE | free inventory space (slot-weight remaining; server-side range 0..81) |

Total = 1 byte. Corroborated three ways: HUD_MESSAGES.md:32 (`BYTE space (inventory
weight/slots)`), API_SURFACE.md:37 (size 1, "free slots" left HUD), and the client
handler decompile which performs exactly one READ_BYTE.

## PURPOSE

Drives the left-HUD "free space" readout — the player's remaining inventory
weight/slot capacity (the same 0..81 slot-weight pool the buy menu spends and that
gates dive height / flag pickup). Client stores it as a signed char and marks the
HUD element active; it is a display-only status value like its siblings TSHealth /
TSArmor / TSCash.

## SERVER SEND SITES (+VERIFY flags)

- Sender: `CBasePlayer::UpdateClientData` @0x81d00 in `TS_player.cpp`
  (GOT slot 0x167388 = `gmsgTSSpace`) — same site as the TSHealth/TSArmor/TSCash/
  PwUp/TSSlowMo block (HUD_MESSAGES.md).
- Pattern (per HUD_MESSAGES.md notes, shared by the whole block): sent **on change
  only** (cached previous value compared each frame) under the ~0.1 s player
  throttle gate (player member [0x14d]).
- Binary evidence: `rz-bin -zz` shows `gmsgTSSpace` (.dynstr/.strtab) and the wire
  string `TSSpace` @ .rodata 0xfe501. No `MsgFunc`-style server symbols expected
  (it's a data global, not a function).
- **VERIFY (re-project pass, server .so decompile of UpdateClientData @0x81d00):**
  1. Exact value expression. Presumed source: the free-slot capacity int at
     `CBasePlayer+0x548` (init 81) — corroborated by `members_auto.tsv`
     (`CBasePlayer 0x548 INTEGER int field_0x548`) and by our RE of the same pool
     in `dlls/ts_gun.cpp` (`TSPlayerFreeSlots` = 81 − SlotsUsed − 20 if carrying
     flag, clamped 0..81). Confirm whether the send reads +0x548 directly or
     recomputes, and whether any clamp happens at the WRITE_BYTE.
  2. The cached-previous-value member offset used for the change check.
  3. Confirm MSG_ONE dest (vs MSG_ONE_UNRELIABLE).

## CLIENT HANDLER BEHAVIOR

Handler `FUN_1004b9b0` (46 bytes), reached via forwarder 0x10049920 with
`this` = static HUD-element object @0x100d43cc (the TS status/left-HUD element):

```c
FUN_100299b0(pbuf, iSize);            // BEGIN_READ
uVar1 = FUN_10029a10();               // READ_BYTE
*(char *)(this + 0x28) = (char)uVar1; // m_iSpace (signed char storage)
*(uint *)(this + 0x10) |= 1;          // m_iFlags |= HUD_ACTIVE
```

No other side effects: no sound, no clamp, no derived state. The draw code of that
element renders the value on the left HUD ("free slots"). Storage is a single
byte at object+0x28; flags dword at +0x10.

## OURS

**Status: ABSENT (both sides).**

- Server `dlls/`: no `TSSpace` anywhere. The sibling messages ARE done —
  `ts_player.cpp` `CBasePlayer::TSUpdateClientData()` lazily registers + sends
  TSHealth(1)/TSArmor(1)/TSCash(2)/PwUp(3)/TSSlowMo(2) on-change. TSSpace is the
  gap in that block. The VALUE already exists: `TSPlayerFreeSlots(CBasePlayer*)`
  in `dlls/ts_gun.cpp` (decl `dlls/ts_weapons.h:59`) — used by CTSStunt dive
  gates and flag pickup, never sent to the client.
- Client `cl_dll/`: no hook, no handler, no space display. `CHudTSStatus`
  (`ts_status.cpp/.h`) hooks only TSHealth/TSArmor/TSCash.
- Audit's invented globals are NOT wrong-name substitutes for this message:
  `gmsgStatusText`/`gmsgStatusValue` are stock HL SDK (StatusText/StatusValue,
  train/status-bar), and `gmsgWeapons` ("Weapons", 8) is the halflife-updated
  SDK's 64-bit m_WeaponBits transport (player.cpp:4421). No rename applies —
  this is a pure ADD, not a rename.

## RECONCILE

Server (`dlls/ts_player.cpp`, inside `CBasePlayer::TSUpdateClientData`):
1. Add `static int gmsgTSSpace = 0;` next to gmsgTSHealth/TSArmor/TSCash.
2. Lazy-register with the exact original name/size:
   `if (gmsgTSSpace == 0) gmsgTSSpace = REG_USER_MSG("TSSpace", 1);`
3. Add cached member `int m_iClientTSSpace` (transient, next to
   m_iClientTSHealth/m_iClientTSCash) and send on change:
   ```c
   int space = TSPlayerFreeSlots(this); // 0..81 — VERIFY vs +0x548 direct read
   if (space != m_iClientTSSpace) {
       m_iClientTSSpace = space;
       MESSAGE_BEGIN(MSG_ONE, gmsgTSSpace, NULL, ENT(pev));
       WRITE_BYTE(space);
       MESSAGE_END();
   }
   ```
4. Reset `m_iClientTSSpace = -1` wherever the sibling caches reset (spawn/ResetHUD
   path) so the first frame always sends.

Client (`cl_dll/ts_status.cpp/.h` — CHudTSStatus is the natural absorber; it IS
the left/status HUD element and already owns the sibling messages):
1. `DECLARE_MESSAGE(m_TSStatus, TSSpace)` + `HOOK_MESSAGE(TSSpace)` in Init().
2. `bool CHudTSStatus::MsgFunc_TSSpace(const char*, int iSize, void* pbuf)`:
   BEGIN_READ(pbuf, iSize); `m_iSpace = READ_BYTE();` `m_iFlags |= HUD_ACTIVE;`
   return true. (Original stores a signed char at +0x28 — a plain int member is
   fine; behaviour-identical for 0..81.)
3. Draw `m_iSpace` in the left status panel ("free slots" readout) alongside
   health/armor/cash — exact placement/label per the original left HUD (VERIFY
   the draw string/position from the client.dll draw fn when styling).

Coverage effect: closes the `TSSpace` MISSING row in the user-message contract
(coverage.py) and the `gmsgTSSpace` MISSING global in AUDIT.md:162
(file attribution: TS_player.cpp — matches putting it in ts_player.cpp).
