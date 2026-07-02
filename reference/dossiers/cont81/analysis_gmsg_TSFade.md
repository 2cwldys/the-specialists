# gmsgTSFade — "TSFade" (slow-mo-aware screen fade)

## WIRE FORMAT

Registered `REG_USER_MSG("TSFade", -1)` (variable) but the payload is **always 10
bytes** — the stock HL `ScreenFade` wire layout, byte-for-byte:

| # | Type  | Size | Field     | Notes |
|---|-------|------|-----------|-------|
| 1 | SHORT | 2    | duration  | 4.12 fixed (`FixedUnsigned16(fadeTime, 1<<12)`), **pre-divided server-side by target's `GiveSlowMul()`** |
| 2 | SHORT | 2    | holdTime  | 4.12 fixed, same slow-mo division |
| 3 | SHORT | 2    | fadeFlags | FFADE_* (IN/OUT/MODULATE/STAYOUT) |
| 4 | BYTE  | 1    | r         | |
| 5 | BYTE  | 1    | g         | |
| 6 | BYTE  | 1    | b         | |
| 7 | BYTE  | 1    | a         | |

Total = 10 bytes. Client decompile (`FUN_1004c1e0` @0x1004c1e0, forwarder
0x10049b60, this = CTSHud 0x100d43cc) reads exactly READ_SHORT x3 then
READ_BYTE x4 in that order — matches the stock `UTIL_ScreenFadeWrite` write
order (duration, holdTime, fadeFlags, r, g, b, a).

Cross-refs: `reference/HUD_MESSAGES.md:21` (`TSFade | -1 | gmsgTSFade`),
`reference/API_SURFACE.md:50`. `rz-bin -zz ts_i386.so` confirms the strings
`TSFade` (.rodata 0xfe493) and `gmsgTSFade` (symbol).

## PURPOSE

TS's replacement for the stock engine-handled `ScreenFade` message. GoldSrc
handles a user message named "ScreenFade" inside the ENGINE (the client dll
never hooks it), so TS could not adjust fade timing for its per-player
slow-motion there. TSFade is the same fade payload under a mod-owned name so
the CLIENT DLL receives it, and the SERVER pre-scales the two time fields by
the recipient's time-dilation factor (`GiveSlowMul()`, CBaseEntity vtable
+0x10 — confirmed `reference/coverage/VTABLES.md:575`), so a fade lasts the
right amount of game-time for a player currently in slow motion (dividing by
a mul < 1 lengthens the wall-clock fade).

## SERVER SEND SITES (+VERIFY flags)

- Sender: `UTIL_ScreenFadeWrite(ScreenFade const&, CBaseEntity*)` @0xdcc54 in
  util.cpp (`reference/signatures-gcc2.txt:1684`). Per scout intel:
  `MSG_ONE` on `gmsgTSFade`, gated on `ent->IsNetClient()`;
  duration and holdTime each divided by the target entity's
  `vtable[+0x10]` return = `GiveSlowMul()` before `FixedUnsigned16`.
  - **VERIFY (re-project pass):** whether the division happens on the float
    fadeTime before FixedUnsigned16 inside Write (i.e. Write receives floats /
    re-derives) or divides the already-fixed shorts; and whether
    `UTIL_ScreenFadeBuild` still exists separately in the original.
- Upstream callers = every stock/TS `UTIL_ScreenFade` / `UTIL_ScreenFadeAll`
  call site: `env_fade` (CFade in effects.cpp), flashbang/concussion grenade
  white flash, player death fade, etc.
  - **VERIFY:** exact TS call-site list via `re.sh refs UTIL_ScreenFadeWrite`
    once the Ghidra project is free.
- Registration: the original registers BOTH stock `"ScreenFade"` (size 10, in
  the stock block — see HUD_MESSAGES.md registration list) and `"TSFade"`
  (size -1) in LinkUserMessages; the `gmsgTSFade` int global is defined in
  player.cpp (per AUDIT). Stock ScreenFade is registered but the sends go
  through TSFade. **VERIFY:** whether ANY original code path still emits the
  stock ScreenFade (assume none).
- Per-player scaling implies `UTIL_ScreenFadeAll` must compute the wire shorts
  PER RECIPIENT (each player has their own GiveSlowMul) — consistent with the
  division living in Write (per-entity), not Build. VERIFY in decompile.

## CLIENT HANDLER BEHAVIOR (ground truth: gmsg_TSFade.c)

`FUN_1004c1e0(pszName, iSize, pbuf)`:
1. `BEGIN_READ(pbuf, iSize)` (FUN_100299b0).
2. READ_SHORT -> `DAT_101914a8[0..1]` (duration), READ_SHORT ->
   `DAT_101914a8[2..3]` (holdTime), READ_SHORT -> `DAT_101914ac` (fadeFlags).
