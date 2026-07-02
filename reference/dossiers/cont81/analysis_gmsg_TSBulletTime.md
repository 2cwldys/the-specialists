# gmsgTSBulletTime — wire name "TSBTime"

Registered server-side as `gmsgTSBulletTime` (symbol confirmed in
`reference/ts_i386.so` .dynstr: `gmsgTSBulletTime`; registered wire string
`TSBTime` in .rodata @0xfe54a). Client handler (forwarder 0x10049a60 →
handler FUN_1004b8e0, `this` = CTSHud instance 0x100d43cc) decompile:
`scratchpad/gmsg_TSBulletTime.c`.

## WIRE FORMAT

```
REG_USER_MSG("TSBTime", 1)          // fixed size 1
MSG_ONE to the owning player:
    BYTE  bullettime                // 0 = off, nonzero = on
```

Ground truth, handler FUN_1004b8e0:

```c
FUN_100299b0(pbuf, iSize);          // BEGIN_READ(pbuf, iSize)
uVar1 = FUN_10029a10();             // READ_BYTE()
DAT_1018d178 = uVar1 != 0;          // global client bool: g_bBulletTime
this->m_iFlags |= 1;                // CTSHud m_iFlags |= HUD_ACTIVE (this+0x10)
```

Exactly one BYTE, no other fields. The client treats it as a boolean
(`!= 0`), so byte-faithful send is `WRITE_BYTE(flag)` with flag 0/1.
Matches `reference/HUD_MESSAGES.md:42` (`TSBTime | 1 | gmsgTSBulletTime |
CBasePlayer::UpdateClientData | BYTE bullettime ([0x5d])`) and
`API_SURFACE.md:46` (`TSBTime | 1 | — | bullet-time`; also in the
registered-name list at line 63).

## PURPOSE

Per-player "you are in bullet-time" flag, server → owning client. It drives
the client's bullet-time render/behaviour variant — concretely the global
bool at DAT_1018d178, which the client fire/recoil path consumes: the RE'd
aim-recoil function (client.dll FUN_10008230, our
`cl_dll/view.cpp:TS_AddViewRecoil`) scales both recoil components by
0.66667 when the bullet-time flag is set. It also wakes the owning CTSHud
element (`m_iFlags |= HUD_ACTIVE`). (Other consumers of DAT_1018d178 —
e.g. render overlays — VERIFY in a later clientdll Ghidra pass; the recoil
consumer is corroborated by our own FUN_10008230 RE.)

## SERVER SEND SITES (+VERIFY)

- Sender: `CBasePlayer::UpdateClientData` (@0x81d00 in ts_i386.so), per
  HUD_MESSAGES.md — the standard TS pattern: change-detect against a cached
  last-sent member, `MSG_ONE` to the player, throttled like the other
  per-frame TS HUD sends. Corroborated by scout intel (GOT slot 0x16735c
  ref from UpdateClientData + funcfile.tsv → TS_player.cpp).
- Value sent: the player's bullet-time byte. **VERIFY (offset conflict):**
  HUD_MESSAGES.md says player member `[0x5d]`; our provenance says the flag
  byte is 0x175 with the change-detect cache at 0x174 (`dlls/player.h:414
  m_iClientBulletTime // 0x174`, `dlls/ts_player.cpp:24 "byte 0x175"`).
  Note 0x5d*4 = 0x174, so `[0x5d]` may be a dword-index rendering of the
  same slot. Resolve against the re-project decompile of UpdateClientData.
- Server-side semantics: presumably set while the player is in bullet-time
  / slow-motion-affected state (`CTSStunt::GoBulletTime`,
  `CBasePlayer::SlowMotionFlow` / `DeactivateSlowMotionFlow`,
  `CTSGameRules::SetGlobalSlowMotion` — all present in
  signatures-gcc2.txt) and cleared when it ends. **VERIFY** which exact
  member/conditions set the byte. Careful: our `m_bSlowBullets` (0x175 in
  player.h:404, "0=slowmo 1=slowpause") is documented as the slow-BULLETS
  variant flag consumed by CBulletManager — do not conflate with the sent
  byte without the decompile.

