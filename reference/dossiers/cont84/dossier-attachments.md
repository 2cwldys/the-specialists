# Dossier: the TS weapon-ATTACHMENT system (silencer / laser / flashlight / scope) + the player gun-anchor cache

RE'd cont84 from `reference/ts_i386.so` (server, gcc2 symbols, typed `re` project) and
`reference/client.dll` (no symbols, `clientdll` project + raw objdump). Companion to
`reference/dossiers/cont83/dossier-ev-guntail.md` (event wire contract) and cont81 §0.
Raw decompile captures: `att_srv_*.c` / `att_cli_*.c` in this directory.

Attach bits (given, confirmed everywhere below): **1 silencer · 2 laser · 4 flashlight · 8 scope**.
Address conventions: server addrs are `nm` addrs (Ghidra = +0x10000); client addrs are
`0x10000000+RVA`. GOT base of the `.so` = 0x165E10; `.rodata` vaddr == file offset.

## 0. The one struct that runs it all: `s_weapon_info`, stride 0x3dc, SAME layout in both binaries

The server's `global_weapon_info` array lives at `.data:0x17db40` (38 × 0x3dc); the client's at
`0x100aa050` (`get_weapon_info` = FUN_100071c0). `Write_<weapon>()` (shared source) fills both.
Fields recovered/confirmed this session (offsets within one entry):

| offset | field | evidence |
|---|---|---|
| +0x3c | **zoomFov1** (int) | SecondaryAttack zoom cycle (§3) |
| +0x40 | **zoomFov2** (int) | SecondaryAttack zoom cycle |
| +0x86 | HUD sprite path | Write_aug store map |
| +0xdc | byte, **multi-buy flag** (1 = knife/m61-style stackable) | only `Write_knife`/`Write_m61` set 1 (`0x77528`, `0x77e62`); drives the buy-menu token format (§2) |
| +0x128 | short, **body offset: silencer** | GetBodyByItems (both binaries, §5) |
| +0x12a | short, **body offset: laser** | " |
| +0x12c | short, **body offset: flashlight** | " |
| +0x12e | short, **body offset: scope** | " |
| +0x130 | short, **body offset: laser+flash combined** (−1 = none; used when (mask&6)==6) | " |
| +0x132 | v_ model path | Deploy/Unzoom |
| +0x172 | **p_ model path** | Deploy |
| +0x1b2 | **p_ model path, SILENCED** (empty for non-sil weapons) | Deploy chooser (§1) |
| +0x1f2 | w_ model path | Write_ store map |
| +0x252 | anim block (11 shorts) — normal stance | cont81/83 |
| +0x28c | anim block — HOLD/tilt stance twin (first short −1 = weapon has no hold stance) | §3 |
| +0x2c6 | fire sound | cont83 + §1 |
| +0x306 | **SILENCED fire sound** (shotguns: the pump/rack sound — disjoint sets) | §1/§5 correction |

There is **no sprintf `p_%s_sil` composition anywhere** — every `_sil` path is a literal string
copied by its `Write_` function.

---

## 1. The p_ model `_sil` swap (server)

### EVIDENCE
All 16 `models/p_*_sil.mdl` strings are referenced exactly once each — inside their
`Write_<weapon>` function, copied to **wi+0x1b2** (e.g. Write_aug @`0x5df2b`:
`lea -0x6f3a0(%ebx)` = 0xf6a70 "models/p_aug_sil.mdl" → `lea 0x1b2(%eax)`).
Runtime readers of the +0x172/+0x1b2 pair (the only two `add $0x172/$0x1b2` sites):

**`CTSGun::Deploy` @0x71030** (att_srv_Deploy.c):
```c
if ((this->m_pActiveItem->attachEquipped & 1) == 0)
    pcVar3 = &this->m_pActiveInfo->szPlayerModel;      // wi+0x172
else
    pcVar3 = &this->m_pActiveInfo->field_0x1b2;        // wi+0x1b2  p_X_sil.mdl
CBasePlayerWeapon::DefaultDeploy(this, &wi->szViewModel /*+0x132*/, pcVar3,
    drawAnim /* = short(item->pAnimBlock[2]) */, &wi->szAnimExt /*+0x232*/, 0, this->pev->body);
```
Just before that, `pev->body = GetBodyByItems(m_weaponType, item->attachEquipped)` (see §5), and
just after, the **draw event** (gun+0x976) is played with flags=1 (FEV_NOTHOST),
iparam1=weaponType, iparam2=attachEquipped.

