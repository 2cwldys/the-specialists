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
