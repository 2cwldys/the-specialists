# cont89 — Weapon fire-cadence + accuracy prediction (clip flicker / semi-auto / spread)

**Symptom (original TS `client.dll` → OUR `hl.so`), firing a gun:**
1. Clip **flickers 19↔20** during fire, snaps correct only on reload.
2. **Full-auto acts like semi** — one shot per click instead of hold-to-fire.
3. **Absurd bullet spread** — the dynamic cone walks all the way out.

**One root: fire-cadence/state desync.** The original client's weapon prediction is faithful
and shares source with the original server, so it predicts *exactly what the original server
would produce*. Every symptom is the original client mispredicting because OUR server puts a
different fire cadence / clip / fire-mode on the wire (and in its own timing) than the original
server did. RE'd on both sides: server `reference/ts_i386.so` (symboled), client
`reference/client.dll` (no symbols; binmap + typed-`this` decompile).

Addresses: server as `nm` / `(Ghidra=nm+0x10000)`. Client as `client.dll` RVA.

---

## The prediction contract (how the original client stays in sync)

The engine sends the player a full `clientdata_t` every frame. The DLL packs the **active
weapon's predicted state into the clientdata `vuser` fields** in the global
`UpdateClientData(edict, sendweapons, clientdata)` — `0xa0294 / 0xb0294`, `/* client.cpp */`.
Verified field map (`clientdata_t`: vuser1@0x1ac, vuser2@0x1b8, vuser3@0x1c4, vuser4@0x1d0):

| clientdata | src (original) | meaning |
|---|---|---|
| vuser3[0] 0x1c4 | `CTSGun+0xc8` (m_weaponType) | active weapon type |
| vuser3[1] 0x1c8 | `activeItem+2`  | **fireMode** (0 AUTO … 5 AKIMBO_BOTH) |
| vuser3[2] 0x1cc | `activeItem+0x10` | attach mask |
| vuser4[0] 0x1d0 | `activeItem+0x24` | altMode |
| **vuser4[1] 0x1d4** | **`activeItem+4`** | **CLIP (live)** |
| vuser4[2] 0x1d8 | `activeItem+0xc` | leftClip |
| vuser2[0] 0x1b8 | `activeItem+8`  | bloody |
| vuser1[2] 0x1b4 | `activeItem+0x14` | active-attach |

`activeItem` = the current `s_weapon_status` in the m_items arsenal (`CTSGun+0xcc`). Our server
writes the **same offsets** (`dlls/client.cpp:2163-2175`, `cd->vuser3.x/.y … cd->vuser4.y`) using
the flat live members (`m_weaponType/m_fireMode/m_iClip`) — packing verified correct.

**Client side — `HUD_WeaponsPostThink` (`client.dll FUN_10003830`, via `HUD_PostRunCmd` RVA
0x45b0):** re-seeds the predicted active weapon **from these `from`-clientdata vuser fields EVERY
FRAME** (weaponType `= __ftol(from.vuser3[0])`, then into a global `s_weapon_status`
`DAT_1009f33c + type*0x30`: +2=fireMode, +4=clip, +0x10=attach, +0x24=altMode…), runs the
predicted `ItemPostFrame`, then writes the result back to `to` at the same offsets.

> **This is the flicker mechanism.** The clip is client-predicted (StandardFire does `clip--`)
> AND authoritatively re-seeded from `vuser4[1]` every frame. If, on any frame, the server's
> `vuser4[1]` does not equal the client's predicted clip, the seed **reverts** the predicted 19
> back to 20 next frame → **19↔20**. They must agree frame-for-frame, which means our server must
> fire/decrement on exactly the frames the client predicts.

---

## (a) Fire cadence + clip — original vs ours