**`TSPrecache` @0x8f320** (second reader, `0x8f637`/`0x8f678`): precaches BOTH +0x172 and +0x1b2
for every weapon.

### VERDICT / ANSWER
- Rule: **p_ model = wi+0x1b2 iff `items[type].attach & 1` (silencer bit), else wi+0x172** —
  chosen in `Deploy` only, i.e. the swap takes effect on (re)deploy. `AddItem` @0x70794's
  reselect/Deploy (known fact) is what makes a mid-life silencer purchase show immediately.
- Model string is never composed; it is a per-weapon literal in `Write_` (a data-table field).
- Weapons with a DISTINCT `_sil` p-model: **aug, beretta, beretta_akimbo (p_berettas_sil),
  desert, fnh, fnh_akimbo, glock18, glock22, m4, mac10 (uses `p_ump_sil.mdl` — asset-name
  quirk), mk23, mk23_akimbo, mp5k, pdw, uzi, uzi_akimbo**.
- **mp5sd and ruger copy their NORMAL p_ model into +0x1b2 too** (integrally-silenced look —
  the field is always valid for them). All other weapons zero-terminate +0x1b2 (no sil).
- Bonus (same Write_ map): the **silenced FIRE SOUND** lives at wi+0x306
  ("weapons/aug/aug-fire-sil.wav" etc.; mp5sd stores its normal fire sound in both slots). See §5.

CONFIDENCE: high (disasm + typed decompile; single choose-site).

---

## 2. The buy/apply grammar (original client, buy-menu Apply = FUN_10039400)

### EVIDENCE (att_cli_buymenu_apply.c + disasm of the sprintf sites)
Preconditions: total cost @menu+0x414 vs cash (`FUN_10036640`) → "#Buy_NoCash"; empty →
"#Buy_ConfigEmpty"; weight @+0x418 vs space (`FUN_10036660`). Then a bubble-sort pass reorders
the 15 weapon slots (stride 0x38 bytes @menu+0x98).

Commands sent via `pfnClientCmd` (`DAT_100d5b74`), literal prefixes @0x10092a64..7c:
1. **`rst_cfg`** — sent FIRST. Server side (`ClientCommand` @0x9def8):
   `strcmp("rst_cfg")` → **`CBasePlayer::ResetArmament()`** (clears the queued armament).
2. **`tkw `** + one token per weapon slot, format **`%i_%i `**:
   - normal gun (wi+0xdc == 0): token = **`<type>_<attach>`** (attach = slot+4);
     if the slot's "bought double" flag (slot+0x2c) is set, an EXTRA token **`<type>_0`** is
     appended — the second gun of an akimbo pair carries NO attach bits (the server's
     `AddItem` mirrors attach onto the pair).
   - multi-buy item (wi+0xdc == 1, knife/m61 grenade): token = **`<type>_<count>`** (slot+0x30).
3. **`tki `** + **`%i `** per equipment item (slots whose isItem char at slot−4 ≠ 0).
4. **`tka `** + **`%i_%i `** per ammo slot = **`<caliberIndex>_<amount>`** (16 slots @menu+0x14,
   stride 8, 0-based caliber index into the 16-row caliber table).

Send order is exactly **rst_cfg → tkw → tki → tka** (one ClientCmd per buffer).

Saved-config path: after sending, prints "#Buy_UsingNewConfig"/"#Buy_UsingLastConfig" (flag
@menu+0x2f08) and does a plain **in-memory memcpy of 0x102 dwords (menu+0x14 → menu+0x41c)** —
the "last config". **No file is written.** The live original's game dir
(`~/.local/share/Steam/steamapps/common/Half-Life/ts`, hl.exe running, PID cwd verified) contains
**no cfg with tkw/tki/tka lines** (grepped every *.cfg + all text files) — consistent. A menu
label `"5 #Buy_SaveConfig"` exists (@0x10094f50); whatever it does, it produced no loadout file
here (see UNRESOLVED).

