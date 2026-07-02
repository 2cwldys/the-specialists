# Dossier: client.dll melee/draw event handlers (knife.sc, katana.sc, draw.sc, coldcock.sc, closecombat.sc)

RE'd from `reference/client.dll` (no symbols) via the prebuilt `clientdll` Ghidra project
(`scripts/ghidra/dc.sh client 0x<RVA>`) + raw objdump disasm for the args the decompiler elided
(all engine calls go through untyped cached fn pointers, so Ghidra drops their arguments —
every sound/anim arg below was read from the pushes in the disassembly, addresses cited).
Raw captures: `ev_knife.c`, `ev_katana.c`, `ev_draw.c`, `ev_coldcock.c`, `ev_closecombat.c`
(this directory) + helper decompiles referenced inline.

| event | handler (Ghidra addr) | RVA |
|---|---|---|
| knife.sc | `LAB_10043c90` | 0x43c90 |
| katana.sc | `LAB_10044230` | 0x44230 |
| draw.sc | `FUN_100425b0` | 0x425b0 |
| coldcock.sc | `LAB_10043800` | 0x43800 |
| closecombat.sc | `FUN_10043200` | 0x43200 |

`event_args_t` offsets used below (real SDK layout — note `ducking` at +0x2c):
`flags`+0 · `entindex`+4 · `origin`+8 · `angles`+0x14 · `velocity`+0x20 · `ducking`+0x2c ·
`fparam1`+0x30 · `fparam2`+0x34 · `iparam1`+0x38 · `iparam2`+0x3c · `bparam1`+0x40 · `bparam2`+0x44.

---

## 0. Shared infrastructure (all five handlers)

### 0.1 Cached engine pointers (client.dll copies gEngfuncs into globals)
- `DAT_100d5b88` = **pfnAngleVectors**(angles, fwd, right, up) — proven by the 4 pushes at
  0x100439b8-cc (coldcock: angles=cached args+0x14, fwd `0x10189af0`, right `0x10189d28`, up `0x10189e80`).
- `DAT_100d5c34` = **pfnRandomFloat**, `DAT_100d5c38` = **pfnRandomLong**, (`DAT_100d5c3c` = pfnHookEvent —
  matches SDK order RandomFloat/RandomLong/HookEvent).
- `DAT_100d5c78` = **pEventAPI** (`event_api_t`): +4 `EV_PlaySound(ent,org,chan,sample,vol,attn,flags,pitch)`,
  +0x10 `EV_IsLocal(idx0)`, +0x18 `EV_LocalPlayerViewheight`, +0x20 `EV_IndexFromTrace`,
  +0x28 `EV_SetUpPlayerPrediction`, +0x2c `EV_PushPMStates`, +0x30 `EV_PopPMStates`,
  +0x34 `EV_SetSolidPlayers`, +0x38 `EV_SetTraceHull`, +0x3c `EV_PlayerTrace`,
  +0x40 `EV_WeaponAnimation(seq,body)`, +0x4c `EV_TraceTexture`.
- `DAT_100d5c7c` = **pDemoAPI** (+0 IsRecording, +4 IsPlayingback).
- `DAT_100d5c0c` = **PM_PointContents**, `DAT_100d5bb8` = GetMaxClients, `DAT_100d5bfc` = GetEntityByIndex.

### 0.2 `get_weapon_info` = `FUN_100071c0(type)`
`return &DAT_100aa050 + type*0x3dc` for `0 <= type < 0x26` (38 rows), else row 0.
**Client-side weapon_info stride = 0x3dc bytes** (not 0x358), table base `0x100aa050`.