3. READ_BYTE x4 -> `DAT_101914ae/af/b0/b1` (r, g, b, a).
4. `_DAT_10190b5c = DAT_100cf944;` — latches the current client time into a
   "fade received at" global (DAT_100cf944 ~ gHUD.m_flTime / cached cl time).
5. `return 1`.

So the handler only RECORDS a 10-byte ScreenFade struct at 0x101914a8 plus a
start-time at 0x10190b5c; the visual application (decoding 4.12 fixed back to
seconds and driving the actual fade — most plausibly via
`gEngfuncs.pfnSetScreenFade`/V_CalcRefdef or a CTSHud draw) happens elsewhere.
**VERIFY:** find the reader of 0x101914a8/0x10190b5c in client.dll (dc.sh
xrefs) to pin the application mechanism and the exact fixed->float decode.

## OURS

- **Server:** `dlls/UserMessages.cpp:60` registers only stock
  `gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade))`. No "TSFade"
  registration anywhere. `dlls/util.cpp:758 UTIL_ScreenFadeWrite` sends
  `MSG_ONE, gmsgFade` with the correct 10-byte layout but NO slow-mo division
  (`UTIL_ScreenFadeBuild` at util.cpp:746 does plain FixedUnsigned16).
  Callers exist and work: ts_grenades.cpp:257 (flashbang white flash),
  effects.cpp:1974/1979 (env_fade), player.cpp:5325, gauss.cpp:274.
  `CBaseEntity::GiveSlowMul()` already exists (`dlls/cbase.h:181`,
  `= FixedSlowAmt(pev->fuser1)`), so the divisor is available.
- **Client:** no `MsgFunc_TSFade`, no hook. Fades currently work only because
  we send the engine-handled stock "ScreenFade". `cl_dll/health.cpp:211` has a
  placeholder comment ("until TS's own (PTakeDam/TSFade) is reconstructed").
  `cl_dll/view.cpp:~1849` already drives `pfnGet/SetScreenFade` for the
  slow-mo tint — a natural neighbor, not a substitute.
- The audit's invented globals `gmsgStatusText`/`gmsgStatusValue`/`gmsgWeapons`
  (UserMessages.cpp:68-71) are NOT TSFade substitutes (status bar / HL weapon
  bits); TSFade is simply **absent**, with stock ScreenFade acting as a
  functional (but not drop-in) stand-in.
- Net status: **partial** — fades render, but on the wrong wire name (engine
  path) with no slow-mo scaling; an original client connected to our server
  gets engine fades but never TSFade (drift under slow-mo); our client on an
  original server gets NO fades at all (we do not hook TSFade).

## RECONCILE

Server side (dlls/):
1. Add `int gmsgTSFade = 0;` defined in **player.cpp** (match original's
   defining file for audit parity; extern in UserMessages.h).
2. In LinkUserMessages, register `gmsgTSFade = REG_USER_MSG("TSFade", -1);`
   in the original's registration ORDER (first message of the TS block, right
   after TeamNames — see HUD_MESSAGES.md). KEEP the stock
   `REG_USER_MSG("ScreenFade", sizeof(ScreenFade))` — the original registers
   both.
3. Rework `UTIL_ScreenFadeWrite(const ScreenFade&, CBaseEntity*)` to send on
   `gmsgTSFade` (same 7 writes, same order), dividing duration and holdTime by
   `pEntity->GiveSlowMul()` before FixedUnsigned16. Because the division is
   per-recipient, move the time-field fixing into the Write path (Build keeps
   flags/rgb; or pass the float times through) so `UTIL_ScreenFadeAll` scales
   per player. Keep the `IsNetClient()` gate. Pin exact float-vs-fixed
   division placement from the @0xdcc54 decompile in the re-project pass.
4. No caller changes — all existing UTIL_ScreenFade/All call sites absorb it.

Client side (cl_dll/):
5. Add `MsgFunc_TSFade(const char*, int, void*)` (on CHud, mirroring the
   original CTSHud forwarder): READ_SHORT duration, READ_SHORT holdTime,
   READ_SHORT fadeFlags, READ_BYTE r,g,b,a; store + latch receive time;
   `HOOK_MESSAGE(TSFade)` in CHud::Init (hud.cpp).
6. Apply the fade from the stored record: decode 4.12 fixed -> seconds and set
   the engine fade via `gEngfuncs.pfnSetScreenFade` (screenfade_t: fadeSpeed/
   fadeEnd/fadeReset from duration/holdTime + receive time, fader/g/b/alpha,
   fadeFlags) — the same mechanism view.cpp's slow-mo tint already uses; take
   care not to fight that tint (it rewrites the fade each frame). VERIFY the
   original's application site before finalizing (see CLIENT HANDLER VERIFY).
7. Retire the health.cpp:211 placeholder comment when done.

Result: byte-identical 10-byte TSFade payload under the original wire name,
slow-mo-aware timing, drop-in in both directions.