CONFIDENCE: high for the wire grammar (decompile + disasm arg-level); medium on the
double-buy/count field split (slot+4 vs slot+0x30 read straight from disasm, but the menu's
slot-filling code was not RE'd).

---

## 3. Right-click gating — `CTSGun::SecondaryAttack` @0x717a8, complete tree

### EVIDENCE (att_srv_SecondaryAttack.c)
```
1. m_pActiveItem == NULL                  -> CCombatAttack(this)            (bare fists)
2. wi->iFireType == 2  (grenade)          -> item->pumpToggle=1; GrenadeFire
3. wi->iFireType == 3  (knife)            -> item->pumpToggle=1; KnifeFire
4. wi->iFireType == 10 (katana)           -> KatanaThrow
5. wi->iFireType == 11                    -> return (nothing)
6. item->fireMode == 4 (akimbo semi):     -> Fire(this)  ONLY IF (m_afButtonPressed & IN_ATTACK2
                                             (0x800)) or DoubleFireRate(player); else return
7. item->fireMode == 5 (akimbo full-auto) -> Fire(this) unconditionally
8. otherwise (a normal gun):
   a. (item->attach & 8) == 0  (NO SCOPE) — the HOLD/TILT stance toggle:
      only if the weapon HAS a hold block (short wi+0x28c != -1):
        toggle item->holdPosition; SendWeaponAnim(short(item->pAnimBlock[7]), 1, pev->body);
        item->pAnimBlock = holdPosition ? &wi->0x28c (hold twin) : &wi->0x252 (normal);
   b. (item->attach & 8) != 0  (SCOPE) — the ZOOM CYCLE:
        fov==0          -> if (wi->zoomFov1) Zoom(zoomFov1)          // 0 -> fov1
        fov!=0          -> if (wi->zoomFov2==0 || fov==zoomFov2) Unzoom()   // fov2 (or fov1 when
                           else Zoom(zoomFov2)                       //  no fov2) -> 0; fov1 -> fov2
   then (both a and b): m_flNextPrimaryAttack = time+0.3; (+0xa8) = time+1.0.
```
**`Zoom(fov)` @0x71710**: `player->pev->fov = fov; if (wi->zoomFov1 < 40) pev->viewmodel = 0;`
(true sniper scopes hide the viewmodel). **`Unzoom` @0x71748**: `fov = 0`; restores
`pev->viewmodel` from wi+0x132 if it was hidden. Unzoom is also called from **SecondaryAttack**
(the cycle), **ColdCock** @0x71eca, **ShotgunReload** @0x74af9, **Reload** @0x74ea4, and
**HolsterWeapon** @0x756e2 — reloading/whipping/holstering always unzooms. Zoom's ONLY caller
is SecondaryAttack.

### VERDICT
- The scoped deagle (desert stores zoomFov2=... and scope body +2) right-click **zooms** because
  scope is `attach & 8`; the unscoped deagle falls into 8a — the "aim-down-center tilt" IS the
  hold-stance toggle (anim slot [7] of the current block + the block-pointer swap; every
  subsequent anim, fire included, then comes from the HOLD twin block at wi+0x28c — this is the
  same hold mechanism cont81 §0 found in the fire path).
- Akimbo right-hand/left-hand split is NOT here: fireMode 4/5 just calls `Fire()` (the barrel
  alternation lives in the fire path); mode 4 is press-gated (semi) unless the DoubleFireRate
  powerup, mode 5 autofires.
- Scope is **static**: nothing in the tree can remove/toggle bit 8 — it only changes what
  right-click DOES. Same for silencer (bit 0 is never touched here). Matches the user's
  "silencer/scope are static capabilities".

CONFIDENCE: high (typed decompile, complete function).

---

## 4. The laser beam + the player gun-anchor cache (client)

### 4a. The cache @0x1018caf8 — WHO WRITES IT, WHAT IT IS

**Writer: FUN_10056e70** (att_cli_attcache_writer.c) — the ONLY writer (`0x10056ec0` is the sole
store site). Per frame, for player indices 0..MaxClients: get the player's `cl_entity`
(`DAT_100d5bfc = GetEntityByIndex`); if `ent && ent->model`:
```c
cache[idx][0..3] = ent->attachment[0..3];   // ent+0xb60..+0xb8c, 4 vec3s, 48 bytes/player
```
It stores **positions only** (the engine's studio code computes `cl_entity.attachment[4]` when
the model is drawn) and it reads them off the **PLAYER entity** — i.e. `player.mdl`'s
attachments, NOT the p_ weapon submodel (ground truth: every `p_*.mdl` checked has **0**
attachments; `player.mdl` has 4: att0=(15,1.5,0) bone29, att1=(20,2,0) bone29, att2=(15,1.5,0)
bone20, att3=(30,3,0) bone29 — att0/1/3 are **collinear points along the gun-hand bone 29**,
att2 is the LEFT-hand (akimbo) gun on bone 20).

**Call chain**: export **`HUD_DrawTransparentTriangles` (0x10034e50) → FUN_10056dd0** (per-frame
effects dispatcher; entry gates on the in-level byte 0x1018d16a and cvar `cl_fx`) →
`FUN_10056f60` → **FUN_10056e70 (cache refresh)** → … → `FUN_1003de00` = `{ FUN_1003bb00
(flashlights + LOCAL laser); FUN_1003d600 (remote lasers) }`. So the cache is refreshed in the
transparent-triangles pass, after the engine has posed/drawn the players, right before the beams
draw — and the same cache feeds the event handlers' third-person effects (cont83 §0.3,
`FUN_10056be0(out, idx, n)` = copy of `cache + (idx*4+n)*12`).

