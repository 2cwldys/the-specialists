# TS predicted weapon events (.sc) — the complete RE map (cont.69r)

The `events/*.sc` files are **empty 4-byte hooks** (`//\n`) — no data. They are the
named handle that ties a SERVER `PLAYBACK_EVENT` to a CLIENT `pfnHookEvent` handler
that runs the predicted viewmodel anim / sound / sprite. We wire the events to our
code (not read the files); values come from the event args + the RE, not the `.sc`.

Currently only **fire.sc** is connected (`m_usFire` → `EV_TSFire` in `cl_dll/ev_ts.cpp`).
Everything else is driven server-side (SendWeaponAnim + EMIT_SOUND) — works but is not
client-predicted. Connecting them = the faithful path.

## The map (both sides)

| event | server handle (CTSGun+) | played by (server) | client handler (client.dll) |
|---|---|---|---|
| `fire.sc`        | `+0x964` | StandardFire/ShotgunFire | `FUN_1003de40` |
| `coldcock.sc`    | `+0x966` | ColdCock/AlternateAttack | `LAB_10043800` |
| `empty.sc`       | `+0x968` | EmptySound (dry trigger) | `LAB_100424e0` |
| `pump.sc`        | `+0x96a` | ShotgunReload (per shell) | `LAB_100419a0` |
| `closecombat.sc` | `+0x96c` | CCombatAttack / CCombatBlock | `FUN_10043200` |
| `firelinked.sc`  | `+0x96e` | AkimboFire (dual barrel) | `FUN_1003de10` |
| `reload.sc`      | `+0x970` | Reload/ReloadWeapon | `LAB_10042690` |
| `knife.sc`       | `+0x972` | KnifeFire | `LAB_10043c90` |
| `katana.sc`      | `+0x974` | KatanaFire | `LAB_10044230` |
| `draw.sc`        | `+0x976` | Deploy | `FUN_100425b0` |
| `grenade.sc`     | `+0x978` | GrenadeFire | `FUN_10042630` |

- Server hook addrs are Ghidra (= nm + 0x10000); client addrs are client.dll Ghidra.
- Client hook registrar = `FUN_10003240` (calls `pfnHookEvent` = `DAT_100d5c3c`).
- `PRECACHE_EVENT` = server `g_engfuncs+0x1e8`; `PLAYBACK_EVENT_FULL` = `g_engfuncs+0x1ec`.

## cont.69s — decompiled the handler architecture (the real contract)

RE'd the client handlers (client.dll, Ghidra). The architecture, and what it means for a
**drop-in-faithful** (not byte-faithful) reconstruction:

- **Each handler caches its event args at entry**, then reproduces the predicted view. The
  draw handler `FUN_100425b0` and the reload handler `FUN_10042690` both do, verbatim:
  `active_type = iparam1 (args+0x38)`, `active_info = get_weapon_info(active_type)`,
  `barrel_mask = iparam2 (args+0x3c)`, `entindex = args+4`, and copy origin/angles/velocity
  (args+8 / +0x14 / +0x20). The reload handler additionally reads `stage = ftol(fparam1)` and
  branches the sound/effect on it (0 = generic reload + eject clip, 9 = per-shell insert
  `weapons/insert_shell.wav`, 10 = per-weapon reload sound at `weapon_info+0x306`; shotgun/
  bull/contender (types 0x21/0x1f/0x24) eject shell models).
- **The fire handler `FUN_1003de40` dispatches on the fire-type byte `weapon_info+0xd2`**
  (== our `s_weapon_info::iFireType`): `1` → pump-shotgun fire `FUN_10040c20`, `4`/`5` → akimbo
  fire `FUN_1003f090`, else → standard fire `FUN_1003dfa0`. So the **weapon type travels in the
  event** (iparam1); the handler re-derives everything from `get_weapon_info(iparam1)`.
- The `DAT_10189xxx` globals (~70 of them in the standard-fire handler) are the client's
  **internal** prediction state (cached vectors, per-shot counters, the active weapon_info ptr
  `DAT_10189ae4`, active type `DAT_10189d0c`). They are NOT part of the protocol.

**The contract (what MUST match for drop-in) = the event NAME + the iparam/fparam/bparam
packing.** The ~70 internal globals are ours to model however we like (CLAUDE.md: behaviour +
interface fidelity, NOT byte/offset). So step 2's "param cache" is a **clean modern
active-weapon struct** (type/info/barrelMask/entindex/origin/angles/velocity), refreshed by
every TS event handler at entry — mirroring the original's per-handler cache-fill, but without
the byte-level global soup.

