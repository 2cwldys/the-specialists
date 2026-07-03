# Dossier: client.dll gun-tail event handlers (reload.sc, empty.sc, pump.sc, grenade.sc, firelinked.sc)

RE'd from `reference/client.dll` (no symbols) via the prebuilt `clientdll` Ghidra project
(`scripts/ghidra/dc.sh client 0x<RVA>`) + raw objdump disasm for the args Ghidra elided
(engine calls go through untyped cached fn pointers). Companion to
`reference/dossiers/cont81/dossier-ev-melee.md` — its §0 (engine-pointer map, `get_weapon_info`
stride 0x3dc @0x100aa050, anim block wi+0x252 / hold twin +0x28c, `FUN_1003df60` hold-aware
selector, `FUN_1001c5c0/5e0` locality gates, `FUN_10007110` attach-mask→body, the
§0.6 timescale pitch formula) is assumed; only NEW shared infra is documented here.
Raw captures in this directory: `ev_reload.c`, `ev_empty.c`, `ev_pump.c`, `ev_grenade.c`,
`ev_firelinked.c`, `ev_fire_wrapper.c` (fire.sc dispatcher), `ev_fire_core.c`
(FUN_1003f090 pistol/akimbo core + brass/visibility helpers), `ev_fire_helpers.c`
(FUN_100450c0 FireBullets + FUN_1003de90 fire-sound + FUN_100474b0 muzzleflash + FUN_10048e60 dlight).

## Hook table (registration fn @0x3240; pfnHookEvent = `DAT_100d5c3c`)