**Why the beam sweeps with the gun during reload**: v_ models carry the same trick — every
`v_*.mdl` checked has 4 attachments as 2+ collinear points along the barrel bone (e.g.
v_glock18: (3,−6,0)/(3,−12,0)/(3,−18,0)/(3,−24,0) all on bone 35; v_uzi_akimbo: att0/1 on bone
35 (right gun), att2/3 on bone 40 (left gun)). Direction is derived as **attachment[1] −
attachment[0]** (and [3]−[2] for the second gun), so when a reload/tilt animation rotates the
gun bone, the derived direction rotates with it. No angles are cached — orientation is
reconstructed from the two points.

### 4b. The laser renderers

**Remote players — FUN_1003d600** (att_cli_laser_remote.c), sprites `laserdot.spr` +
`laserbeam.spr`; registers 6 cvars lazily (thunks 0x1003ddf0..0x1003dda0 — `cl_laser`,
`cl_lasercolor` are in this family). Per player idx (skipping the local player):
- Gates: **ActItems byte `DAT_1018d128[idx*2]` & 2** (laser ACTIVE), entity valid+modeled,
  `playerVisible[idx]` (FUN_100579a0).
- Start (`FUN_10068e20` → DAT_10189960): the player **entity origin** (ent+0xb48) with a stance
  Z-adjust (dead/short-sequence → z−const & roll 80; ducking (+0x378==1) vs standing → different
  +z consts) — an approximate gun-height point, plus small random jitter on the out-angles.
- **Direction = normalize(cache[idx][1] − cache[idx][0])** — the two bone-29 anchor points.
  Per-player smoothed state @DAT_10189638[idx*3]: `dir += (newDir − dir) * min(frametime *
  cvar, cap)`, with a hard SNAP to newDir when `dot(old,new) < threshold` (fast flicks don't
  lag). Then normalized again.
- `PlayerTrace(start, start + dir * 8192-ish)`; **beam** = triapi quad, `laserbeam.spr`,
  RenderMode 5 (TransAdd), width 4.0, from start to `traceEnd − 2*dir`.
- **Dot**: `laserdot.spr` at `traceEnd − dir*const`, size scaled by distance from
  `cache[idx][0]` (FUN_10035b10), drawn only if the trace hit.
- **Color** (`FUN_100582f0`, from byte `DAT_1018d128[idx*2+1]`): 1 → green (0,205,0),
  2 → blue (0,120,255), else → **red (255,25,25)**. That byte is written by the **`PlInfo`**
  user message (handler 0x1004b7e0 via thunk 0x10049b40): 3 bytes `(idx, subkey, value)`,
  only `subkey==0` → laser color. (`ActItems` writes byte [idx*2]; `PlInfo` writes [idx*2+1].)