### Per-event arg packing (the protocol, RE'd from the handlers)
| event | iparam1 | iparam2 | fparam1 | bparam1 |
|---|---|---|---|---|
| draw.sc  | tsweapon_t | barrel mask | — | — |
| fire.sc  | tsweapon_t | barrel side/mask | — | empty? |
| reload.sc| tsweapon_t | barrel mask | reload stage (0/9/10) | — |
(origin/angles/velocity are filled by PLAYBACK_EVENT_FULL from the player edict — pass g_vecZero.)

## cont.69s — ✅ RESOLVED: CTSGun is now CLIENT-PREDICTED, the viewmodel anims STICK

The "anim doesn't stick" problem (documented below as the original blocker) is FIXED. CTSGun was made
a real client-predicted weapon: ts_gun.cpp compiles into client.so (3a); the class is in ts_gun.h;
CTSGun::GetWeaponData/SetWeaponData ship the TS active state (type/reserve/fire-mode) the generic
weapon_data loop doesn't (3c); g_TSGun is registered in HUD_InitClientWeapons + the HUD_WeaponsPostThink
TSGUN_WEAPON_ID case (3b); UseDecrement() returns true. **The key fix (3d) was the TIME BASE:** CTSGun's
attack/idle/reload timers were on absolute gpGlobals->time (the cont.69q workaround); with UseDecrement
the engine + the server's CBasePlayer::ItemPostFrame decrement loop treat them as a 0-based RELATIVE
countdown, so absolute values corrupted. Reverted them all to UTIL_WeaponTimeBase() (0 under the global
CLIENT_WEAPONS, decremented on both sides). The fire/muzzle events now play with FEV_NOTHOST so the
predicting host plays its own predicted copy and isn't double-sent the server copy.

**Verified in-engine (Con_Printf-instrumented, then cleaned up):** `cl: [TSDIAG] StandardFire()` and
`cl: [TSDIAG] Reload()` both fire (client prediction runs the weapon code); the predicted clip
decrements; the fire (slide) and reload viewmodel anims now ANIMATE (frames differ — were
pixel-identical/static before); a SINGLE muzzle flash (FEV_NOTHOST dedup, no doubling); server fires
repeatedly (no fire-once-stuck regression — the server decrement loop handles the 0-base now that
UseDecrement is true). No crash.

The remaining 11-handler port (3e) is now OPTIONAL polish: the viewmodel ANIMS already predict via
SendWeaponAnim in the predicted ItemPostFrame, so the `.sc` events are only needed for predicted
EFFECTS (muzzle sprite/shell/sound) — which EV_TSFire already does for fire. The other actions
(coldcock/empty/pump/closecombat/grenade/knife/katana) could get predicted-effect events too, but their
anims already stick. TODO if pursuing 3e: also test server-only (dedicated, no client prediction)
firing across weapons to confirm the time-base change holds there.

---
### (historical) The original blocker — anim-prediction was BLOCKED by the server-authoritative CTSGun

The original predicts the viewmodel anim in each handler (it `EV_WeaponAnimation`s the shoot/
reload seq) because the original CTSGun **is** client-predicted. **Ours is not** — our
reconstruction drives CTSGun server-authoritatively (ledger cont.69p), and the build defines
`CLIENT_WEAPONS`, so `HUD_WeaponsPostThink` runs the HL weapon-prediction loop every frame.
CTSGun is not registered in that loop, so the predicted viewmodel anim is **overwritten each
frame** and never shows.

**Proven in-engine (reload experiment, reverted):** replaced `SendWeaponAnim(iReloadAnim)` in
`CTSGun::Reload` with a predicted `events/reload.sc` → `EV_WeaponAnimation`. Result: the clip
**refilled** (server reload ran) but the reload viewmodel anim **never played** across ~1.8 s of
capture. The fire event's local effects (muzzle flash / punch) DO work, so `EV_IsLocal` is fine —
it's specifically `EV_WeaponAnimation` that doesn't stick. (This is exactly what the `EV_TSFire`
comment already noted for the fire anim.)

**Consequences for the .sc port:**
- Predicted events CAN faithfully do **sounds + effects** (muzzle-flash sprite, shell eject, view
  punch, temp-entities) — `EV_TSFire` already does. These are worth porting (drop-in arg packing).
