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