**Local player — inside FUN_1003bb00** (att_cli_flashlight_locallaser.c, which also draws
flashlight beams — gated on ActItems bit 4 @0x1003b815 and the gHUD+0x14 flags): sprite
**`laserbeamanim.spr`** (animated frames advanced by `frametime * cvar`), color from the SAME
color byte (local idx). Start = **viewmodel entity attachment[0]** (`DAT_101914a0+0xb60`),
direction = **vm attachment[1] − attachment[0]** (`+0xb6c − +0xb60` per component);
for akimbo a SECOND beam from vm attachment[2] with direction `attachment[3] − attachment[2]`
(`+0xb84 − +0xb78`). Same trace + quad + dot pattern.

### 4c. The third-person muzzle-flash anchor
Confirmed: the standard-fire path's third-person `FUN_100474b0` call @0x1003e3ab is fed
`FUN_10056be0(out, idx, **0**)` (explicit `push $0x0` @0x1003e393) — **cache attachment 0** =
the player's right-gun muzzle bone point. The pistol/akimbo core (cont83 §5.2 stands): single
akimbo shot → attachment (bparam1 ? 0 : 2), linked → both 0 and 2. Shotgun pump port =
attachment 3 (cont83 §3).

CONFIDENCE: high on writer/reader/direction/gates (decompiles + model ground truth); medium on
the smoothing constants (cvar-driven, names not individually resolved).

---

## 5. Viewmodel vs others: bodies, WeaponInfo, ActItems

### GetBodyByItems — server @0x6f0a4 == client FUN_10007110, byte-identical logic
(att_srv_GetBodyByItems.c; client disasm read directly)
```c
body = 0;
if (mask & 1)              body += wi->silBody;            // +0x128
if ((mask & 6) == 6 && wi->laserFlashBody != -1)           // +0x130 combo group
                           body += wi->laserFlashBody;
else { if (mask & 2)       body += wi->laserBody;          // +0x12a
       if (mask & 4)       body += wi->flashBody; }        // +0x12c
if (mask & 8)              body += wi->scopeBody;          // +0x12e
else if (type == 36)       body += 1;                      // contender: UNscoped body is +1
```
Per-weapon values (from Write_): glock18/mk23 sil 1, laser 2, flash 2, combo 2; m4 sil 1,
laser 4, flash 2, scope 8 (additive body groups); desert sil 1, scope 2; contender scope 2;
mp5sd/mac10/benelli scope(=lasersight rail?) 1; barrett/ruger all 0. (The prior "+0x178..+0x180"
note was these very fields — see §6.)

### Who uses which mask
- **Server Deploy**: `pev->body = GetBodyByItems(type, item->attachEquipped)` and passes it to
  DefaultDeploy/SendWeaponAnim — the EQUIPPED mask.
- **Client event handlers** (fire/reload/pump/draw…): `FUN_10007110(iparam1, iparam2)` — the
  EQUIPPED mask from the event.
- **Client per-frame viewmodel updater FUN_10068f70** (att_cli_viewmodel_update.c): when the
  viewmodel MODEL changes it recomputes `DAT_10191d5c = FUN_10007110(type,
  ActItems_ACTIVE_byte[local])` — the ACTIVE mask. (No other reader of 0x10191d5c was found by
  objdump scan — likely a field inside the static viewmodel entity written by address; VERIFY.)

### The WeaponInfo message (handler FUN_1004f800, this = gHUD-obj 0x100d0ca0)
Wire (READ_BYTE = FUN_10029a10, READ_SHORT = FUN_10029a50):
**BYTE weaponType · BYTE clip · SHORT ammo[caliber] · BYTE fireMode · BYTE attachFlags** —
stored at +0x7b8 (type; −1 keeps previous), +0x78c (clip), +0x748[wi->caliber] (ammo),
**+0x78a (fireMode)**, **+0x798 (attach)**. NB the 4th byte is FIRE MODE, not attach — +0x78a
feeds the HUD fire-mode label switch (`#HUD_SemiAuto/#HUD_Burst/#HUD_PumpAction/#HUD_FreeSemi/
#HUD_FreeFull/#HUD_FullAuto` in FUN_1004d7e0) and the `weapons/switch.wav` click on change
(FUN_1004d350, compares vs cached +0x7bc).
**The 5th byte (+0x798, attach) is read ONLY by the ammo-panel HUD draw FUN_1004d7e0**
(@0x1004d881) — i.e. it drives HUD attachment display, NOT models/bodies/effects. (A per-frame
mirror FUN_100581c0 also refreshes +0x78a/+0x78c/+0x798 from the local weapon-data records:
rec+0x2/rec+0x4/rec+0x10.)