- Predicted events CANNOT do the **viewmodel anim** — that must stay `SendWeaponAnim` (server),
  which already triggers the model's embedded clipout/clipin/reload sound events.
- The fully faithful predicted path (anims included) requires **making CTSGun client-predicted**
  first: a client-side weapon object registered in `HUD_InitClientWeapons`/`HUD_WeaponsPostThink`
  (`HUD_GetWeaponData`), mirroring CTSGun's clip/timers/state — a separate, larger effort. **This
  is the real gate** on the "FULL byte-faithful prediction" goal. Decision needed (see ledger).

## cont.69s — step 3b/3c: CTSGun client-prediction SCAFFOLDED; enabling it surfaced bugs to debug

Built the harness to make CTSGun a client-predicted weapon (so SendWeaponAnim predicts and the anim
sticks). Done + committed as inert scaffolding (DISABLED — no runtime change):
- `dlls/ts_gun.h` — extracted the CTSGun class so the client harness can instantiate it.
- `CTSGun::GetWeaponData`/`SetWeaponData` (3c) — ship the TS active state the prediction needs that
  the generic weapon_data loop doesn't: weapon TYPE (iuser1), active-caliber RESERVE (iuser2),
  fire-mode (iuser3). (m_iClip + timers + m_fInReload are shipped generically.)
- `CTSGun::UseDecrement()` — gates GetWeaponData packing + client prediction. Currently `false`.
- `cl_dll/hl/hl_weapons.cpp` — `CTSGun g_TSGun;` + the registration (HUD_PrepEntity + the
  `case TSGUN_WEAPON_ID` in HUD_WeaponsPostThink) are present but COMMENTED OUT.

**Enabling it (UseDecrement→true + uncomment the registration) was tested in-engine and is NOT yet
correct — DO NOT ship enabled:**
- The predicted viewmodel anim STILL did not stick — firing + reloading kept the idle pose (reload
  frames were pixel-identical; the fire frame showed the idle pose while the muzzle/impacts played).
- Fire effects appeared DOUBLED (muzzle/shells) — the fire event plays from both the client
  prediction and the server without dedup.
- No crash (EMIT_SOUND is a client no-op; messages aren't on the prediction path; FireBulletsPlayer
  is client-safe).

**ROOT CAUSE FOUND (cont.69s, Con_Printf-instrumented) — a TIME-BASE mismatch.** The diagnostic
(logged in HUD_WeaponsPostThink) confirmed the good news first: `ItemPostFrame` DOES run client-side
for id 16, the gates pass, and `SetWeaponData` mirrors correctly (`wpnType=5` for the M4, reserve,
clip). So registration + state-shipping (3b/3c) are CORRECT. But:
- `m_flNextPrimaryAttack` was seen DECREASING frame-to-frame (37.92→37.85…) while client time rose —
  i.e. **the engine decrements a UseDecrement weapon's `m_flNextPrimaryAttack` as a RELATIVE
  countdown each client frame.** CTSGun sets it to ABSOLUTE `gpGlobals->time + delay`, so the
  decrement corrupts it → the predicted attack/reload gate misfires → `SendWeaponAnim` isn't driven
  client-side (`animBefore==animAfter==0`) → the viewmodel stays idle. THAT is "anim doesn't stick".
- **Why CTSGun uses absolute time:** `CLIENT_WEAPONS` is defined GLOBALLY in CMakeLists (both the
  `hl` and `client` targets — line ~62, applied to every target). That makes `UTIL_WeaponTimeBase()`
  return 0 on the SERVER too, so cont.69q switched CTSGun's timers to `gpGlobals->time` to avoid the
  "fire-once-stuck" (a 0 base the server never decrements). That workaround is incompatible with
  prediction.

