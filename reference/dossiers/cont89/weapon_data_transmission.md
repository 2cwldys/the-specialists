# cont89 — Weapon-state client-prediction transmission (the "kungfu-for-every-weapon" drop-in bug)

**Symptom.** Original TS client (Steam `hl.exe` / `client.dll`) vs. OUR reconstructed
server (`hl.so`): every weapon renders **melee / kungfu** (melee anim + sound + bob,
no bullet marks / tracers) while the SERVER fires the real gun correctly (enemies take
real damage). Movement also rubber-bands. The `delta.lst` wire table was ruled out by an
A/B swap.

**Root cause (now fully RE-confirmed).** Our server transmits the client-prediction weapon
state through an **entirely invented layout** on **two channels**, and the original client
reads **neither** the way we pack them:

1. We invented a `CTSGun::GetWeaponData` / `SetWeaponData` **virtual override**
   (`dlls/ts_gun.cpp:760,786`) + a `gun->GetWeaponData(*item)` call in the generic loop
   (`dlls/client.cpp:2011`). **The original has NO such override** — there is no
   `GetWeaponData__6CTSGun` / `SetWeaponData__6CTSGun` symbol in `ts_i386.so`. The original
   uses the **stock** generic loop only.
2. The original carries the **active weapon's whole predicted state through the CLIENTDATA
   block** (`clientdata_t` vuser1–4 / iuser / fuser), written by the DLL-level
   `UpdateClientData` (`UpdateClientData__FPC7edict_siP12clientdata_s`, nm `0xa0294`) and
   read back in `HUD_WeaponsPostThink`. **Most critically the active `m_weaponType` rides
   `clientdata.vuser2.z` (cd+0x1c4).** Our DLL `UpdateClientData` (`dlls/client.cpp` ~2088–2160)
   fills those same vuser fields with a **different, invented meaning** (ammo counts / powerups /
   ammo types) and **never writes the weapon type there**.

Consequence on the original client: `HUD_WeaponsPostThink` computes the active weapon as
`g_ActiveTSWeapon = (int)clientdata.vuser2.z`. Our server leaves that field 0, so the client's
active weapon is **tsweapon_t 0 = TSW_MELEE = kungfu for EVERY weapon**. Meanwhile our invented
`GetWeaponData` override scribbles TS values into the stock prediction fields
(`m_chargeReady`/`m_fInAttack`/`m_fireState`/`m_flStartThrow`/`m_flReleaseThrow`), so the stock
prediction the original client DOES read is corrupt → the rubber-banding.

---

## (a) `weapon_data_t` field → CTSGun member — the STOCK generic loop

`GetWeaponData(edict_s*, weapon_data_s*)` @ nm `0xa00ac` (Ghidra `0xb00ac`) is a **generic
loop over all items** (NOT a per-weapon virtual). It is **byte-identical to the stock HL SDK
loop** (`dlls/client.cpp:1996–2009`), packing only stock `CBasePlayerWeapon` members. Each
`weapon_data_t` slot is `0x58` bytes.

| wd off | weapon_data_t field | server reads (weapon member) | stock member / semantic |
|---|---|---|---|
| 0x00 | m_iId                  | loop index                | m_iId |
| 0x04 | m_iClip                | wpn+0xb4                  | m_iClip |
| 0x08 | m_flNextPrimaryAttack  | wpn+0x9c (max −0.001)     | m_flNextPrimaryAttack |
| 0x0c | m_flNextSecondaryAttack| wpn+0xa0 (max −0.001)     | m_flNextSecondaryAttack |
| 0x10 | m_flTimeWeaponIdle     | wpn+0xa8 (max −0.001)     | m_flTimeWeaponIdle |
| 0x14 | m_fInReload            | wpn+0xc0                  | m_fInReload |
| 0x18 | m_fInSpecialReload     | wpn+0x98                  | m_fInSpecialReload |
| 0x38 | **iuser1**             | wpn+0x54                  | **m_chargeReady** |
| 0x3c | **iuser2**             | wpn+0x58                  | **m_fInAttack** |
| 0x40 | **iuser3**             | wpn+0x5c                  | **m_fireState** |
| 0x48 | **fuser1**             | *(float*)(pev+0x254) (max −0.001) | **pev->fuser1** |
| 0x4c | **fuser2**             | wpn+0x4c                  | **m_flStartThrow** |
| 0x50 | **fuser3**             | wpn+0x50                  | **m_flReleaseThrow** |