### The ActItems message (handler FUN_1004fa70)
Wire: **BYTE playerIdx · BYTE activeMask** → `DAT_1018d128[idx*2] = activeMask` (0 ≤ idx < 33)
+ HUD dirty flag. **That is ALL it does** — no p_ model swap, no body change. Its byte gates the
laser (bit 2) and flashlight (bit 4) renderers (§4) and, for the LOCAL player, the
viewmodel-body recompute above. Third-person silencer visuals come purely from the server's
`pev->weaponmodel` swap (§1); scope/silencer have no active state (static bits), matching the
user's description.

CONFIDENCE: high (all handlers fully decompiled; reader sets enumerated by address scan).

---

## 6. "Draw-anim offsets by mask" — resolved (it was bodies, not anims)

**FUN_100425b0** (the draw.sc handler, att_cli_ev_draw.c) in full:
```c
type = iparam1; wi = get_weapon_info(type); mask = iparam2; idx = entindex;
if (IsLocalOrSpectated(idx)) {
    block = FUN_1003df60(wi);                  // hold-aware: wi+0x252 or +0x28c
    body  = FUN_10007110(type, mask);          // GetBodyByItems
    WeaponAnim(short(block[2]), body);         // anim slot [2] = draw, ALWAYS
}
```
The draw ANIMATION is always anim-block slot [2] (hold-aware). **The attach mask never selects a
different draw anim — it selects the BODY.** The prior finding's "+0x178..+0x180 fields" were
the absolute addresses `0x100aa178..0x100aa180` = weapon 0's **wi+0x128..+0x130 body-offset
fields** (client array base 0x100aa050), i.e. exactly the GetBodyByItems table of §5. There is
no mask→anim mapping to port.

Server side of draw: `Deploy` plays DefaultDeploy(block[2]) for the owner and sends the draw.sc
event (gun+0x976, FEV_NOTHOST, iparam1=type, iparam2=attachEquipped) for spectators — the same
local-predicted/event-spectator split as grenade.sc (cont83 §4).

CONFIDENCE: high.

---

## CORRECTION to cont83 (fire-sound flag)

cont83 §5.4 read the `FUN_1003de90` sound flag as "linked" (both-barrels). It is the **SILENCER
bit**: the pistol/akimbo core stores `event args->iparam2` verbatim into `DAT_1018a0b4`
(@0x1003f28d, `mov 0x3c(%edi),%ecx; mov %ecx,0x1018a0b4`) and passes `DAT_1018a0b4 & 1`
(&`%ebx`==1, set once at fn entry) at the call @0x10040975; the standard-fire path passes
`DAT_10189d10 & 1` (@0x1003ef05); the shotgun path passes literal 0 (@0x1004184f). So:
- flag==1 (silenced) → sample **wi+0x306** (the `-sil` recording), vol RandomFloat(0.84,0.92),
  **attenuation = 2 × caliber attn** (falls off twice as fast);