**`CTSGun::StandardFire` (`0x724a4 / 0x824a4`), one shot:**
```
ShootBulletsPlayer(...)                       // bullets FIRST
if (!InfiniteClips) activeItem->iClip--        // decrement gated by InfiniteClips
bparam1 = (activeItem->iClip == 0)
PlaybackEvent(m_usFire, …, weaponType, activeItem->attach, bparam1, player+0x7ec /*seed*/)
if      (fireMode == BURST) rate = 0.03; if (burstOrPhase==1) rate = 0.2   // fast, 0.2 last
else if (fireMode == SEMI && (activeInfo->iFireModes & 1)) rate = 0.1       // ← SEMI+has-AUTO: FIXED 0.1
else    rate = activeInfo->flFireRate
CTSGun+0x9c (m_flNextPrimaryAttack) = WeaponTimeBase() + rate
CTSGun+0xa8 (m_flTimeWeaponIdle)    = WeaponTimeBase() + 2.0   (0.5 for wpn 0x24 when dry)
```
Client `StandardFire` (`FUN_100091e0`) does the identical thing: `activeItem->iClip--` (+ the
m_items-global copy `type*0x30+0x108`), the event, and `weapon+0x8c (m_flNextPrimaryAttack) =
time + rate` with the **same** rate ladder (burst 0.2/0.03, **semi+flag 0.1**, else `flFireRate
@activeInfo+0x18`). So the client predicts 0.1s cadence for a dual-mode weapon in semi.

**Two clip fields in the original — do NOT conflate:**
- `activeItem->iClip` (`activeItem+4`) — the LIVE fire clip. Decremented in StandardFire; sent
  via **vuser4[1]** (the prediction seed) and used by `WeaponIdle`.
- `CTSGun::m_iClip` (`CTSGun+0xb4`, stock `CBasePlayerWeapon::m_iClip`) — the **weapon_data**
  channel (`GetWeaponData 0xa00ac/0xb00ac` packs `weapon_data.m_iClip = gun+0xb4`). It is set
  **only** in `WeaponIdle` (`0x754dc/0x854dc`: `this->m_iClip = activeItem->iClip;`) and to `1`
  in Spawn. During sustained fire, StandardFire pushes `m_flTimeWeaponIdle +2s`, so `WeaponIdle`
  does **not** run → `gun+0xb4` stays **stale** (last idle value). i.e. the original's
  `weapon_data.m_iClip` is *stable during fire*.

**Ours (divergences):**
- `dlls/ts_gun.cpp:1058` `m_iClip--` is **unconditional** (original gates on `!InfiniteClips`),
  and runs **before** `ShootBulletsPlayer` (original: after). Cosmetic ordering; the
  InfiniteClips gate is a real (rare) divergence.
- `dlls/ts_gun.cpp:1119-1133` rate = `wi->flFireRate` only — **missing the `SEMI && (iFireModes
  & 1) → 0.1` special case.** For a dual-mode weapon in semi, the client predicts 0.1s but our
  server re-arms at `flFireRate` (e.g. 0.054) → the two disagree on the next eligible-fire frame
  → the server accepts/rejects a shot the client didn't/did → clip diverges by one → flicker +
  over/under-count. **This is the top concrete cadence divergence.** (`ts_gun.cpp` is shared —
  fixing StandardFire fixes both hl.so and client.so.)
- We **conflate** the two clips: `m_iClip` is our live clip AND the `weapon_data.m_iClip` source
  (`dlls/client.cpp:1998 item->m_iClip = gun->m_iClip`). So our `weapon_data.m_iClip`
  **decrements live** every shot, whereas the original's is WeaponIdle-synced/stale. Our
  `WeaponIdle` (`ts_gun.cpp:2715+`) does not mirror the clip into a separate `weapon_data`
  field (comment admits it). This is a second authoritative clip on the wire that the original
  client does not expect to move during fire.

