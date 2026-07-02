# gmsgTSPlayerInfo — wire name "PlInfo" (size 3)

Server-side global: `gmsgTSPlayerInfo` @ `ts_i386.so` .data `0x1034d4` (nm-visible).
Wire-name string `"PlInfo"` @ .so offset `0xfe590`; trigger string `"lasclr_"` @ `0xfb686`.
Client handler: `client.dll` `FUN_1004b7e0` (size 0x51), forwarder `0x10049b40`,
`this` = CTSHud instance `0x100d43cc`.

## WIRE FORMAT (byte-faithful swap contract)

Registered `REG_USER_MSG("PlInfo", 3)` — fixed size 3.

| # | Type | Field | Notes |
|---|------|-------|-------|
| 1 | BYTE | playerIndex | ENTINDEX(player); client accepts only `0 < idx < 0x41` (1..64) |
| 2 | BYTE | infoType | only `0` (laser color) is handled by the client; anything else read then ignored |
| 3 | BYTE | value | laser color index (client palette: 1=green, 2=blue, else red) |

Direction: server -> all clients (MSG_ALL).

## PURPOSE

Broadcasts one player's **laser-sight color** so every client renders that
player's laser beam/dot in the color he chose. It is the network half of the
`cl_lasercolor` cvar: the owner's client tells the server via the console
command `lasclr_<n>`, the server rebroadcasts to everyone as PlInfo, and each
client stores it in a per-player table consulted when drawing OTHER players'
lasers (the local player uses his own cvar directly). The generic `infoType`
byte suggests a "player info" envelope, but type 0 is the only type either
side ever uses.

## CLIENT SEND SIDE (the trigger, RE'd from client.dll)

- Cvar `cl_lasercolor`, default `"10"` (registered near `0x100270b2`; cvar_t*
  cached at `0x100df44c`).
- Per-frame CTSHud method at `0x1004ce10`:
  `int v = (int)cl_lasercolor->value; if (v != this->m_iLastLaserColor /* +0x3728 */)
  { m_iLastLaserColor = v; sprintf(buf, "lasclr_%i", v); ServerCmd(buf); }`
  (`"lasclr_%i"` string @ `0x10095e08`; call through engine-table slot
  `0x100d5b74` = pfnServerCmd). So the client sends `lasclr_<n>` **on change**
  (and once at startup since the cache starts != cvar).

## SERVER SEND SITES (single site; specifics VERIFY on the re-project pass)

- `ClientCommand` (original dlls/client.cpp), site ~`0x9dea8` in `ts_i386.so`:
  `strstr(cmd, "lasclr_") == cmd` -> `val = atoi(cmd + 7)` ->
  `MESSAGE_BEGIN(MSG_ALL, gmsgTSPlayerInfo); WRITE_BYTE(ENTINDEX(pEntity));
  WRITE_BYTE(0); WRITE_BYTE(val); MESSAGE_END();`
- VERIFY (needs the typed re-project decompile, unavailable this pass):
  - exact atoi/clamping (client handler stores raw char, so probably no clamp);
  - MSG_ALL vs MSG_BROADCAST (reliable vs unreliable);
  - REG_USER_MSG("PlInfo", 3) registration site;
  - NO late-joiner resend is apparent (only one send site) — a client
    connecting after the broadcast sees default red until that player
    re-sends `lasclr_`. The client-side table is zeroed by `FUN_10056b60`
    (caller VERIFY, presumably InitHUD / map change) — whether the owner's
    client re-sends after a map change depends on whether m_iLastLaserColor
    (+0x3728) is reset there (VERIFY; cf. map-change-state-reset memory).

## CLIENT HANDLER BEHAVIOR (ground truth: FUN_1004b7e0 decompile)

    BEGIN_READ(pbuf, iSize);
    idx  = READ_BYTE();
    type = READ_BYTE();
    val  = READ_BYTE();
    if (idx > 0 && idx < 0x41 && type == 0)
        DAT_1018d128[idx*2 + 1] = (char)val;   // per-player laser color
    this->m_iFlags |= 1;                        // HUD_ACTIVE on CTSHud (+0x10)

Backing store `DAT_1018d128` = per-player **2-byte** record array (index idx*2),
shared with the `ActItems` message:
- `[idx*2 + 0]` = active-attachment mask — written by the ActItems handler
  (`mov %al,0x1018d128(,%esi,2)` @ `0x1004fa9b`), tested @ `0x1003b815`
  (`testb $0x4`) and `0x1003d780` (variable bit), zeroed per-player @ `0x1006530e`.