## CLIENT HANDLER BEHAVIOR

`__thiscall` on the CTSHud object (0x100d43cc):
1. `BEGIN_READ(pbuf, iSize)`
2. `READ_BYTE()`
3. store `byte != 0` into the client-global bullet-time bool (DAT_1018d178)
4. `m_iFlags |= HUD_ACTIVE` on the hud element

No other side effects.

## OURS

Status: **partial scaffolding, message entirely unimplemented.**
- Server: `dlls/player.h:414` declares `m_iClientBulletTime = -1` (the
  change-detect cache, never used); `dlls/ts_player.cpp:24` carries the
  SLICE comment "gmsgTSBulletTime 'TSBTime' ... deferred". No
  `REG_USER_MSG("TSBTime", ...)`, no send anywhere in `dlls/`.
- Client: no HOOK_MESSAGE/MsgFunc for TSBTime, no global flag. The consumer
  exists and waits: `cl_dll/view.cpp:TS_AddViewRecoil(..., bool
  bBulletTime)` implements the 0.66667 scaling, but the only call site
  `cl_dll/ev_ts.cpp:346` hardcodes `false` with a VERIFY comment ("no
  client slow-mo flag wired yet").
- Not a rename case: the audit's invented globals are unrelated —
  `gmsgStatusText`/`gmsgStatusValue` are stock HL messages the original
  also registers (API_SURFACE.md:63), and `gmsgWeapons` ("Weapons", 8,
  `dlls/UserMessages.cpp:71`) is an SDK-leftover weapon-list message, not a
  bullet-time substitute. Nothing of ours carries this payload under
  another name.

## RECONCILE

Server (dlls/ts_player.cpp, inside `TSUpdateClientData()` alongside
TSHealth/TSArmor/TSCash — matching the original's UpdateClientData home):
1. Lazily register: `static int gmsgTSBulletTime = 0; if (!gmsgTSBulletTime)
   gmsgTSBulletTime = REG_USER_MSG("TSBTime", 1);` — exact wire name
   `TSBTime`, size 1.
2. Compute the current flag (VERIFY exact source member against the
   re-project decompile of UpdateClientData; candidate: the player
   bullet-time byte @0x175 / the in-slow-motion state), change-detect
   against the existing `m_iClientBulletTime`, and on change:
   `MESSAGE_BEGIN(MSG_ONE, gmsgTSBulletTime, NULL, ENT(pev));
   WRITE_BYTE(flag); MESSAGE_END();` then update the cache.
3. Reset `m_iClientBulletTime = -1` wherever the sibling client-caches
   reset (respawn/ResetHUD path) so the flag re-sends after respawn/map
   change (map-change-state-reset memory).

Client (cl_dll):
1. Hook on the TS hud element that owns the sibling TS state messages
   (pattern: `DECLARE_MESSAGE(m_TSxxx, TSBTime)` + `HOOK_MESSAGE(TSBTime)`
   in Init — same shape as `CHudTSPowerUp::MsgFunc_TSSlowMo` in
   `cl_dll/ts_powerup_hud.cpp`; original `this` is CTSHud, so our umbrella
   TS hud element is the faithful home).
2. Handler: `BEGIN_READ(pbuf, iSize); g_bBulletTime = READ_BYTE() != 0;
   m_iFlags |= HUD_ACTIVE; return true;` with a new client global
   `bool g_bBulletTime` (mirror of DAT_1018d178).
3. Absorb into the existing consumer: change `cl_dll/ev_ts.cpp:346` to pass
   `g_bBulletTime` instead of the hardcoded `false` in `TS_AddViewRecoil`
   — closes the documented VERIFY there and completes FUN_10008230 parity.
   Later, audit whether DAT_1018d178 has additional client readers (render
   overlay) in the clientdll project.