**`gmsgWeaponInfo` (the HUD ammo message) — `CTSGun::UpdateClientData 0x758c8/0x858c8`:** the
original sends it **only when weaponType or reserve changes** (`bVar1 = (m_weaponType != cache)
|| (reserve != cache)`), NOT on clip change — so it is **silent during fire** (reserve only
changes on reload). The clip byte it carries is a cached snapshot. During fire the HUD clip is
therefore the **predicted** weapon state; the message re-syncs it on reload/switch → *"syncs
only on reload."* **Ours over-sends:** `dlls/ts_gun.cpp:2448` includes `clipByte ==
m_iClientClip` in the change gate, so we emit `WeaponInfo` on **every** clip change — spamming a
redundant authoritative clip during fire.

---

## (b) Semi vs auto gate — MATCHES (both sides), so verify the DATA

`CTSGun::PrimaryAttack` (`0x71a20 / 0x81a20`) gates on `activeItem->fireMode`:
- **mode 0 AUTO / 5 AKIMBO_BOTH → fall straight to `Fire()`** = fire every held frame.
- **mode 1 SEMI / 4 AKIMBO_ALT → fire only if `m_afButtonPressed & IN_ATTACK` (fresh press) OR
  `DoubleFireRate`** = one shot per press (held-continuous only with the bullet-time powerup).
- mode 2 BURST (sets `burstOrPhase`, on `player+0x814 & 1`); mode 3 PUMP (`pev->button & 1`).

Client `PrimaryAttack` (`FUN_100089a0`, reached from `ItemPostFrame FUN_1000b7b0` when `(button
& IN_ATTACK) && weapon+0x8c <= time`) reads `activeItem+2` and gates **byte-identically**.

Our `CTSGun::PrimaryAttack` (`dlls/ts_gun.cpp:944-988`) is equivalent (AUTO/AKIMBO_BOTH/PUMP →
`held`; SEMI/AKIMBO_ALT → `pressed`; DoubleFireRate → `held`). Enum
(`dlls/ts_weapon_info.h:263`) matches: AUTO=0, SEMI=1, BURST=2, PUMP=3, AKIMBO_ALT=4,
AKIMBO_BOTH=5.

⇒ The gate logic is NOT inverted. "Full-auto acts like semi" therefore means the **mode value on
the wire is SEMI(1) when it should be AUTO(0).** The client predicts semi iff `vuser3[1]==1`.
Our `vuser3[1] = tsgun->m_fireMode`, set on deploy to `TSDefaultFireMode(wi->iFireModes)`
(`dlls/ts_gun.cpp:316`; "auto if offered, else pump, else burst, else semi"). If a weapon's
`iFireModes` mask in our table (`dlls/ts_weapon_info.cpp`) lacks the AUTO bit (`1<<TSFM_AUTO ==
1`), `TSDefaultFireMode` returns SEMI and **both** sides gate semi (self-consistent, so no
flicker — but wrong). **Action: audit each auto weapon's `iFireModes` against the original.**
(If instead our server keeps m_fireMode AUTO but the *cadence* desync above starves the fire
loop, that shows as flicker, not clean semi — so clean semi is a data/mask bug, flicker is a
cadence bug. They can co-occur.)

---

## (c) Spread accumulator — client-predicted-only, downstream of cadence

`CTSGun::ScaleSpread` (`0x702a8 / 0x802a8`): folds a per-player accumulator into this shot's
cone, then grows it; recovers per-shot by time-since-last-shot and again per-frame in PreThink.
- Storage: **`CBasePlayer+0x778`** (server) — our `m_flSpreadAccum` (`dlls/player.h:401`). Client
  copy at **`player+0x764`**, last-shot time at `weapon+0x8a0` (our `m_flLastShotTime`,
  `ts_gun.h:178`). Logic byte-identical (0.875 silenced base; factor 2 ×0.5 doublefire ×0.775
  sil; recovery `GiveSlowMul(player)*dt` capped 0.5; accum capped 0.1) — matches
  `dlls/ts_gun.cpp:1194`.
- **Per-frame recovery** in `CBasePlayer::PreThink` (`0x808cc/0x908cc`): `if (accum>0) accum -=
  GiveSlowMul*frametime*0.1; clamp 0`. Ours matches (`dlls/player.cpp:1921-1925`).
- **NOT networked.** `UpdateClientData` never writes `player+0x778` into any clientdata field,
  and the client confirms **no seed** of `player+0x764` in the postthink — the accumulator is
  **purely client-predicted**. The client's weapon prediction runs StandardFire (grows it) but
  does **not** run `CBasePlayer::PreThink` (server-only think) — so the client does NOT predict
  the per-frame recovery. It stays in sync with the server **only because the client fires the
  identical shots** (the per-shot recovery inside ScaleSpread, which the client DOES run, keeps
  it bounded when cadence matches).

⇒ **Spread blow-out is downstream.** When the cadence desync (a/b) makes the client predict
extra shots, its accumulator over-grows with no server correction → the cone walks out.
Server-side spread stays correct (our PreThink recovery works). **Fix the cadence and the spread
self-corrects — the accumulator needs no networking.** (Note: server bullets use a fresh
`random_seed = RandomLong(0,32000)` set each `CBasePlayer::ItemPostFrame 0xcc4d0/0xdc4d0` into
`player+0x7ec`, so the exact pellet *pattern* is server-authoritative and not client-predictable
— that is cosmetic and separate from the cone size.)

---

## (d) Reconciliation plan (make the original client predict our server exactly)

The seed channel `vuser4[1]`/`vuser3[1]` and the gate are already correct; the fixes make our
server's fire *cadence and clip channels* byte-faithful so the every-frame re-seed always equals
the prediction. `ts_gun.cpp` compiles into BOTH libs, so StandardFire fixes cover our client too.

**Priority 1 — fire rate (the concrete cadence divergence).** In `CTSGun::StandardFire`
(`dlls/ts_gun.cpp:1119-1133`) add the original's semi-with-auto special case before the default:
```
if (m_fireMode == TSFM_BURST) { … 0.03 / 0.2 … }
else if (m_fireMode == TSFM_SEMI && (wi->iFireModes & (1<<TSFM_AUTO))) flRate = 0.1f;  // NEW
else flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.15f;
```
Also review the BURST ladder to match the original's `burstOrPhase==1 → 0.2` test (ours uses
`<=0`).

**Priority 2 — fire-mode data audit.** Verify each auto weapon's `iFireModes` mask in
`dlls/ts_weapon_info.cpp` includes `1<<TSFM_AUTO` so `TSDefaultFireMode` yields AUTO(0). Any
auto weapon defaulting to SEMI(1) fires "like semi" on both sides. Cross-check against the
original's `compile_weapons_info` mask values.

**Priority 3 — clip channels: mirror the original's stable-during-fire behaviour.**
- `weapon_data.m_iClip`: stop sending the live clip. Keep a separate cache synced only in
  `CTSGun::WeaponIdle` (`this-><cache> = m_iClip`, like the original `gun+0xb4 = activeItem->iClip`)
  and pack THAT into `weapon_data.m_iClip` (`dlls/client.cpp:1998`). Leave `vuser4[1]` = live
  `m_iClip` (the real prediction seed).
- `gmsgWeaponInfo` change gate (`dlls/ts_gun.cpp:2448`): drop `clipByte` from the comparison —
  send only on **weaponType or reserve** change, matching `CTSGun::UpdateClientData 0x858c8`. The
  clip byte still rides the message; it just stops re-emitting every shot.

**Priority 4 — decrement fidelity.** Gate the `m_iClip--` on `!InfiniteClips` and order it after
`ShootBulletsPlayer` (match `StandardFire 0x824a4`). Low impact but part of drop-in exactness.

**Verification (live A/B — user drives).** After P1/P2, if any residual flicker remains, confirm
the exact frame with a live trace (the user keeps original TS in `hl.exe`; see CLAUDE.md §4b):
`binmap.py CTSGun::StandardFire` → `live.py break mp.dll 0x<rva>` (or a caller if string-poor),
fire one shot, read `activeItem->iClip` (`activeItem+4`) and `m_flNextPrimaryAttack`
(`gun+0x9c`); compare against our server's `m_iClip`/`m_flNextPrimaryAttack` for the same
usercmd. Equal values on the same frame = the vuser4[1] re-seed matches the prediction = no
flicker. Do the same for a full-auto weapon to confirm `vuser3[1]==0` and continuous fire.

---

## Appendix — addresses

Server (`ts_i386.so`): StandardFire `0x724a4/0x824a4` · Fire `0x71c78/0x81c78` · PrimaryAttack
`0x71a20/0x81a20` · ScaleSpread `0x702a8/0x802a8` · WeaponIdle `0x754dc/0x854dc` ·
CTSGun::UpdateClientData `0x758c8/0x858c8` · CBasePlayer::UpdateClientData `0x81d00/0x91d00` ·
UpdateClientData(dll) `0xa0294/0xb0294` · GetWeaponData(dll) `0xa00ac/0xb00ac` ·
CBasePlayer::PreThink `0x808cc/0x908cc` · CBasePlayer::ItemPostFrame `0xcc4d0/0xdc4d0`.

Client (`client.dll` RVA): HUD_WeaponsPostThink `FUN_10003830` (via HUD_PostRunCmd 0x45b0) ·
weapon ItemPostFrame `FUN_1000b7b0` (vtbl 0x134) · PrimaryAttack `FUN_100089a0` (vtbl 0x174) ·
Fire `FUN_10008b30` · StandardFire `FUN_100091e0` · ScaleSpread `FUN_10008100`. Predicted
weapon: m_flNextPrimaryAttack `weapon+0x8c`, fireMode `activeItem+2`, clip `activeItem+4`; spread
accumulator `player+0x764`, last-shot `weapon+0x8a0`; global weapon table `DAT_1009f33c +
type*0x30`.