| event | handler (Ghidra addr) | RVA | orig server handle slot (CTSGun::Precache @0x70660) |
|---|---|---|---|
| reload.sc | `FUN_10042690` | 0x42690 | gun+0x970 |
| empty.sc | `FUN_100424e0` | 0x424e0 | gun+0x968 |
| pump.sc | `FUN_100419a0` | 0x419a0 | gun+0x96a |
| grenade.sc | `FUN_10042630` | 0x42630 | gun+0x978 |
| firelinked.sc | `FUN_1003de10` | 0x3de10 | gun+0x96e |
| (fire.sc, RE'd previously) | `FUN_1003de40` | 0x3de40 | gun+0x964 |

Full original handle map (recovered from the Precache stores, GOT 0x165E10): +0x964 fire ·
+0x966 coldcock · +0x968 empty · +0x96a **pump** · +0x96c closecombat · +0x96e **firelinked** ·
+0x970 reload · +0x972 knife · +0x974 katana · +0x976 draw · +0x978 grenade.
A second string-ref site @0x84e0 is the CLIENT-side `pfnPrecacheEvent(1, name)` mirror storing
the event indices into a client struct at +0x824..+0x838 (same 11 events).

`event_args_t` offsets: `entindex`+4 · `origin`+8 · `angles`+0x14 · `velocity`+0x20 ·
`fparam1`+0x30 · `fparam2`+0x34 · `iparam1`+0x38 · `iparam2`+0x3c · `bparam1`+0x40 · `bparam2`+0x44.

---

## 0. New shared infrastructure (beyond cont81 §0)

### 0.1 Client `weapon_info` fields used here (stride 0x3dc; layout ≠ the server's 0x358 struct)
- **+0xcc** int = single/base weapon type (17 = the skorpion family — matches server
  `iSingleType`+0xcc semantics: skorpion AND akimbo-skorpion both read 17).
- **+0xd2** byte = **fire class** (values seen: 1 = pump shotgun, 4/5 = akimbo-capable pistol /
  akimbo-only — same value space as server `iFireType`@+0x348).
- **+0xe8** char[] = **shell casing model** path (empty ⇒ no brass).
- **+0x2c6** char[64] = **fire sound**; **+0x306** char[64] = **second sound** — dual-purpose:
  the pump/rack ("scock") sound for shotguns, the both-barrels bang for akimbos (they're
  disjoint weapon sets, one field serves both). reload.sc phase 10 and pump.sc play it too.
- **+0x34c** int = shell-eject/muzzle **forward offset** (units back along fwd from the attachment).
- **+0x350** byte, **+0x354** int, **+0x358** float = muzzleflash params (sprite variant / scale)
  read by `FUN_100474b0`.
- Low offsets shared with the server struct: +0x8 caliber (index into the 16-row caliber table),
  +0x1c damage, +0x20 int, +0x24 pellet count, +0x2c spread flag — passed through to FireBullets.

### 0.2 Anim-block index map extended (block = `FUN_1003df60(wi)` → wi+0x252 or hold +0x28c)
[1] reload · [2] draw · [3..5] **fire-linked** (both barrels) · [8] shotgun-reload OPEN (also the
sawed-off shell-retained reload) · [9] shell INSERT · [10] reload CLOSE · [13/14] **pump rack**
(random) · [15..17] melee swings · [19/20/21] grenade pin/overhand/underhand · [22/23] fire
LEFT barrel · [24/25] fire RIGHT barrel · [28] coldcock. All five handlers here use the
HOLD-AWARE selector (unlike melee, which always uses +0x252).

### 0.3 Brass / attachment / visibility helpers
- `FUN_10056890(pos, angles, vel, playerVel, modelidx, bounceSound)` = **TS_EjectBrass**:
  `pEfxAPI->R_TempModel(pos, vel, {angles.x, angles.y − 90, angles.z}, life DAT_1018ca54, model, 0)`,
  then sets tempent `hitSound = bounceSound` (1 = shellcasing, 2 = shotshell), flags |= 4,
  random avelocity = RandomFloat(−60,60)² per axis (z scaled ×0.5). Keeps a 64-slot ring
  (`DAT_1018bcc8`) so old shells free their slot.
- `FUN_10056be0(out, playerIdx, n)` = cached **player attachment origin** [idx*4+n] from the
  per-frame table @0x1018caf8 (attachment 0/2 = the two gun barrels/eject ports, 3 = pump port).
- `FUN_100579a0(idx)` = **player-visible-this-frame** byte table @0x1018bd44 (idx 0..32) — gates
  all third-person brass/clip/smoke so effects only spawn for players actually drawn.
- `DAT_101914a0` = the local **viewmodel cl_entity\***; `+0xb60` = its `attachment[0]`,
  `+0xb78` = `attachment[2]` (stride 0xc). `DAT_10190b28` = the predicted local gun-position
  vec3 global (fallback eject origin when there is no viewmodel entity).

### 0.4 Effect helpers
- `FUN_10047130(scale, alphaBase, pos, tail, dir, spread, flag)` = **gun-smoke ribbon**:
  5 puffs of `sprites/gun_smoke_add.spr` along `dir` (framerate ~0.1, base scale
  `scale*0.5`, grows ×0.2/step, alpha decays ×0.8, per-puff jitter alphaBase+RandomLong,
  drift RandomFloat(±0.2·scale)); gated on **cvar `cl_fx`** (`DAT_100df464`, default "1",
  registered @0x26fdd) and, when `flag==0`, rate-limited via the timescale clock.
- `FUN_100474b0(attachPos, idx, attach#, …, wi, …)` = **TS muzzleflash sprite** at a player
  attachment — `sprites/ts_muzzleflash1h.spr` (variant `ts_muzzleflash6.spr` by wi+0x354/0x350),
  scale from wi+0x358 (randomized), plus a particle burst.
- `FUN_10048e60(x,y,z)` = **muzzle dynamic light**: `CL_AllocDlight(0)`, radius 192,
  rgb (240,221,172), die = time + 0.06 (timescaled), rate-limited to one per 0.1s (timescaled),
  gated on a cached fx byte `DAT_1018ca58` (VERIFY exact cvar source).
- `FUN_1003de90(idx)` = the **fire-sound player** (shares the caller's frame — reads wi, the
  linked flag, origin, and the precomputed timescale pitch base):
  - single: `EV_PlaySound(idx, origin, CHAN_WEAPON, wi+0x2c6, RandomFloat(0.92,1.0),
    attn = caliberTable[wi+8].attn, 0, 25+75·timescale+RandomLong(−5,5))`
  - linked: sample = **wi+0x306**, vol **RandomFloat(0.84,0.92)**, attn **×2**, same pitch.
  `FUN_100071f0(caliber)` = row of the 16-entry × 0x38 caliber table @0x1009ffc0 (attn = first float).
- `FUN_100450c0` = the TS **FireBullets** clone (trace/impact/decal/tracer per volley; plays the
  near-miss `weapons/whizz_d.wav` at the local viewpoint; NO fire sound — that's FUN_1003de90).

Standard sound params for everything in this dossier (same as melee §0.6): CHAN_WEAPON(1),
vol RandomFloat(0.92,1.0), attn 0.8, flags 0, **pitch = 25 + 75·g_flTimescale + RandomLong(−5,5)**
unless stated otherwise.

---

## 1. empty.sc — `FUN_100424e0` (server: EmptySound @0x71be4, all params 0)

**Args read:** `entindex`, `origin` — nothing else. 182 bytes total.

**Flow (the whole handler):**
`EV_PlaySound(entindex, origin, CHAN_WEAPON, "weapons/pistol-empty.wav",
RandomFloat(0.92,1.0), 0.8, 0, 25+75·timescale+RandomLong(−5,5))`.
- **One universal dry-click for every gun** (no weapon-type read, no anim, no locality gate).
- NB the filename is **hyphenated** `pistol-empty.wav` (verified @0x100911e4).
- The contender's silent dry-fire is a SERVER-side gate (EmptySound early-returns), not client.

---

## 2. reload.sc — `FUN_10042690` (server: Reload @0x74d5c / ShotgunReload @0x74aac; iparam1=weaponType, iparam2=attach mask, fparam1=phase 0/8/9/10)

**Args read:** `iparam1` weaponType (→ wi), `iparam2` attach mask, **`fparam1` → ftol → phase**,
`entindex/origin/angles/velocity`. AngleVectors → fwd `0x10189de8`, right `0x10189d98`, up `0x10189cd8`.

**Flow:**
1. **Viewmodel anim** — gate `in-eye-spectated || IsLocalOrSpectated` (no 3rd-person check):
   `block = FUN_1003df60(wi)` (hold-aware); `body = FUN_10007110(type, mask)`;
   **seq = block[phase == 0 ? 1 : phase]** — i.e. the phase IS the anim-block index
   (8 = open, 9 = insert, 10 = close), and phase 0 (a full magazine reload) plays block[1].
   (This also means a server may send fparam1=8 to play block[8] with no sound — the
   sawed-off shell-retained reload variant; VERIFY the original server's Reload does this.)
2. **phase 9 (insert):** play `weapons/insert-shell.wav` (std params) — **no locality gate**,
   the local player hears it too. (Hyphenated filename, @0x100953a4.)
3. **phase 10 (close):** ONLY if `thirdperson || !IsLocalOrSpectated`: play **wi+0x306**
   (the weapon's own rack/close sound, std params). The local first-person player is skipped —
   his close/rack sound comes from the pump.sc/viewmodel side.
4. **phase 0 (full reload):** if `!thirdperson && IsLocalOrSpectated` → **return** (the local
   player gets the anim only; reload foley rides the viewmodel's anim events). Otherwise:
   play `weapons/generic_reload.wav` (std params), then if `playerVisible[idx]` eject the
   spent brass/magazine (all from PLAYER attachments, angles = args angles):
   - **TSW_SAWEDOFF (33):** 2 × `models/shotgun_shell.mdl` from attachment 0;
     vel = playerVel + right·RandomLong(−30,30) + fwd·10.0 + up·RandomLong(0,30);
     TS_EjectBrass bounce **2** (shotshell).
   - **TSW_BULL (31):** 5 × `models/9mm_shell.mdl`, same velocity recipe, bounce **1** (the
     revolver dumping its cylinder).
   - **TSW_CONTENDER (36):** 1 × `models/556_shell.mdl`, same recipe, bounce **1**.
   - **default — the magazine toss:** N = **2 if fire class (wi+0xd2) ∈ {4,5} (akimbo) else 1**
     `models/generic_clip.mdl` via raw `R_TempModel`: pos = attachment[i==0 ? 0 : 2] − fwd·8.0;
     vel = right·(i==0 ? +60 : −60) + playerVel; angles = RandomFloat(−60,60)³; life **2.0**;
     bounce sound **0** (silent clip). Akimbo reloads drop one mag each side.

---

## 3. pump.sc — `FUN_100419a0` (server: ShotgunFire's rack half; iparam1=weaponType, iparam2=attach mask)

**Args read:** `iparam1` weaponType (→ wi), `iparam2` attach mask, `entindex/origin/angles/velocity`.
AngleVectors → fwd `0x10189fc8`, right `0x10189e10`, up `0x10189d68`. Precaches the shell model
from **wi+0xe8** (0 if the field is empty). Caches two randoms up front:
R1 = RandomLong(60,80), R2 = RandomLong(50,100).

**Flow:**
1. **Viewmodel anim** — gate `IsLocalOrSpectated`:
   `EV_WeaponAnimation( holdBlock[13 + RandomLong(0,1)], FUN_10007110(type, mask) )` —
   **the rack anim is picked randomly client-side from block [13]/[14]**.
2. **First-person shell** (still inside the local gate, `!CL_IsThirdPerson()`, shell model set):
   - viewmodel entity available (`DAT_101914a0`): eject from **vm attachment[0]** (+0xb60)
     − fwd·wi[+0x34c]; vel = playerVel + fwd·R1 + up·R2 + right·R1;
     `TS_EjectBrass(pos, angles, vel, shell, 2)`; then a **gun-smoke ribbon**
     `FUN_10047130(1, RandomLong(20,40), port, port+vel-ish, fwd, 4.0, 1)` from the port.
   - no viewmodel entity: eject from the gun-position global `DAT_10190b28`, no smoke.
3. **Third-person / other players** (`thirdperson || !IsLocalOrSpectated`, `playerVisible[idx]`,
   shell set): same eject + smoke from **player attachment 3** (the shotgun eject port).
4. **ALWAYS (every viewer, incl. local):** `EV_PlaySound(**−1**, origin, CHAN_WEAPON,
   **wi+0x306** (the per-weapon rack sound: m3-pump / scock / spas12-pump), RandomFloat(0.92,1.0),
   0.8, 0, 25+75·timescale+RandomLong(−5,5))`. **ent = −1** — deliberately NOT the player entity,
   so the rack never channel-cuts the still-playing CHAN_WEAPON gunshot.

---

## 4. grenade.sc — `FUN_10042630` (server: GrenadeFire @0x732f0 / GrenadeIdle @0x74f18; iparam1 = anim-block index 19/20/21)

**Args read:** `iparam1` = **anim-block index** (19 pin-pull / 20 overhand / 21 underhand),
`entindex`. wi is **hardcoded `get_weapon_info(0x18)`** (24 = TSW_M61). 91 bytes total.

**Flow (whole handler):**
```
if (FUN_1001c5c0(entindex))                    // in-eye SPECTATING this player ONLY
    EV_WeaponAnimation( holdBlock(m61)[iparam1], body = 1 );
```
No sound, no effects. Exactly the draw.sc pattern: the thrower's own anim is the predicted
`SendWeaponAnim`; the event exists so an in-eye spectator sees the pin/throw. Note the
**hardcoded body = 1** on the m61 viewmodel.

---

## 5. firelinked.sc — `FUN_1003de10` (server: AkimboFire both-barrels; iparam1=weaponType, iparam2=attach mask, fparam1/2=spread, bparam2=seed)

The handler is a 33-byte wrapper: `FUN_1003f090(args, get_weapon_info(iparam1), linked=1)` —
the same **pistol/akimbo fire core** fire.sc uses for fire class 4/5 with `linked=0`
(fire.sc @0x3de40 dispatch on wi+0xd2: 4/5 → this core, 1 → shotgun `FUN_10040c20`,
else → standard `FUN_1003dfa0`).

**Core args (event wire contract):** `iparam1` weaponType, `iparam2` attach mask (bit0 =
silencer — kills the muzzle light), **`bparam1` (+0x40) = barrel side for SINGLE akimbo shots**
(0 → right barrel/attachment 2, 1 → left/attachment 0; ignored when linked), **`bparam2`
(+0x44) = the FireBullets random seed**, `fparam1/fparam2` (+0x30/0x34) = spread x/y fed
straight to FireBullets. AngleVectors → fwd `0x10189cb8`, right `0x10189e50`, up `0x10189d18`.

**How linked=1 differs from fire.sc (linked=0) — every divergence:**
1. **Viewmodel anim** (gate IsLocalOrSpectated): linked → `block[3 + RandomLong(0,2)]`
   (**[3]/[4]/[5]**, the both-barrels anims); single → bparam1 ? `block[22 + RandomLong(0,1)]`
   (left [22]/[23]) : `block[24 + RandomLong(0,1)]` (right [24]/[25]). Body = attach mask via
   `FUN_10007110` in both.
2. **Muzzleflash sprite** (`FUN_100474b0`): linked → drawn at **BOTH** attachments 0 and 2;
   single → at attachment (bparam1 ? 0 : 2) only.
3. **Shells:** linked → **TWO** casings — first-person from vm attachment[0] (+0xb60) AND
   attachment[2] (+0xb78), each − fwd·wi[+0x34c] (no-viewmodel fallback: `DAT_10190b28` ∓
   right·4.0); third-person from the player attachments. Single → one, from the fired barrel's
   attachment. Velocity (both): playerVel + fwd·(R2+50) + up·R2 + right·R1, where
   R2 = RandomLong(50,100) and **R1 = RandomLong(120,140), except the skorpion family
   (wi+0xcc == 17): RandomLong(−40,40)**. Bounce sound 1 (shellcasing); model wi+0xe8.
4. **Sound** (`FUN_1003de90`): linked plays **ONE** bang for both barrels — sample **wi+0x306**
   (the akimbo double-fire sound), vol **RandomFloat(0.84,0.92)**, attn **doubled**; single plays
   wi+0x2c6 at RandomFloat(0.92,1.0), attn = caliber table. Same timescale pitch either way.
5. **Bullets:** linked calls FireBullets **TWICE** — gunpos + right·5.0 (seed = bparam2) and
   gunpos − right·5.0 (**seed = bparam2 + 0x21**); single calls once at gunpos (seed = bparam2).
   Both volleys use fparam1/fparam2 spread and the wi ballistic fields (+0x1c/+0x20/+8/+0x2c/+0x24).
6. Muzzle dlight (`FUN_10048e60`, radius 192 @ origin + up·32 + fwd·16, suppressed by mask bit0)
   and the gun-smoke ribbons (16-length unsilenced; silenced: 8-length + a 1-puff at the
   silencer tip = attachment − fwd·wi[+0x34c]) are IDENTICAL in both modes (smoke per fired
   attachment).

---

## 6. Swap-contract summary

| event | iparam1 | iparam2 | fparam1/2 | bparam1 | bparam2 | vm anim (client) | sounds (client) |
|---|---|---|---|---|---|---|---|
| empty.sc | — | — | — | — | — | none | pistol-empty.wav, everyone |
| reload.sc | weaponType | attach mask | fparam1 = phase 0/8/9/10 | — | — | `hold[phase==0?1:phase]`, gate local-or-spec | 9: insert-shell (all); 10: wi+0x306 (non-local only); 0: generic_reload + brass/clips (non-local only) |
| pump.sc | weaponType | attach mask | — | — | — | `hold[13+RL(0,1)]`, gate local-or-spec | wi+0x306 @ ent −1, EVERYONE |
| grenade.sc | anim idx 19/20/21 | — | — | — | — | `hold(m61)[iparam1]` body **1**, in-eye spec ONLY | none |
| firelinked.sc | weaponType | attach mask | spread x/y | (unused) | seed | `hold[3+RL(0,2)]` | wi+0x306, vol 0.84–0.92, attn×2 — one bang, both barrels |
| fire.sc class 4/5 (ref) | weaponType | attach mask | spread x/y | barrel 0=R/1=L | seed | `hold[24/25]` R, `hold[22/23]` L | wi+0x2c6, vol 0.92–1.0 |

All sounds CHAN_WEAPON, attn 0.8 (except the fire core's caliber-table attn), pitch
25+75·timescale+RandomLong(−5,5). Brass: TS_EjectBrass life `DAT_1018ca54`, bounce 1/2;
smoke gated on `cl_fx`; third-person effects gated on `playerVisible[idx]`.

---

## 7. RECONCILE PLAN vs our reconstruction

Integration points: hooks in `cl_dll/hl/hl_events.cpp` (`Game_HookEvents` — currently hooks
fire/draw/knife/katana/coldcock/closecombat only), handlers in `cl_dll/ev_ts.cpp`; server
senders in `dlls/ts_gun.cpp`. Coverage REPORT already lists `events/pump.sc` +
`events/firelinked.sc` as MISSING from our build.

1. **empty.sc** — add `EV_TSEmpty` (the 5-line handler above) + hook. Then DELETE the server
   stand-in `EMIT_SOUND_DYN(..., CHAN_ITEM, "weapons/357_cock1.wav", ...)` inside
   `CTSGun::EmptySound` (@ts_gun.cpp ~2415): wrong sound, wrong channel, wrong side. Our
   event send (all params 0, FEV_NOTHOST) is already faithful; CTSGun is client-predicted so
   the local click comes from the predicted event. Keep the server-side contender early-out.
2. **reload.sc** — our server phase packing (0/8/9/10 in fparam1, iparam1=type,
   iparam2=attach) is already faithful. Add `EV_TSReload`; then REMOVE the stand-in
   `SendWeaponAnim(anim/reloadBase+N)` calls in `Reload`/`ShotgunReload` (the original leaves
   the anim entirely to the event — our predicted event playback would otherwise DOUBLE the
   anim) and the server `EMIT_SOUND insert-shell.wav` stand-in (the handler plays it, ungated,
   pitch-timescaled). VERIFY server-side: the sawed-off shell-retained reload probably ships as
   fparam1=8 (client: block[8], silent) — re-RE `Reload__6CTSGun` @0x74d5c before wiring our
   `iReloadKeepAnim` path into the event.
3. **pump.sc** — MISSING end-to-end. Server: precache `m_usPump` (gun+0x96a) in
   `CTSGun::Precache`, and in `ShotgunFire`'s rack half replace `SendWeaponAnim(pumpSeq)` +
   the CHAN_ITEM `EMIT_SOUND` with `PLAYBACK_EVENT_FULL(FEV_NOTHOST, ..., m_usPump, ...,
   iparam1=m_weaponType, iparam2=attach)`. Client `EV_TSPump` as §3. Notes: the original picks
   the rack anim CLIENT-side from block [13]/[14] (our hardcoded seq 8/2 came from the same
   models — the block gives it per-weapon + hold-stance for free); the rack sound field is
   wi+0x306 (our per-weapon m3-pump/scock/spas12-pump strings ARE that field's values — move
   them into `s_weapon_info` as `pszAltSound`); the original plays it at **ent −1** on
   CHAN_WEAPON, which is HOW it avoids cutting the gunshot (our CHAN_ITEM hack solved the same
   problem differently — switch to the original mechanism for drop-in fidelity).
4. **grenade.sc** — our server already sends iparam1=19/20/21 with our predicted
   `SendWeaponAnim(2/3/4)`; that matches the original split (local = predicted anim, event =
   spectator-only). Just add the tiny `EV_TSGrenade` (in-eye gate + block[iparam1] + body 1)
   for in-eye spectators. KEEP our SendWeaponAnim calls (unlike reload).
5. **firelinked.sc** — our `AkimboFire` merges the both-barrel shot into fire.sc with a
   `bparam2 = side (2 = both)` extension; the original client IGNORES that (bparam2 is the
   bullet SEED in the pistol core) — so against the original client our akimbo volley gets a
   wrong-but-harmless seed and only ONE muzzle flash/shell. Fix: precache `m_usFireLinked`
   (gun+0x96e); the both-barrels path sends `firelinked.sc` (iparam1=type, iparam2=attach,
   fparam1/2=spread, bparam2=shared seed); single-barrel akimbo shots keep fire.sc with
   **bparam1 = barrel side** (0 right / 1 left — NB for fire class 4/5 the client reads bparam1
   as the barrel, not our documented `clip==0` flag, which only applies to the non-akimbo
   dispatch paths) and **bparam2 = seed**, dropping our side-in-bparam2 extension. Client-side,
   `EV_TSFireLinked` = the fire core with linked=1 (anims 3/4/5, both attachments, ±right·5
   volleys with seed/seed+33, the wi+0x306 double-bang at 0.84–0.92 vol, attn×2). Our
   `EV_TSFire` should also gain the skorpion-family shell-velocity quirk (wi+0xcc==17 →
   right vel RandomLong(−40,40)) and the caliber-table attenuation when we tighten it.
6. **Shared plumbing to build once** for these handlers: TS_EjectBrass (§0.3), the player
   attachment cache, the playerVisible table, the gun-smoke ribbon + `cl_fx` cvar, and the
   per-weapon `wi+0x306` alt-sound + `wi+0xe8` shell-model + `wi+0x34c` eject-offset fields in
   our client weapon-info mirror.