The original packs **nothing** into `iuser4` / `fuser4` / `m_iWeaponState` / the aim/zoom/pump
fields (`0x1c`–`0x34`, `0x44`, `0x54`). `SetWeaponData` on the client is the stock inverse of the
identical member list (no override).

### The requested weapon-member offsets, resolved
| weapon off | semantic (stock CBasePlayerWeapon unless noted) | carried by |
|---|---|---|
| 0x4c | m_flStartThrow      | weapon_data.fuser2 |
| 0x50 | m_flReleaseThrow    | weapon_data.fuser3 |
| 0x54 | m_chargeReady       | weapon_data.iuser1 |
| 0x58 | m_fInAttack         | weapon_data.iuser2 |
| 0x5c | m_fireState         | weapon_data.iuser3 |
| 0x98 | m_fInSpecialReload  | weapon_data +0x18 |
| 0x9c | m_flNextPrimaryAttack | weapon_data +0x08 |
| 0xa0 | m_flNextSecondaryAttack | weapon_data +0x0c |
| 0xa8 | m_flTimeWeaponIdle  | weapon_data +0x10 |
| 0xb4 | m_iClip             | weapon_data +0x04 |
| 0xc0 | m_fInReload         | weapon_data +0x14 |
| **0xc8** | **m_weaponType (TS-specific — active tsweapon_t)** | **NOT weapon_data — travels via clientdata.vuser2.z (see (c))** |
| pev+0x254 | pev->fuser1     | weapon_data.fuser1 |

`gun+0xcc` = `m_pActiveItem` (`s_weapon_status*`), `gun+0xd0` = pending/next item,
`gun+0x114` = the embedded `s_weapon_status` arsenal array (stride `0x38`),
`gun+0xd4` = a per-firemode timing array — all confirmed from the two `UpdateClientData`
decompiles.

---

## (b) Client read path — `HUD_WeaponsPostThink` (client.dll `FUN_10003830`, RVA 0x3830)

Reached via the exported `HUD_PostRunCmd` (RVA `0x45b0` → calls `FUN_10003830`). Two proofs
the weapon type is NOT a weapon_data field:

- **SetWeaponData is stock.** The from→weapon unpack reads `weapon_data` slot fields at
  `+0x04/+0x08/+0x0c/+0x10/+0x14/+0x18/+0x40(iuser3)/+0x48(fuser1)/+0x54(fuser4)` straight
  into the client weapon object — the stock inverse. No weapon-type read anywhere in the slot.
- **The active weapon type is a global set from CLIENTDATA:**
  ```
  if (weapon_id == 0x10)                 // 0x10 == TSGUN_WEAPON_ID (all TS guns share it)
      g_ActiveTSWeapon = (int)*(float*)(from_state + 0x318);   // == clientdata.vuser2.z
  ```
  (disasm `0x39f0`: `flds 0x318(%esi); call __ftol; mov %eax,0x1009f2f0`.) That global then
  indexes the per-weapon status/anim table `&DAT_1009f33c + type*0x30` and
  `get_weapon_info(type)` (`FUN_100071c0`) — i.e. it drives which weapon's fire/anim the
  predictor runs. With it stuck at 0, the predictor runs **kungfu**.

`local_state` clientdata is embedded at **base +0x154** (`weapon_data[0]` at `+0x330`), so
`clientdata+X` in the server ↔ `local_state+(0x154+X)` on the client (`cd+0x1c4 ↔ ls+0x318`,
verified).

---

## (c) The clientdata weapon-status channel — server pack (`UpdateClientData` nm 0xa0294)

Runs only when `sendweapons != 0`. Walks to the local player's active weapon
`iVar4` (= `m_pActiveItem->GetWeaponPtr()`; the `m_iId==0x10` gate), then:

| cd off | ls off | float field | server source | semantic (our name) | conf |
|---|---|---|---|---|---|
| **0x1c4** | **0x318** | **vuser2.z** | **(int)(gun+0xc8)** | **m_weaponType (active tsweapon_t)** | **HIGH** |
| 0x1b0 | 0x304 | vuser1.x | (int)(player+0x724) | melee / stamina bar (`m_flMeleeBar`) | HIGH |
| 0x1b4 | 0x308 | vuser1.y | item+0x14 | active-attach / barrel (`TSItem.active`) | MED |
| 0x1b8 | 0x30c | vuser1.z | item+0x08 | blade/bloody-or-state byte (`TSItem.bloody`) | MED |
| 0x1bc | 0x310 | vuser2.x | gun+0xd4[fireMode*4] | per-firemode timing value | MED |
| 0x1c0 | 0x314 | vuser2.y | item+0x2c | holdPosition stance (`TSItem.holdPosition`) | MED |
| 0x1c8 | 0x31c | vuser3.x | item+0x02 | fire-mode (`TSItem.mode`) | MED |
| 0x1cc | 0x320 | vuser3.y | item+0x10 | equipped attach mask (`TSItem.attach`) | MED |
| 0x1d0 | 0x324 | vuser3.z | item+0x24 | alt-mode byte (`TSItem.altMode`) | MED |
| 0x1d4 | 0x328 | vuser4.x | item+0x04 | active clip (`TSItem.clip`) | HIGH |
| 0x1d8 | 0x32c | vuser4.y | item+0x0c | (per-barrel/second clip — VERIFY) | LOW |
| 0x68  | —     | m_iId     | 0x10 | active weapon id | HIGH |
| 0x6c  | —     | weaponanim| pending anim (from active/next item) | queued weaponanim | MED |
| 0x70  | —     | (cd 0x70) | item+0x1c | leftClip (our `gun+0x130`) — VERIFY | LOW |

Plus the always-on passthrough (top of the fn): `cd.iuser1..iuser4 = pev->iuser1..4`
(`cd+0x18c/0x190/0x198/0x19c` ← `pev+0x2c4/0x2c8/0x2d0/0x2d4`) and `cd.fuser*` — these are the
stock spectator/observer fields, unrelated to the weapon channel.

**Our divergence (`dlls/client.cpp` ~2088–2160).** We fill the SAME vuser slots with invented
ammo/powerup data: `vuser1 = ammo_9mm/357/argrens`, `vuser2.x = ammo_hornets`,
`vuser3.z = m_iSecondaryAmmoType`, `vuser4 = ammo-type/rgAmmo`, `iuser4 = m_iPowerups`,
`fuser2/3 = m_flNextAmmoBurn/m_flAmmoStartCharge`. **We never write vuser2.z = m_weaponType.**
Our client reads this invented layout consistently with our server, so OUR client works — but it
is not drop-in with the original.

### `s_weapon_status` (the arsenal slot, = our `TSItem`, stride 0x38, at gun+0x114)
Confirmed field offsets used above: `+0x00 owned`, `+0x02 mode`, `+0x04 clip`, `+0x10 attach`,
`+0x14 active`, `+0x24 altMode`, `+0x2c holdPosition`. `+0x08`, `+0x0c`, `+0x1c` map to our
`bloody` / a second clip / `leftClip` respectively but are **VERIFY** (only their offsets, not
full semantics, are RE-confirmed here).

---

## (d) `WeaponInfo` message — byte format (orig vs ours)

Server writer = `CTSGun::UpdateClientData` (nm `0x758c8`). This is the HUD/ammo channel
(MSG_ONE to the owner), **separate** from the prediction channels above.

Original wire (both branches), 6 bytes:
```
BYTE  weaponType     (this->m_weaponType)
BYTE  clip           (active m_pActiveItem->iClip; melee: ROUND(player+0x774) stamina bar)
SHORT reserve        (m_ammoReserve[caliber])
BYTE  fireMode       (field_0x97e — cached active fire-mode)
BYTE  flags/attach   (field_0x98c — cached active attach mask)
```
Our wire (`dlls/ts_gun.cpp:2517–2523`): `BYTE m_weaponType, BYTE clipByte, SHORT reserve,
BYTE m_fireMode, BYTE equipped`. **Byte-format identical (B,B,S,B,B).** ✅

Two behavioral (not wire) differences, non-fatal but note for fidelity:
- **Change-detection.** Original re-sends `WeaponInfo` only when `m_weaponType` **or** the
  caliber reserve changes (`field_0x97a`/`field_0x9b4` shadow caches). Ours also re-sends on
  clip / fireMode / attach change (more frequent, same bytes).
- **Rate-limit.** Original gates the whole `UpdateClientData` body behind a ~0.1s timer
  (`field_0x9e0 = gpGlobals->time + 0.1`); we send every call.
- `TSWeaponStatus` per-slot loop and the `TSActiveItems` broadcast also live inside the original
  `UpdateClientData`; our split matches functionally (already merged back per cont.88).

**The WeaponInfo channel is NOT the bug** — it is wire-compatible and DOES deliver `m_weaponType`
to the original client's HUD (`MsgFunc_WeaponInfo`). But the original client's **predictor** takes
the weapon type from `clientdata.vuser2.z`, not from `WeaponInfo`, so a correct `WeaponInfo` alone
cannot fix the kungfu prediction.

---

## Reconciliation plan (server + client) — become drop-in with the ORIGINAL