**THE FIX (next task — wide blast radius on core firing timing, test across weapons):**
1. Scope `CLIENT_WEAPONS` to the **client** target only (CMakeLists `set_common_properties` →
   move it onto the client target), matching the stock HL SDK. Then `UTIL_WeaponTimeBase()` =
   `gpGlobals->time` server-side (CTSGun server firing unchanged from cont.69q) and `0` client-side
   (the engine's relative decrement is then correct).
2. Revert CTSGun's timers `gpGlobals->time` → `UTIL_WeaponTimeBase()` everywhere
   (m_flNextPrimaryAttack/Secondary, m_flTimeWeaponIdle, the reload delays, GetNextAttackDelay).
3. Re-enable prediction: `UseDecrement()` → the CLIENT_WEAPONS form + uncomment the 2 hl_weapons.cpp
   registration lines. Re-test (the anim should now stick).
4. Retest server-only firing (dedicated, no client prediction) — confirm cont.69q "fire/reload/
   stuck" fixes still hold with the time base now `gpGlobals->time` server-side.
5. EVENT DEDUP: once prediction is correct, play the fire/muzzle event excluding the host client
   (FEV_NOTHOST) so muzzle/shell effects aren't doubled.

Verified the disabled checkpoint is regression-free (prediction off; instrumentation removed).

## Per-event RE (fill in as each is decompiled + implemented)

- **fire.sc** (FUN_1003de40): muzzle-flash sprite at the muzzle attachment (the
  `FUN_100474b0` builder, ts_muzzleflash1h/6), fire sound, shell eject, viewmodel
  shoot anim, V_PunchAxis. iparam1 = tsweapon_t, iparam2 = barrel side (akimbo),
  bparam1 = empty. ✅ reconstructed (cl_dll/ev_ts.cpp EV_TSFire — sprite + sound +
  shell; muzzle multi-segment is approximated).

## ★ Chosen approach (cont.69r): reconstruct the FULL client weapon-param subsystem

The event handlers don't take their data from the event much — they read a CLIENT-SIDE
weapon-param state, so we must reconstruct that state for byte-faithful handlers.

**Foundation RE'd:**
- **Client weapon table** = `FUN_100071c0(type)` → `&DAT_100aa050 + type*0xf7` (0xf7*4 = 0x3dc = 988).
  This is the SAME `s_weapon_info` (988 bytes) the server builds — the client has its own copy at
  `DAT_100aa050` (38 entries), built by a client `compile_weapons_info` mirror. We already have the
  field data in `dlls/ts_weapons.cpp`; the client needs the same table (share or duplicate).
- **Active-weapon param cache** = the `DAT_10189xxx` globals (e.g. `DAT_10189d0c` = active weapon type,
  `DAT_10189ae4` = `FUN_100071c0(type)` = active weapon_info ptr, plus cached origin/angles/velocity/
  attachment + per-shot state). Populated when the client deploys/switches a weapon (the weapon-param
  message / the draw event) and read by every event handler (fire/reload/pump/…) for shell type, clip
  model, muzzle attachment, anim indices, etc.
- **Studio/event API ptrs** the handlers call: `DAT_100d5c78` (studio/temp-entity), `DAT_100d5c74`,
  `DAT_100d5c38` (RANDOM_LONG), `DAT_100d5c34` (RANDOM_FLOAT), `DAT_100d5c3c` (pfnHookEvent).

**Build order (each step RE'd + verified):**
1. **Client weapon table**: make the full `s_weapon_info[38]` available client-side (share the server
   `compile_weapons_info` data, or a `cl_dll` mirror) + a client `get_weapon_info`. (need the full
   struct fields the handlers read: shell-model id, clip model, attachment idx, anim seqs, +0xd2 grip…)
2. **Active-param cache**: reconstruct the deploy/weapon-switch path that fills the active state
   (the server already sends a weapon-param message on deploy — RE `UpdateClientData`/the WeaponInfo
   message + the client receiver; cache the active `s_weapon_info` + per-shot state).
3. **Event handlers**: port each `FUN_xxxx`/`LAB_xxxx` handler (table above) onto the cache, hook them,
   and `PLAYBACK_EVENT` them server-side at the playback sites (replacing the SendWeaponAnim/EMIT_SOUND).
4. Build + in-engine verify each (predicted 1st-person, no lag), all 38 weapons.

This is a multi-step front; do it in order (table → cache → handlers), not all at once.

## Implementation pattern (per event)
1. RE the client handler `FUN_xxxx` → its anim (EV_WeaponAnimation) / sound
   (EV_PlaySound) / effects, and the event args it reads.
2. Add an `EV_TS<Name>` to `cl_dll/ev_ts.cpp`; hook it (the cl_dll event-hook
   registration — see where EV_TSFire is hooked).
3. Server: `PRECACHE_EVENT` a handle in `CTSGun::Precache`; `PLAYBACK_EVENT_FULL`
   it at the playback site, replacing the SendWeaponAnim/EMIT_SOUND stand-in.
4. Build + in-engine verify (1st-person predicted, no lag).