### 0.3 The viewmodel **anim block** at `weapon_info+0x252` (and hold-stance twin at +0x28c)
A table of **shorts = viewmodel sequence numbers, indexed by logical anim id**:
`animblock[i] = *(int16*)(wi + 0x252 + 2*i)`, 29 entries (0x28c−0x252 = 58 bytes).
Known indices (from these handlers): **[2] = draw** (draw.sc reads `+4`), **[15..17] = melee swings**
(knife/katana), **[28] = coldcock** (`+0x38`). A **second full block at +0x28c** is the
**hold-stance (alternate grip) anim set** — same meaning as our server-side RE of
`CTSGun::SecondaryAttack` re-pointing the active anim block between wi+0x252/wi+0x28c
(`dlls/ts_weapon_info.h` iHoldChange comment). Selector helper:
- `FUN_1003df60(wi)`: if `pDemoAPI->IsPlayingback()` refresh flag `DAT_10189f9d` from the
  demo-restored global `DAT_100d6078`; return `wi+0x28c` if (`DAT_10189f9d && g_iUser1 != 5`)
  else `wi+0x252`. `DAT_10189f9d` = latched local hold-stance flag: read per frame from a
  per-player TS state float @+0x314 in the state-processing loop @~0x10003bbe (`ftol` of it),
  written into demos via Demo `WriteBuffer` type 4 when it changes while recording, and zeroed
  in a HUD reset @0x1004cd40. (VERIFY the exact +0x314 source struct; the +0x252/+0x28c
  hold-set semantics are corroborated by the server RE.)
Only **draw.sc** goes through this selector; knife/katana/coldcock/closecombat always index
**wi+0x252** directly (melee has no hold stance).

