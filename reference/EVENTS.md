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

## Implementation pattern (per event)
1. RE the client handler `FUN_xxxx` → its anim (EV_WeaponAnimation) / sound
   (EV_PlaySound) / effects, and the event args it reads.
2. Add an `EV_TS<Name>` to `cl_dll/ev_ts.cpp`; hook it (the cl_dll event-hook
   registration — see where EV_TSFire is hooked).
3. Server: `PRECACHE_EVENT` a handle in `CTSGun::Precache`; `PLAYBACK_EVENT_FULL`
   it at the playback site, replacing the SendWeaponAnim/EMIT_SOUND stand-in.
4. Build + in-engine verify (1st-person predicted, no lag).
