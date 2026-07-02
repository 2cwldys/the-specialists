# gmsg TSState — protocol analysis (mod-ts drop-in reconstruction)

Message name (wire): `TSState` · registered size **1** · Ghidra server global `gmsgTSState`
(strings confirmed in `ts_i386.so`: `.rodata` "TSState" @0xfe4f2, symbol `gmsgTSState` in
.dynstr/.strtab — registration is real, not a leftover).

## WIRE FORMAT

```
TSState (size 1, MSG_ONE to the owning player):
    BYTE  state        // player pose/state: 0 = normal/standing, 1 = ducking, 2 = stunt/locked
```

- Exactly one byte; REG_USER_MSG size 1 (fixed). No other fields.
- Client read (ground truth, handler `FUN_1004b950` @0x1004b950, forwarder 0x100498e0,
  `this` = CTSHud singleton 0x100d43cc):
  ```c
  BEGIN_READ(pbuf, iSize);          // FUN_100299b0
  state = READ_BYTE();              // FUN_10029a10
  *(char*)(this + 0x22) = state;    // CTSHud+0x22 = state byte
  *(uint*)(this + 0x10) |= 1;       // m_iFlags |= HUD_ACTIVE
  ```
- Value source server-side: player member `[0x126]` in the HUD_MESSAGES int-index
  notation = byte offset **0x498** (0x126*4) — the same notation decodes PwUp
  [0x1ca]/[0x1cc] to 0x728/0x730, which our ts_player.cpp independently confirmed.
  API_SURFACE.md:36 "player-state byte (@0x498)" agrees.
- Our reconstruction already has this member: `dlls/player.h:483`
  `int m_iPlayerState; // 0x498 — pose: 0 normal, 1 ducking, 2 stunt/locked`,
  derived by `CBasePlayer::UpdatePlayerState()` (@0x82a08, ts_player.cpp:507):
  `(pev->iuser4 & 0x30) -> 2; (pev->flags & FL_DUCKING) -> 1; else 0`.

## PURPOSE

Networks the player's **pose/state byte** (stand / duck / stunt-locked) to their own
client each frame (change-driven). Client-side it lands in CTSHud+0x22 and drives:

1. The **status-panel posture figure** (bottom-left HUD): the stand/crouched/prone
   sprite (sprites `player/stand|crouched|prone`) — state 0/1/2 maps 1:1 to the
   figure. (Our CHudTSStatus draws exactly this panel.)
2. Per scout notes / HUD_DRAWCHAIN.md, the **dynamic-crosshair colour/state**
   (HUD_DRAWCHAIN.md:41: dyn crosshair "coloured by state", 1 -> red 0xc3,0x2d,0x2d).
   VERIFY which of CTSHud+0x22 vs the global DAT_1018a60c feeds that colour switch
   (drawchain doc names the global; the state byte is the likely writer — confirm in
   the client Draw decompile `FUN_1004e9f0` on the re-project pass).

## SERVER SEND SITES (+VERIFY flags)

- **Sender:** `CBasePlayer::UpdateClientData` @0x81d00 (`TS_player.cpp`, GOT slot
  0x167ac0 per scout) — the same per-frame HUD block that sends TSHealth/TSArmor/
  TSCash/TSSpace. HUD_MESSAGES.md:30: `MSG_ONE`, one `WRITE_BYTE(player[0x498])`.
- **State producer:** `UpdatePlayerState` @0x82a08 derives the byte each frame
  (signatures-gcc2.txt has `CBasePlayer::UpdateClientData()`; our player.cpp:1961
  already calls UpdatePlayerState from the same spot).
- **VERIFY (re-project pass, server .so decompile):**
  - change-throttling: siblings are only sent when the value differs from a cached
    copy; `members_auto.tsv` shows adjacent `CBasePlayer 0x498` AND `0x499` accessed —
    0x499 is plausibly the last-sent cache byte. Confirm.
  - whether a full resend is forced on ResetHUD/respawn (m_fInitHUD path), as the
    siblings do.
  - exact MSG destination (MSG_ONE assumed per HUD_MESSAGES; confirm not MSG_ALL —
    the doc marks the two MSG_ALL cases (ActItems, TSPAward) explicitly, TSState is
    not marked, so MSG_ONE is near-certain).

## CLIENT HANDLER BEHAVIOR

`__UserMsg_TSState` -> forwarder 0x100498e0 (`mov ecx, 0x100d43cc`) ->
`FUN_1004b950(CTSHud* this, pszName, iSize, pbuf)`:

- BEGIN_READ(pbuf, iSize); one READ_BYTE.
- Store the byte at `CTSHud+0x22` (between armor @+0x21 and cash @+0x24 — the same
  singleton whose +0x20/+0x21/+0x24 our CHudTSStatus mirrors as m_iHealth/m_iArmor/
  m_iCash).
- `m_iFlags |= 1` (HUD_ACTIVE) — same as every other CTSHud handler.
- No draw work in the handler; consumption happens in the Draw chain (posture figure,
  and state-coloured dynamic crosshair per HUD_DRAWCHAIN).

## OURS

- **Message: ABSENT both sides.** `grep -r TSState dlls cl_dll` -> no register, no
  send, no HOOK_MESSAGE, no MsgFunc.
- **Server half-exists:** `m_iPlayerState` (player.h:483, provenance 0x498) is derived
  every frame by `UpdatePlayerState()` (ts_player.cpp:507, called from player.cpp:1961
  inside UpdateClientData) — but never networked.
- **Client consumer exists but uses an INVENTED substitute:** `CHudTSStatus::Draw`
  (cl_dll/ts_status.cpp:143-156) picks the posture figure from **local** derivation —
  `g_iTSProne` (an invented hook global, ts_status.cpp:37) + the local player's
  `curstate.usehull == 1` duck check — instead of the server's TSState byte. That is
  a drop-in violation twice over: (a) against the ORIGINAL server our client never
  learns the stunt/locked (prone) state, since nothing sets g_iTSProne from the wire;
  (b) against the original CLIENT our server never feeds the posture figure or the
  state-coloured crosshair at all.
- The audit's invented globals `gmsgStatusText`/`gmsgStatusValue` are stock-HL
  train/status leftovers and `gmsgWeapons` ("Weapons" 8) is an invented loadout
  bitmask — **none is a renamed TSState**; no rename reconcile applies here. The
  substitutes to retire are `g_iTSProne` + the usehull check.

## RECONCILE

Server (dlls):
1. Add `static int gmsgTSState = 0;` in `dlls/ts_player.cpp`, lazily registered in
   `CBasePlayer::TSUpdateClientData()` as `REG_USER_MSG("TSState", 1)` (the file's
   established pattern beside TSHealth/TSArmor/TSCash).
2. In TSUpdateClientData, after the TSArmor block (mirroring the original's
   TSArmor->TSState->TSCash order):
   ```c
   if (m_iPlayerState != m_iClientTSState) {
       m_iClientTSState = m_iPlayerState;
       MESSAGE_BEGIN(MSG_ONE, gmsgTSState, NULL, ENT(pev));
       WRITE_BYTE(m_iPlayerState);
       MESSAGE_END();
   }
   ```
   with `int m_iClientTSState` added beside the other m_iClientTS* caches (player.h),
   reset with them on ResetHUD/respawn so the state re-sends. (VERIFY the original's
   throttle/cache — plausible 0x499 — on the re-project pass; change-throttle matches
   every sibling send in this function.)
3. `UpdatePlayerState()` already runs before the send (player.cpp:1961) — no change.

Client (cl_dll):
4. `CHudTSStatus` absorbs it (it IS our CTSHud+0x20..0x24 mirror): add
   `bool MsgFunc_TSState(const char*, int, void*)` to ts_status.h, `HOOK_MESSAGE(TSState)`
   in `CHudTSStatus::Init()` (ts_status.cpp:49-51 block), member `int m_iState;`
   (reset to 0 in Reset()). Handler = BEGIN_READ + READ_BYTE into m_iState +
   `m_iFlags |= HUD_ACTIVE` — byte-faithful to FUN_1004b950.
5. Replace the invented posture derivation in Draw (ts_status.cpp:143-156):
   `posture = m_iState;` (0 stand / 1 crouched / 2 prone). Delete `g_iTSProne` and the
   `usehull` check (and any stunt-code writes to g_iTSProne — the state now comes from
   the server, as the original does).
6. VERIFY-then-wire the dynamic-crosshair state colour (state==1/2 -> red per
   HUD_DRAWCHAIN.md:41) once `FUN_1004e9f0` is decompiled — expose m_iState (or move
   the byte onto gHUD) for the reticle/crosshair code (cl_dll/ts_reticle.cpp) if
   confirmed.

Registration-order note: the original registers TSState inside LinkUserMessages
between TSArmor and TSCash; our lazy-registration pattern assigns engine msg ids in
first-send order. Ids are negotiated by NAME in SVC_NEWUSERMSG, so order is not part
of the swap contract — but keeping the send adjacent to TSArmor/TSCash keeps the
first-frame id block close to original.