### 0.4 Locality gates
- `FUN_1001c5c0(idx)` = **in-eye-spectating idx**: `g_iUser1(DAT_10190b18)==5 && g_iUser2(DAT_10190b20)==idx`
  (both copied from local entity_state iuser1/iuser2 @0x1001bac4; TS's first-person spectate mode value is 5).
- `FUN_1001c5e0(idx)` = **IsLocalOrSpectated**: `pEventAPI->EV_IsLocal(idx-1) || FUN_1001c5c0(idx)`.

### 0.5 `FUN_1001c620(args, out, base)` = EV_GetGunPosition (TS flavor)
`out = base(args origin) + view_ofs`: default `(0,0,28)`; if idx valid and local(-or-spectated
non-in-eye) uses `EV_LocalPlayerViewheight`; in-eye-spectated uses target entity origin delta.

### 0.6 Melee sound pitch formula (identical at every EV_PlaySound site here)
```
pitch = RandomLong(-5,5) + 25 - ftol(g_flTimescale * -75.0)
      = 25 + 75*g_flTimescale + RandomLong(-5,5)         // = 100 ± 5 at normal time
```
`g_flTimescale` = `DAT_100963e8` (init 1.0, reset to 1.0 @0x100208a9, driven by the
TS slow-motion/bullet-time interpolator — sounds pitch DOWN in slow-mo). `-75.0` = const
@0x1007a904. Volume at every melee PlaySound = **RandomFloat(0.92, 1.0)**, attenuation **0.8**
(0x3f4ccccd), flags **0**, channel **1 (CHAN_WEAPON)**, ent = event entindex, origin = args origin.

### 0.7 Texture-impact helper `FUN_10045df0(idx, &tr, start, end)` → returns texture-type char
(the TS clone of `EV_HLDM_PlayTextureSound`; called by knife/katana/coldcock/closecombat on hit)
1. `PM_PointContents(tr.endpos) == CONTENTS_SKY(-6)` → return **0** (case "0" = silent).
2. `EV_IndexFromTrace(tr)` in `[1..MaxClients]` → return **'F'** (flesh) immediately, **no sound**
   (the server owns flesh-hit sounds).
3. Else `EV_TraceTexture(tr.ent, start, end)` → strip `-/+` prefix and `{ ! ~ space` → `PM_FindTextureType`.
4. Plays ONE random texture sound at `EV_PlaySound(0, tr.endpos, CHAN_STATIC(6), snd,
   vol, 0.8, 0, <pitch formula 0.6>)`:
   'D' dirt pl_dirt1-3 vol 0.9 · default/concrete pl_step1-2 0.9 · 'G' grate pl_grate1/4 0.9 ·
   'M' metal pl_metal1-2 0.9 · 'P'/'Y' glass debris_glass1-3 0.8 · 'S' slosh1/3/2/4 0.9 ·
   'T' tile1/3/2/4 0.8 · 'V' duct1(x2) 0.5 · 'W' wood debris_wood1-3 0.9. Returns the char.

### 0.8 Effect helpers (TS custom particle manager @`DAT_1018bde8`)
- `FUN_10046f50(pos, normal, "sprites/scintille_metal.spr", 5, N)` — metal-glint sprites placed
  along the surface normal (`pos + normal*step`, step from N*0.5), N = 6 (knife) / 12 (katana);
  framerate-ish fields 0.05/0.7 fixed.
- `FUN_100492b0(pos, normal, 7, 1.15, 25.0)` — **spark shower**: loads `sprites/sparks.spr`,
  count = RandomLong(3,7); each spark velocity = normal*17 + (RandomFloat(-25,25),
  RandomFloat(-25,25), RandomFloat(0,25)), scale RandomFloat(0.5,1.15)*1.15,
  life RandomFloat(0.1,1.0) (rendermode 3 if life>0.7 && density global > 0.9).
- `FUN_100464f0(pos, normal, sprite, 5, 12, 30)` — single **smoke puff**: at `pos + normal*6`
  (12*0.5), scale 3.3, framerate 7.0, brightness/alpha 30, vel (0,0,-2), used with
  `sprites/wood_smoke_add.spr` / `sprites/gun_smoke_add.spr`.
- `FUN_10002610` = Vector copy (MSVC by-value return plumbing — no behavior).

Every handler opens with 9 guard-bit `FUN_1006af63(<thunk>)` blocks — MSVC one-time
`atexit` registration for static locals; **all thunks are bare `ret`s (no behavior)**.

---

## 1. knife.sc — `LAB_10043c90` (server: KnifeFire, FEV_NOTHOST, iparam1=weaponType, iparam2=bloody)

**Args read:** `entindex`+4, `origin/angles/velocity` (+8/+0x14/+0x20, cached to globals),
`iparam1`+0x38 = **weaponType** (→ `get_weapon_info`, and its anim block wi+0x252 → `DAT_1018a044`),
`iparam2`+0x3c = **bloody flag** (→ `DAT_10189cc4`). fparams/bparams unused.

**Flow:**
1. `AngleVectors(angles)`; gunpos = GetGunPosition(origin); **end = gunpos + fwd*35.0** (0x1007a8d8).
2. `EV_SetUpPlayerPrediction(0,1)` (knife's call @0x10043ee8 consumes earlier stack slots —
   Ghidra renders `(0)` — same (0,1) idiom as katana/coldcock/closecombat), `EV_PushPMStates`,
   `EV_SetSolidPlayers(idx-1)`,
   `EV_SetTraceHull(2)` (point hull), `EV_PlayerTrace(gunpos, end, PM_STUDIO_BOX(2), -1, &tr)`.
3. `bloody = iparam2` (0x10043f3c). Branch on `tr.fraction == 1.0`:
   - **MISS** → play `weapons/knife/knife_miss.wav` (§0.6 formula: CHAN_WEAPON, vol
     RandomFloat(0.92,1.0), attn 0.8, pitch 100±5 scaled by timescale) @0x10044092.
   - **HIT** → `chr = FUN_10045df0(...)` (plays the texture sound itself, §0.7). Switch (jump
     table @0x10044134, byte-index @0x10044140):
     - `chr ∈ {0,'D','W'}` (sky/dirt/wood) → **nothing extra** (texture sound only), jump to anim.
     - `chr == 'F'` (flesh/player) → **`bloody = 1`** (overrides iparam2), **no sound, no sparks**.
     - **default** (concrete/metal/tile/…) → `scintille_metal.spr` glints (N=6) + spark shower
       `FUN_100492b0(…, 7, 1.15, 25.0)` + play **`weapons/knife/knife_hit.wav`** (same §0.6 params).
4. **Viewmodel anim** — gated `FUN_1001c5e0(idx) && !CL_IsThirdPerson()`:
   `swing = RandomLong(0,2)` → logical anim id **15/16/17 chosen CLIENT-side** (0x100440c5-f4);
   `EV_WeaponAnimation( animblock252[swing_id], body = bloody )` (0x100440fe-1b:
   `movswl (wi+0x252)[id*2]`, body push = `DAT_10189cc4`).
5. `EV_PopPMStates`.

**State written:** only the handler-local globals (bloody `DAT_10189cc4`, anim id `DAT_10189adc`,
cached args). No persistent bloody latch client-side — **the bloody viewmodel = BODY 1 of the
knife viewmodel**, re-sent by the server every swing in iparam2 (and server sets `pev->body=1`
on the world model; a flesh hit blooms it client-side immediately via the 'F' branch).

---

## 2. katana.sc — `LAB_10044230` (server: KatanaFire, FEV_NOTHOST, iparam1=vmAnim 15/16/17, iparam2=bloody)

**Args read:** `entindex`, `origin/angles/velocity` (cached), `iparam1` = **logical anim id 15/16/17
from the server**, `iparam2` = **bloody**. Weapon info is **hardcoded `get_weapon_info(0x22)`**
(type 34 = katana) — iparam1 is NOT a weapon type here.

**Flow:**
1. AngleVectors; gunpos. **Trace direction follows the server's swing table**:
   `iparam1 == 15` (back slash) → **end = gunpos − fwd*55.0** (0x1007a8f4; traces BEHIND, matching
   the server's reach-55 negative-damage back slash); else (16/17) → **end = gunpos + fwd*50.0**
   (0x1007cd74).
2. Same trace setup as knife: SetUpPlayerPrediction(0,1) (`push 1; push 0` @0x10044509),
   PushPMStates, SetSolidPlayers(idx-1), SetTraceHull(2), PlayerTrace(gunpos, end, 2, -1, &tr).
3. `bloody = iparam2`. `tr.fraction == 1.0`?
   - **MISS** → `RandomLong(0,1)`: 1 → **`weapons/katana/miss.wav`**, 0 → **`weapons/katana/miss1.wav`**
     (§0.6 params) @0x1004462e-44.
   - **HIT** → texture helper (plays material sound); switch (@0x100446fc):
     `{0,'D','W'}` → nothing extra; `'F'` → `bloody = 1`, silent; **default** →
     scintille_metal glints (**N=12**) + sparks(7, 1.15, 25.0) + `RandomLong(0,1)`:
     1 → **`weapons/katana/hit.wav`**, 0 → **`weapons/katana/hit1.wav`** (§0.6 params).
4. **Viewmodel anim** — gated `FUN_1001c5e0(idx) && !CL_IsThirdPerson()`:
   `EV_WeaponAnimation( animblock252(katana)[iparam1], body = bloody )` (0x100446c9-e6) —
   **iparam1 is REMAPPED through the wi+0x252 short table, not used as a raw sequence**
   (indices 15/16/17 → shorts at wi+0x270/0x272/0x274).
5. PopPMStates. Same bloody model: **body 1**, no persistent client latch.

---

## 3. draw.sc — `FUN_100425b0` (server: Deploy epilogue, iparam1=weaponType, iparam2=attachEquipped)

**Args read:** `iparam1` = weaponType (→ `get_weapon_info`), `iparam2` = **barrel/attachment mask**,
`entindex`. Nothing else — no origin/angles, **no sound, no effects, no trace** (123 bytes total).

**Flow (whole handler):**
```
if (FUN_1001c5c0(entindex)) {                    // in-eye SPECTATING this player ONLY
    block = FUN_1003df60(wi);                    // wi+0x252, or wi+0x28c if hold-stance latched
    body  = FUN_10007110(type, iparam2);         // attachment mask -> viewmodel body (§3.1)
    EV_WeaponAnimation( *(int16*)(block+4), body );   // animblock[2] = draw sequence
}
```
**The local player is deliberately NOT gated in** — the shooter's own draw anim comes from the
predicted `Deploy` (`SendWeaponAnim`); this event exists so an **in-eye spectator** sees the
draw. Anim = **anim-block entry [2]** via the hold-stance-aware selector (§0.3) — the only
handler of the five that consults the +0x28c table.

### 3.1 `FUN_10007110(type, mask)` — attachment mask → viewmodel body
Shorts at wi+0x128/0x12a/0x12c/0x12e = per-bit body offsets for mask bits 1/2/4/8
(silencer/laser/flashlight/scope per our attachment-bit map), wi+0x130 = special combined
offset used when bits 2|4 are BOTH set and it isn't -1 (laser+flashlight combo body):
```
body = 0
if (mask&1) body += s128
if ((mask&2)&&(mask&4)&&s130!=-1) body += s130
else { if (mask&2) body += s12a;  if (mask&4) body += s12c }
if (mask&8) body += s12e
else if (type == 0x24) body += 1        // type 36 quirk: +1 body when no scope bit
```

---

## 4. coldcock.sc — `LAB_10043800` (server: ColdCock/AlternateAttack; iparam1=weaponType, iparam2=attachment mask)

**Args read:** `iparam1` = weaponType (→ wi + its +0x252 block), `iparam2` = **attachment/barrel
mask** (NOT bloody — it goes through `FUN_10007110` exactly like draw.sc), `entindex`,
`origin/angles/velocity` (cached). No fparams.

**Flow:**
1. **Viewmodel anim FIRST** — gated `FUN_1001c5c0(idx) || FUN_1001c5e0(idx)` (= local OR
   spectated; **NO third-person check**, unlike knife/katana):
   `EV_WeaponAnimation( animblock252[28], FUN_10007110(type, iparam2) )` — **hardcoded logical
   index 28** (`movswl 0x38(wi+0x252)` @0x10043a2b), body keeps the gun's attachments visible.
2. gunpos; **end = gunpos + fwd*30.0** (0x1007a874).
3. **Whoosh ALWAYS** (before the trace, hit or miss): `EV_PlaySound(idx, origin, CHAN_WEAPON,
   "player/closecombat.wav", RandomFloat(0.92,1.0), 0.8, 0, <pitch §0.6>)` @0x10043b13.
4. SetUpPlayerPrediction(0,1), PushPMStates, SetSolidPlayers(idx-1), SetTraceHull(2),
   PlayerTrace(gunpos, end, 2, -1, &tr).
5. If **hit** (`fraction != 1`): `chr = FUN_10045df0(...)` (texture sound, §0.7), then smoke puff
   `FUN_100464f0(endpos, normal, sprite, 5, 12, 30)` with
   `'D'/'W'` → `sprites/wood_smoke_add.spr`; `'F'` → **nothing** (skip effect);
   default → `sprites/gun_smoke_add.spr`. **No sparks, no weapon hit sound, no bloody logic.**
6. PopPMStates. **No state written** beyond the arg cache.

---

## 5. closecombat.sc — `FUN_10043200` (server: CCombatAttack/CCombatBlock via gun+0x96c)

**Args read (this nails the wire contract the server-side analysis left VERIFY):**
- `iparam1` = **move id**: 0 = BLOCK, 1(/default) = punch, 2 = second strike, 3 = back move.
- `iparam2` = **anim-block index** — the client plays `animblock252[iparam2]` raw.
- `fparam1` (+0x30, `flds 0x30(esi)` @0x100433bb) = **weaponType as a float** → `ftol` →
  `get_weapon_info` (kung-fu is TSW_MELEE but the type still travels in fparam1).
- `entindex`, `origin/angles/velocity` (cached). iparam2 also read into `edi` @0x100433b8.

**Flow:**
1. **Viewmodel anim FIRST** — gate `FUN_1001c5c0(idx) || FUN_1001c5e0(idx)` (no 3rd-person check):
   `EV_WeaponAnimation( *(int16*)(wi + 0x252 + iparam2*2), body = 0 )` @0x100433f0-401.
2. gunpos.
3. `iparam1 == 0` (**BLOCK**) → play `player/block.wav` (CHAN_WEAPON, RandomFloat(0.92,1.0),
   0.8, 0, pitch §0.6) @0x10043750 and **return** (no trace, no effects).
4. Otherwise pick reach (all along `fwd` from gunpos):
   - `iparam1 == 3` → **end = gunpos + fwd*(-55.0)** (0x1007d6fc — traces BEHIND: the back kick),
   - `iparam1 == 2` → **fwd*45.0** (0x1007a900),
   - else (1/other) → **fwd*35.0** (0x1007a8d8).
5. **Whoosh ALWAYS**: `player/closecombat.wav` (same params as coldcock) @0x1004360c.
6. SetUpPlayerPrediction(0,1), PushPMStates, SetSolidPlayers(idx-1), SetTraceHull(2),
   PlayerTrace(gunpos, end, 2, -1, &tr).
7. On **hit**: texture helper (material sound) + the same smoke puff as coldcock
   (`'D'/'W'` wood_smoke_add, `'F'` nothing, default gun_smoke_add; `FUN_100464f0(…,5,12,30)`).
8. PopPMStates. No state written.

---

## 6. Swap-contract summary (what our reconstruction must emit/consume)

| event | iparam1 | iparam2 | fparam1 | vm anim source (client) | body | sounds (client) |
|---|---|---|---|---|---|---|
| knife.sc | weaponType | bloody 0/1 | — | `wi252[RandomLong(0,2)+15]` (client picks swing) | bloody (forced 1 on flesh) | miss: knife_miss.wav; hard hit: knife_hit.wav + sparks; wood/dirt: texture snd only; flesh: silent |
| katana.sc | vmAnim id 15/16/17 | bloody 0/1 | — | `wi252(katana)[iparam1]` (REMAPPED, not raw seq; 15 traces 55 BEHIND, 16/17 50 fwd) | bloody | miss/miss1.wav, hit/hit1.wav (50/50), sparks on hard |
| draw.sc | weaponType | attach mask | — | `activeblock[2]` (hold-aware +0x252/+0x28c) | `FUN_10007110(type,mask)` | none |
| coldcock.sc | weaponType | attach mask | — | `wi252[28]` | `FUN_10007110(type,mask)` | player/closecombat.wav always (whoosh) |
| closecombat.sc | move 0..3 (0=block) | anim-block index | weaponType (float) | `wi252[iparam2]` | 0 | block: player/block.wav; else closecombat.wav always |

Gating: knife/katana anims = `IsLocalOrSpectated && !thirdperson`; coldcock/closecombat anims =
`IsLocalOrSpectated` (no 3p check); **draw anim = in-eye spectator ONLY** (local uses predicted
Deploy). All melee EV_PlaySounds: CHAN_WEAPON, vol RandomFloat(0.92,1.0), attn 0.8,
**pitch = 25 + 75*timescale + RandomLong(-5,5)** (timescale global = bullet-time scale, 1.0 normal).
Texture-impact sounds/CHAN_STATIC come from the shared helper for every non-flesh hit (knife's
"the server doesn't send a whoosh because the client plays it" holds: miss whoosh for
knife/katana is client-side; coldcock/closecombat whoosh is client-side and unconditional).
Bloody viewmodel = **body 1 via EV_WeaponAnimation's body arg** (iparam2, or client-forced on a
flesh hit); persistence across draws is server-side (knife draw would need the server to carry
it — draw.sc's iparam2 is the attachment mask, and knives have no attachments; the original
server re-sends bloody per swing and sets world `pev->body`).

Constants cheat-sheet: knife reach 35 · katana 50 (back 55) · coldcock 30 · closecombat 35/45/−55 ·
trace = point hull 2, PM_STUDIO_BOX, ignore −1 · sparks 3–7 @spread 25 vel normal*17 ·
glints 6 (knife) / 12 (katana) · smoke puff scale 3.3 fr 7 alpha 30.