- flag==0 → wi+0x2c6, vol 0.92–1.0, attn = caliber attn.
Same timescale pitch both ways. Akimbo-linked UNsilenced fire therefore plays wi+0x2c6 like any
shot; wi+0x306 is only "the akimbo double-bang / pump rack" in the sense that those weapon sets
reuse the field (shotgun rack via pump.sc's direct EV_PlaySound, cont83 §3, still stands).

---

## ORCHESTRATOR SUMMARY

| # | finding | mechanism (original) | where OURS must change |
|---|---|---|---|
| 1 | Silenced p_ model | `Write_` fills wi+0x1b2 with a literal `p_X_sil.mdl` (16 weapons; mp5sd/ruger duplicate their normal p_); `CTSGun::Deploy` picks +0x1b2 iff `attach&1`; `TSPrecache` precaches both | add `szPlayerModelSil` to our `s_weapon_info` + the 18 values; choose it in our `Deploy`; precache both; rely on AddItem's redeploy for instant swap |
| 1b | Silenced fire sound | wi+0x306 = `-fire-sil.wav` per silenceable weapon; client picks it when `iparam2&1` with vol 0.84–0.92 and attn×2 | add the `-sil` sample field + the mask-bit0 branch to our EV fire sound (NOT a "linked" branch — fix per the cont83 correction) |
| 2 | Buy grammar | `rst_cfg` (→ ResetArmament) then `tkw t_a t_a …` (guns `type_attach`; knife/m61 `type_count`; akimbo double adds `type_0`), `tki i …`, `tka cal_amt …`; last-config is an in-memory memcpy, NO file | our client menu must emit exactly these four commands in this order with `%i_%i ` tokens; server must accept `rst_cfg` |
| 3 | Right-click | attach&8 → zoom cycle 0→fov1→fov2→0 (`Zoom` hides viewmodel when fov1<40; Reload/ShotgunReload/ColdCock/Holster all `Unzoom`); no scope → hold/tilt stance toggle (anim block[7] + block-pointer swap to wi+0x28c), gated on the hold block existing; fireMode 4/5 → Fire() (4 = press-gated) | mirror the full tree incl. the 0.3/1.0 cooldowns and the unzoom-on-reload/holster hooks |
| 4a | Gun-anchor cache | `HUD_DrawTransparentTriangles` → per-frame copy of every player cl_entity's engine-computed `attachment[0..3]` into a 33×4-vec3 table; player.mdl att0/1/3 = collinear points on gun bone 29, att2 = left gun bone 20 | implement the cache exactly (positions only, from the PLAYER entity), refreshed in DrawTransparentTriangles before beam/effect draws |
| 4b | Laser beam | remote: gate ActItems&2 + visible; dir = smoothed normalize(att1−att0) (snap on big deviation); start = origin+stance-z; laserbeam.spr width 4 to traceEnd, laserdot.spr at hit; color via `PlInfo(idx,0,color)` (1 grn/2 blu/else red). Local: laserbeamanim.spr from VM att0 along att1−att0 (+ second beam att2→att3 akimbo) | our laser must derive direction from the attachment PAIR (this is what makes it track reload sweeps), not from v_angle; add the PlInfo color byte + the smoothing |
| 4c | 3rd-person muzzle flare | standard fire anchors FUN_100474b0 at cache att **0**; akimbo single at (bparam1?0:2), linked both; pump port = att 3 | anchor our third-person flares at the cache attachments (not entity origin offsets) |
| 5 | Bodies & messages | ONE GetBodyByItems (wi+0x128..+0x130 per-weapon offsets, laser+flash combo group, contender +1 unscoped) used with EQUIPPED mask by server Deploy + all events; ACTIVE mask only for the local per-frame viewmodel body; WeaponInfo = type/clip/ammo/fireMode/attach (attach byte = HUD-only); ActItems = idx+activeMask (HUD/effects gate only) | port the body-offset fields + combo/contender rules into our shared GetBodyByItems; keep WeaponInfo's 5th byte HUD-only; do NOT drive models from ActItems |
| 6 | Draw anim | always hold-aware block[2]; mask affects BODY only (the "+0x178" lead was the body table at 0x100aa178) | no mask→draw-anim work needed; ensure our draw event passes the equipped mask for the body |

## UNRESOLVED
- **"#Buy_SaveConfig" menu action**: label exists (slot 5); no cfg write path found in
  client.dll and no loadout file in the live game dir — likely the same in-memory snapshot, but
  the menu-item handler was not RE'd. (Live test: click it, then scan the ts/ dir mtimes.)
- **DAT_10191d5c** (viewmodel body computed from the ACTIVE mask): written on viewmodel model
  change, no reader found by address scan — probably a field of the static viewmodel entity
  reached via base register (would make the LOCAL viewmodel body follow laser/flash toggles);
  verify live (`live.py watch client.dll 0x191d5c`).
- **gHUD+0x14 (0x100d0cb4) bit flags** gating the flashlight/local-laser fn (`&2` at entry,
  `&0x40` inside): writer not located (register-relative); semantics assumed "has item" bits.
- The 6+8 lazily-registered cvars in the two beam renderers (thunks 0x1003dda0–0x1003ddf0,
  0x1003d180–0x1003d1f0): names not individually dumped (cl_laser/cl_lasercolor family).
- Smoothing constants of the remote laser (frametime×cvar cap, snap threshold `_DAT_1007d6f0`)
  read as .rodata floats — values not extracted.
- Buy-menu slot-field provenance (slot+4 attach vs slot+0x30 count) inferred from the Apply
  disasm only; the menu code that FILLS the slots was not decompiled.