- `[idx*2 + 1]` = laser color — written ONLY by this handler
  (`mov %al,0x1018d129(,%esi,2)` @ `0x1004b817`).
- Whole array zeroed (33 entries) by `FUN_10056b60` (reset/init; caller VERIFY).
- The color-byte READ in the laser-draw path doesn't appear as a scaled-index
  instruction (likely register-computed address inside the beam-draw fn) —
  exact consumer VERIFY, but semantics are pinned by the palette fn
  `FUN_100582f0` (1=green, 2=blue, else red), already RE'd in our ts_attachments.

## OURS

**Status: partial — surrounding machinery exists, the message itself is absent.**

- `grep -r "PlInfo|TSPlayerInfo|lasclr"` over dlls/ + cl_dll/: ZERO hits.
  No REG_USER_MSG, no ClientCommand branch, no client hook, no lasclr sender.
- NOT one of the invented `gmsgStatusText`/`gmsgStatusValue`/`gmsgWeapons`
  globals — those are stock-HL leftovers, unrelated; nothing of ours is a
  renamed substitute. Pure gap, already self-documented:
  `cl_dll/ts_attachments.cpp:285` — "local laser uses cl_lasercolor; others
  default red (their colour byte is a SLICE)".
- What we DO have (absorbers):
  - `cl_dll/ts_attachments.cpp`: `g_playerAttach[TS_MAXPLAYERS]` = the even
    byte of the original pair (`MsgFunc_ActItems` mirrors `0x1004fa70`);
    `GetLaserColor(int,...)` = the original palette; the other-players beam
    loop (~line 273) currently hardcodes color 0 for non-local players.
  - `cl_dll/ts_hud.cpp:52`: `CVAR_CREATE("cl_lasercolor", "10", FCVAR_ARCHIVE)`
    — name AND default match the original.
  - `dlls/client.cpp`: ClientCommand already hosts prefix-match branches
    (`weapon_` @579/721, `cutwire_` @682) — pattern for the `lasclr_` branch.

## RECONCILE

1. **Server** (dlls/): add global `int gmsgTSPlayerInfo;` (original name) and
   `gmsgTSPlayerInfo = REG_USER_MSG("PlInfo", 3);` at the registration site.
   In `ClientCommand` add the prefix branch in the original's strstr idiom:
   `else if ((pstr = strstr(pcmd, "lasclr_")) != NULL && pstr == pcmd) {
      MESSAGE_BEGIN(MSG_ALL, gmsgTSPlayerInfo);
      WRITE_BYTE(ENTINDEX(pEntity)); WRITE_BYTE(0);
      WRITE_BYTE(atoi(pcmd + 7)); MESSAGE_END(); }`
   No clamp (original stores the raw byte; exact arg handling VERIFY).
2. **Client hook** (`cl_dll/ts_attachments.cpp` — it already owns the paired
   even byte): add `static unsigned char g_playerLaserColor[TS_MAXPLAYERS];`
   next to `g_playerAttach`; `gEngfuncs.pfnHookUserMsg("PlInfo", MsgFunc_PlInfo)`
   in `TS_Attachments_Init()`. Handler = the decompile exactly: 3 READ_BYTEs,
   store only if idx in [1,64] (+ our array bound) && type==0.
3. **Client draw**: in the beam loop (~ts_attachments.cpp:287) replace
   `GetLaserColor(isLocal ? (int)cl_lasercolor : 0, ...)` with
   `GetLaserColor(isLocal ? (int)cl_lasercolor : g_playerLaserColor[i], ...)`;
   delete the SLICE comment.
4. **Client sender**: per-frame CTSHud think (`ts_hud.cpp`): cache
   `m_iLastLaserColor`; when `(int)cl_lasercolor->value` differs, send
   `sprintf(buf, "lasclr_%i", v); gEngfuncs.pfnServerCmd(buf);` — ServerCmd,
   NOT ClientCmd (engine-table slot 0x100d5b74 = pfnServerCmd).
5. **Reset**: zero `g_playerLaserColor` wherever `g_playerAttach` is reset
   (mirrors `FUN_10056b60` zeroing both bytes). Do NOT invent a late-joiner
   resend the original lacks (drop-in rule) unless the re-project VERIFY pass
   finds one.