**Goal:** stop transmitting via the invented `GetWeaponData` override; carry the active-weapon
predicted state on the **clientdata vuser channel** exactly as the original, most importantly
`vuser2.z = m_weaponType`. Preserve the round-6/round-7i barrel-clip and attach intent by mapping
them onto the original's field choices (clip → `vuser4.x`; per-barrel/attach → the vuser/`cd+0x70`
slots the original uses), NOT onto `weapon_data.fuser2/3`.

### Server
1. **Delete the invented weapon_data override.**
   - `dlls/ts_gun.cpp:760–817` — remove `CTSGun::GetWeaponData` and `CTSGun::SetWeaponData`.
   - `dlls/ts_gun.h:66`-area — remove their declarations (`GetWeaponData`/`SetWeaponData`).
   - `dlls/client.cpp:2011` — remove `gun->GetWeaponData(*item);`. The generic loop
     (`client.cpp:1996–2009`) already packs the stock members exactly like nm `0xa00ac` — leave it.
2. **Populate the clientdata weapon channel in the DLL `UpdateClientData`** (`dlls/client.cpp`,
   the `if (sendweapons)` / active-weapon block ~2116–2160). Replace the invented ammo/powerup
   vuser assignments with the original's, per table (c). Minimum viable fix (unblocks the
   kungfu symptom): set `cd->vuser2.z = (float)gun->m_weaponType;` and `cd->m_iId = 0x10`.
   Full fidelity: also `vuser1.x = m_flMeleeBar`, `vuser4.x = active clip`, `vuser3.x = mode`,
   `vuser3.y = attach`, `vuser3.z = altMode`, `vuser2.y = holdPosition`, `vuser2.x = per-firemode
   timing`, `vuser1.y = active`, `vuser1.z = bloody`, and the `cd+0x70` leftClip slot.
   Keep melee-bar/`m_iId`/anim writes as in the decompile.
3. Mirror the original's `UpdateClientData` gating if strict fidelity is wanted (0.1s rate-limit,
   type/reserve-only WeaponInfo change-detect) — optional; wire is already compatible.

### Client (our `cl_dll`, so OUR client stays consistent with the new server layout)
4. **Read the weapon type from clientdata, not weapon_data.** In `HUD_WeaponsPostThink`
   (`cl_dll/hl/hl_weapons.cpp` — our copy) set the active `CTSGun::m_weaponType` from
   `from->client.vuser2.z` (the `m_iId==0x10` branch), mirroring `0x39f0`. Then drop the
   `CTSGun::SetWeaponData` reliance on `iuser1` for the type.
5. Unpack the rest of the clientdata channel (per table (c)) into the predicted active
   `CTSGun` / its `m_items[m_weaponType]` (clip → `TSItem.clip`, per-barrel → the mapped slot,
   attach/active/mode/altMode/holdPosition) — the inverse of step 2. Remove the invented
   `SetWeaponData` unpack of `iuser1..4/fuser2/3/m_iWeaponState` (`dlls/ts_gun.cpp:786–817`).
6. Because both our client AND server change together, our-vs-our prediction stays correct while
   our server also becomes drop-in with the ORIGINAL client (which reads exactly `vuser2.z` +
   stock weapon_data).

### Verification
- **Drop-in test:** original `hl.exe` client → our `hl.so` server: every weapon renders its own
  viewmodel + real fire (tracers/marks), no kungfu, no rubber-band. (User-driven playtest —
  live `hl.exe` pid available for a gdb watch on `0x1009f2f0` to confirm the type arrives.)
- **Regression:** our client → our server unchanged (barrel-clip dry-click, attach submodels,
  laser/flash toggle, silenced sound all still predict).

### Provenance
- `GetWeaponData` generic loop: `ts_i386.so` nm `0xa00ac` (Ghidra `0xb00ac`).
- DLL `UpdateClientData` (clientdata pack): `ts_i386.so` nm `0xa0294` (Ghidra `0xb0294`).
- `CTSGun::UpdateClientData` (WeaponInfo/WStatus): `ts_i386.so` nm `0x758c8`.
- Client `HUD_WeaponsPostThink`: `client.dll` `FUN_10003830` (RVA `0x3830`), via `HUD_PostRunCmd`
  RVA `0x45b0`; the weapon-type read at disasm `0x39f0` (`flds 0x318; __ftol; mov 0x1009f2f0`).
- No `GetWeaponData__6CTSGun` / `SetWeaponData__6CTSGun` symbol exists in `ts_i386.so` (the
  override is ours).
