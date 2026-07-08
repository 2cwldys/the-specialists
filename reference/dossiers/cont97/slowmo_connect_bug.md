# cont.97 — slow-mo tint + motion-blur "shadows" on connect/spectate (IN PROGRESS — server-side, unfinished)

## Symptom (user, ORIGINAL client hl.exe vs our rig server)
Joining / RE-connecting into the press-fire-to-play **spectate** state shows:
- the teal/desaturated **slow-motion screen tint** over the whole view, AND
- accumulating motion-blur **"shadows"** — ghost/snapshot copies of players ("like it's snapshotting the
  player and saving it as a new ent") — plus frozen muzzle-flash tempents piling up.
- **Resets to normal when you press fire and spawn.**
- **Happens EVEN WITH NO PLAYERS on the server** (the user isolated this) — so it is NOT SlowMotionFlow
  (nobody is using a slow powerup to slow anyone).

## Critical framing
The user tests with the **ORIGINAL client (hl.exe)**, which is FAITHFUL. So the client-side fix I shipped
this session (`538142a`, resetting `g_flClientSlow` in our cl_dll `CHudTSPowerUp::VidInit`) **only helps the
Xash/our-cl_dll client — it does NOT touch the original client.** The persisting bug on the original client
means **OUR SERVER is sending the original client a stale/slow value it shouldn't** (or failing to send a
reset). **This is a SERVER-side bug. Do the RE on the server + the original client's message pipeline, not
our cl_dll.**

## RE done this session (verified)
- **Client slow globals (client.dll):** target `DAT_1009ff04`, current `DAT_100963e8`.
  - `MsgFunc_TSSlowMo` = `0x10049a00` → `FUN_1004bef0(&DAT_100d43cc, …)`: `DAT_1009ff04 = msgfloat * scale`
    (normalizes to 1.0 when the value == the neutral). goslow.wav plays on the normal→slow edge.
  - **`CHud::VidInit` = `0x100206c0`** resets the CURRENT `DAT_100963e8 = 1.0` at `0x100208a9` on
    connect/level-change — but does **NOT** reset the TARGET `DAT_1009ff04`. So the target is message-driven
    and persists across a reconnect.
  - The screen **tint** AND the **blur ghost spawner** both gate on the slow CURRENT:
    blur spawner `FUN_10029780` (called from HUD_CreateEntities) spawns a ghost only while
    `DAT_100963e8 <= threshold(_DAT_1007d4e8)`. Blur array = `DAT_100dff48` (stride 0x18). Our equivalents:
    tint `cl_dll/view.cpp:~1959` (`g_flClientSlow < 1.0`), blur `cl_dll/ts_motionblur.cpp:~127`
    (`g_flClientSlow > 0.99` → clear), spawner from `entity.cpp` HUD_CreateEntities.
- **Original `SlowMotionFlow` = `0x86bb0` (ghidra `0x96bb0`, TS_player.cpp):** the LOS-victim loop checks
  `UTIL_PlayerByIndex(i)->IsPlayer()` and LOS — **NO spectator/observer gate.** ⇒ **slowing spectators IS
  original behaviour.** (I tried a spectator-skip; the user rejected it; I reverted it. Our `SlowMotionFlow`
  in `dlls/ts_player.cpp` now matches the original — no gate. KEEP it that way.)

## Why the lone-spectator (no players) case still tints (the OPEN root)
No players ⇒ nobody runs SlowMotionFlow ⇒ nothing sets `fuser1<1` on the spectator legitimately. Yet the
ORIGINAL client tints on reconnect. The client resets the CURRENT in VidInit but NOT the TARGET
(`DAT_1009ff04`). So a **stale target from the previous session** (if the player disconnected while slowed —
or any stale value) persists on the client, and the original client eases the current toward it → tint +
(if blur active) ghosts. **The server is responsible for pushing a fresh `TSSlowMo`(normal) on connect to
reset the client's target — and ours evidently isn't.**

Our server send (`dlls/player.cpp:~4713`, inside `CBasePlayer::UpdateClientData` = the TS block of UCD
@0x81d00): `slowAmt = pev->fuser1 * 100` (with an activator override), sent **change-detected**:
`if (slowAmt != m_flClientSlowAmt) { m_flClientSlowAmt = slowAmt; MSG_ONE gmsgTSSlowAmt WRITE_COORD(slowAmt); }`.
`m_flClientSlowAmt` init `-1` (`dlls/player.h:419`, `// 0x16c`), reset to `-1` at `dlls/ts_player.cpp:547`.

## NEXT STEPS (do the RE, don't guess — the user's directive)
1. **Does `UpdateClientData` (the gmsgTSSlowAmt send) even run for a SPECTATING player?** If UCD is skipped
   in observer mode, the reset never sends. RE the original UCD @0x81d00 gating + our `CBasePlayer::UpdateClientData`.
2. **Does the ORIGINAL server force-send `TSSlowMo`(normal) on connect / spawn / StartObserver?** RE:
   - `StartObserver` = `0x89634` (ghidra `0x99634`) — sets `fuser1=1.0` server-side; does it also send/trigger
     the client slow reset? (Decompile already archived earlier; re-check the TSSlowMo/SlowAmt sends in it.)
   - The original's gmsgTSSlowMo (server "TSSlowMo" reg) send site(s) + change-detect member — find where the
     original resets its send-cache on connect/spawn so the reset re-fires. `binmap.py <name>` → mp.dll RVA →
     `live.py break` on the running original to watch the actual send on a real reconnect.
3. **Verify `fuser1` is actually 1.0 for a lone spectator** on our server (entvars default 0 → corrected in
   `player.cpp:~2179` `if (pev->fuser1==0) pev->fuser1=1.0`, then `InterpolateSlowMotion(frametime,&fuser1,m_flSlowMotionGo)`).
   If `m_flSlowMotionGo` is stale <1 on a spectator, fuser1 eases down and the server sends a real slow — that
   would be a server bug (StartObserver sets m_flSlowMotionGo=1.0, so check what re-lowers it).
4. **Likely fix (confirm against the RE first):** force a `gmsgTSSlowAmt`(100/normal) to the joiner on
   connect/spawn/StartObserver (e.g. set `m_flClientSlowAmt = -1` there so the next UCD send re-fires with the
   real fuser1), **matching whatever the original does to reset the client's slow target on connect.** Do NOT
   invent — find the original's mechanism (it MUST have one, since the original client doesn't self-reset the
   target).
5. The **accumulation** (ghosts/muzzle-flashes) on a FULL server during real slow-mo may be partly original
   (frozen time → tempents don't expire). Separate concern; confirm vs the original before "fixing" — the
   original client uses the same blur pipeline. Focus on the no-players stale-target case first (clearly a bug).

## Tooling note
`re.sh`/`dc.sh` are blocked while the Ghidra **MCP** or GUI holds the `re`/`clientdll` project `.lock`. This
session I had the MCP connected to `re` (client.dll + ts_i386.so open) which blocked `re.sh 0x96bb0` until the
user freed it. Disconnect the MCP / close the GUI program before headless runs, or use the MCP `decompile_function`
directly. Client decompiles: `dc.sh client 0x<rva>`. Server: `re.sh <Class|0xghidraaddr>` (ghidra = nm+0x10000).
