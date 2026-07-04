# Dossier — Bullet penetration / wall pierce (cont.86)

RE of the ORIGINAL TS pierce model in `reference/ts_i386.so`, compared against our
`dlls/TS_combat.cpp::CBasePlayer::ShootBulletsPlayer`. Goal: explain the reported over‑penetration
(instant‑hit fire piercing non‑breakable walls for *every* firearm, incl. the ruger) and give a
faithfulness‑grounded fix.

- **Original function:** `ShootBulletsPlayer__11CBasePlayerUcG6VectorN22fiii` @ nm `0x5f69c`
  (Ghidra `0x6f69c`). Raw typed decompile: `decomp/ShootBulletsPlayer_0x5f69c.c`.
- **Ammo table builder:** `compile_ammo_info__Fv` @ nm `0x6f51c` (Ghidra `0x7f51c`). Raw:
  `decomp/compile_ammo_info_0x6f51c.c`. `s_ammo_info` = 0x38 bytes/entry, `global_ammo_info[16]`
  @ nm `0x176e00` (Ghidra `0x186e00`), entry `cal` at `0x186e00 + cal*0x38`.

---

## TL;DR

**Our pierce reconstruction is mathematically FAITHFUL.** The original has **no separate
"can‑penetrate" caliber flag**. The pierce GATE is literally `damage > 5.0`, and the wall‑thickness
step is `min(2*damage, 48)` — exactly what we have. The **caliber gate the user observes is REAL but
INDIRECT**: it enters through the per‑caliber factor **`flFactor2` (`s_ammo_info+0x18`)**, which
multiplies the post‑falloff damage *before* the gate and the step are computed. Barrett (.50BMG,
`flFactor2=1.0`, dmg 185) → step capped at 48u; ruger (.22LR, `flFactor2=0.9`, dmg 15) → step 27u;
shells (`flFactor2=0.0`) → damage decays to 0 → **shotguns NEVER pierce** (gate `0 > 5` false).

The **one structural divergence** in our code: we wrap the `*= flFactor2` decay (and the through‑body
`*0.5`) inside `if (pEntity) { ... }`. The original applies **`*= flFactor2` unconditionally on any
hit** (`flFraction != 1.0`), outside the pEntity path. Whenever `pEntity` is null on a hit in our
build, we **skip the caliber decay**, so the step is computed from **undecayed** damage → the pierce
distance inflates (and for `flFactor2==0` shotguns, they go from "never pierce" to "pierce up to 48u").
**Fix = hoist the `flFactor2` decay + IsPlayer halving out of the `if (pEntity)` guard** to match the
original.

---

## 1. The PIERCE GATE — confidence: HIGH (decompile‑exact)

From `ShootBulletsPlayer_0x5f69c.c`, the block after the pellet loop (the mode‑0 volley path):

```c
if (((rbullet_1156 == 0) &&
    (ApplyMultiDamage__FP9entvars_sT0(...), 1 < (int)a2.z - 1U))   // a2.z == iMode
   && ((float)dc_5 < damage_ammount_1152)) {                       // dc_5 = 5.0
```

- **iMode gate** `1 < (int)a2.z - 1U`: unsigned compare of `(iMode - 1)`. TRUE for iMode==0 and
  iMode>=3; FALSE for iMode==1 and iMode==2. → `(iMode == 0 || iMode > 2)`. **Matches ours** (line 150).
- **Damage gate** `dc_5 < damage_ammount_1152` → `damage > 5.0`. `dc_5` is the double `5.0`.
  **NO ammo‑info field is read in the gate.** There is no `get_ammo_info(...)->canPierce` / caliber
  threshold anywhere in the condition. **Matches ours** (`flDamageAmount > 5.0f`).

**But `damage_ammount_1152` at this point is already caliber‑decayed.** Inside the pellet loop
(mode‑0 hit path), after `TraceAttack`:

```c
damage_ammount_1152 = damage_ammount_1152 * *(float *)(a_info_1134 + 0x18);   // *= flFactor2
iVar6 = (**(code **)(*(int *)(pEntity_1155 + 0x60) + 200))(pEntity_1155);      // IsPlayer (vtbl+0xC8)
if (iVar6 != 0) damage_ammount_1152 = (float)dc_0_5 * damage_ammount_1152;     // through a body: *0.5
```

So the value that reaches BOTH the `>5` gate and the step is
`rangemult * flDamage * flFactor2 (* 0.5 if last hit was a player)`. **This is the caliber gate.**
`flFactor2` = `s_ammo_info+0x18` (same offset our code labels `flFactor2`).

> Provenance note on the original's `pEntity_1155`: it is `edict->pvPrivateData` (`+0x7c`) and is
> **assumed non‑null** — `TraceAttack`, the `flFactor2` multiply, and `IsPlayer` are all called
> WITHOUT a null check. On a world/wall hit the original's `pEntity_1155` = the `CWorld` private data
> (non‑null), so `flFactor2` is always applied.

---

## 2. The WALL‑THICKNESS step — confidence: HIGH (decompile‑exact)

```c
fVar2 = damage_ammount_1152 + damage_ammount_1152;        // 2 * damage
if (fc_48 <= damage_ammount_1152 + damage_ammount_1152)   // fc_48 = 48.0
    fVar2 = fc_48;                                         // cap at 48
// vecSrcPierce = tr.vecEndPos + fVar2 * vecDir
...
iVar6 = UTIL_PointContents__FRC6Vector(&vecSrcPierce);
if (iVar6 != -2)                                           // -2 == CONTENTS_SOLID
    ShootBulletsPlayer(1, vecSrcPierce, vecDir, {x*spread.x, y*spread.y, 0},
                       damage_ammount_1152 * damage_adjust_1154, /*iMode=*/2, iCaliber, shared_rand);
```

- `step = min(2*damage, 48.0)`. Constants: `fc_48 = 48.0f`, cap is `<=`. **Matches ours exactly**
  (`step = flDamageAmount*2; if (step>48) step=48;`).
- `damage` here is the **post‑`flFactor2`** value (§1). So the step is caliber‑scaled.
- **Thickness test is the step‑past‑then‑PointContents trick, NOT an explicit exit‑point trace.**
  It steps `step` units along `vecDir` from the entry hit `tr.vecEndPos`, then asks
  `UTIL_PointContents(thatPoint) != CONTENTS_SOLID`. If the point past the wall is no longer inside
  world solid, the wall is thinner than `step` → continue. `-2` is `CONTENTS_SOLID`. **Matches ours.**
- Note: `UTIL_PointContents` tests the **world BSP only** — a func_breakable in the way does not
  read as SOLID, which is why bullets pass through breakables regardless of thickness (this is
  original behavior, both mode‑0 and the continuation just TraceAttack whatever the next trace hits).

---

## 3. The continuation leg (iMode==2) — confidence: HIGH

Entered as the recursion with `cShots=1, iMode=2`. In the decompile this is the
`if (a2.z == 2.8026e-45)` branch (`2.8026e-45` = float with bit pattern 2 = iMode==2):

- **Single trace of `flDamage_ptr... ` length 4096u** from `vecSrcPierce`
  (`local_38 = fc_4096 * dir` → `vecEnd = src + 4096*dir`).
- **Falloff = the FIXED `/300` form, NOT the ammo's range unit.**
  `distance_1153 = length * fc_0_0033333334140479565` (= `dist/300`), then
  `idx = ROUND(distance_1153 * fc_10)` = `round(dist/30)`; `damage = get_range_multipler(idx)*flDamage`.
  (Contrast the mode‑0 volley leg, which uses `dist / (10 * iRangeUnit)` then `*10` → `round(dist/iRangeUnit)`.)
  **Matches ours** (line 79: `round(dist * (1/300) * 10)`).
- **No further recursion, no flFactor2 decay, no gate.** It just `TraceAttack`s whatever it hits and
  returns. → **maximum 2 surfaces total** (entry wall + one continuation hit). **Matches ours.**
- The continuation `flDamage` passed in is `damage_ammount * damage_adjust` (post‑decay). So the
  leg beyond the wall keeps decaying by caliber implicitly (its base is already decayed).

---

## 4. Which calibers penetrate — confidence: HIGH (all values read from the binary)

`flFactor2` per caliber, **verified from `compile_ammo_info_0x6f51c.c`** (entry base = `0x186e00 + cal*0x38`,
`+0x18` field), cross‑checked against our `dlls/ts_weapon_info.cpp::compile_ammo_info` — **all match**:

| cal | name | flFactor2 (+0x18) | raw hex |
|----:|------|------------------:|---------|
| 1 | 9x19 | 0.2 | 3e4ccccd |
| 2 | .45Acp | 0.2 | 3e4ccccd |
| 3 | **SHELLS** | **0.0** | 00000000 |
| 4 | 5.56 | 0.9 | 3f666666 |
| 5 | .50AE | 0.5 | 3f000000 |
| 6 | 7.62x39 | 0.7 | 3f333333 |
| 7 | 5.7x28 | 1.0 | 3f800000 |
| 8 | **50BMG (barrett)** | **1.0** | 3f800000 |
| 9 | 10mm | 0.5 | 3f000000 |
| 10 | **.22LR (ruger)** | **0.9** | 3f666666 |
| 11 | .454 | 0.75 | 3f400000 |
| 12 | .32Acp | 0.0 | 00000000 |
| 13 | 7.62x51 (m60) | 0.7 | 3f333333 |

Weapon→caliber→damage (from `Write_ruger` @0x88a00 / `Write_barrett` @0x5e018 for caliber; `Sparta()`
overrides for damage — memory *sparta-overrides-write-weapon*):

- **Ruger (weapon 28):** caliber **10** (.22LR), Sparta damage **15.0** (Write was 16.0).
  Point‑blank (rangemult=1): post‑decay `15 * 0.9 = 13.5`. Gate `13.5 > 5` ✓.
  **step = min(2*13.5, 48) = 27u.** → pierces world walls **< 27u**.
- **Barrett (weapon 18):** caliber **8** (50BMG), Sparta damage **185.0** (Write was 210.0).
  Post‑decay `185 * 1.0 = 185`. Gate ✓. **step = min(370, 48) = 48u.** → pierces world walls **< 48u**.
- **Shotguns (SHELLS, caliber 3):** `flFactor2 = 0.0` → post‑decay damage `= 0` → gate `0 > 5`
  **FALSE** → **never pierce.** (`.32Acp` skorpion caliber 12 is also 0.0.)

**So the barrett‑yes / ruger‑no difference the user reports IS the model working:** for any wall in
the **27–48u** band, the barrett (step 48) continues and the ruger (step 27) is stopped. This is
faithfully reproduced by our code **as long as `flFactor2` actually reaches the step at runtime** —
see §5.

---

## 5. Where our build can diverge — confidence: MEDIUM (the actionable fix)

Structurally our `TS_combat.cpp` matches the original gate/step/continuation. The **only** divergence
is the null‑guard placement:

**Original (`flFraction != 1.0` block):** computes `damage`, calls `TraceAttack`, then
**unconditionally** `damage *= flFactor2` and `if (IsPlayer) damage *= 0.5` — `pEntity` assumed
non‑null.

**Ours (`dlls/TS_combat.cpp` ~lines 109–134):**
```c
flDamageAmount = get_range_multipler(idx) * flDamage;   // set OUTSIDE the guard (line ~115)
...
if (pEntity)                                            // line ~123 — GUARD
{
    pEntity->TraceAttack(...);
    flDamageAmount *= ai->flFactor2;                    // line ~130 — INSIDE the guard
    if (pEntity->IsPlayer()) flDamageAmount *= 0.5f;
}
```

If `CBaseEntity::Instance(tr.pHit)` ever returns **null on a hit** (e.g. a hit entity without private
data, or any engine/SDK case where the world edict yields no `CBaseEntity`), our build **keeps
`flDamageAmount` at the undecayed `rangemult*flDamage`**, so:
- the `>5` gate uses undecayed damage, and
- the step becomes `min(2 * rangemult*flDamage, 48)` instead of `min(2 * decayed, 48)`.

Consequences of skipping the decay:
- **Shotguns (`flFactor2=0`) flip from NEVER‑pierce to pierce up to 48u** — the single biggest
  over‑penetration hole.
- **Ruger** step goes 27 → up to `min(2*15, 48) = 30`; other low‑`flFactor2` calibers inflate more
  (9mm/.45 `0.2`: `min(2*dmg*0.2,48)` vs `min(2*dmg,48)` — e.g. 9mm 24dmg: 9.6u → 48u).

Even in the normal case (world hit → `CWorld` non‑null → decay applies), the faithful ordering is to
apply `flFactor2`/`IsPlayer` on **any** hit, not gated on our `pEntity` pointer. Making that change
removes the divergence class entirely and is the exact original behavior.

---

## 6. Fix recipe for `dlls/TS_combat.cpp`

Hoist the caliber decay and the through‑body halving OUT of the `if (pEntity)` guard so they run on
every hit (`flFraction != 1.0`), exactly like the original. `TraceAttack` stays guarded (only a real
entity can take damage); the world case simply decays the carry‑over without a TraceAttack.

Replace the mode‑0 hit body (roughly lines 115–133) with this shape:

```c
flDamageAmount = get_range_multipler(idx) * flDamage;
const float dmgMult = CVAR_GET_FLOAT("damagemult");
if (dmgMult > 0.0f && CVAR_GET_FLOAT("sv_cheats") != 0)
    flDamageAdjust = dmgMult;
const int dmgFlags = (iMode == 1) ? (DMG_BULLET | DMG_NEVERGIB | (1 << 25))
                                  : (DMG_BULLET | DMG_NEVERGIB);
if (pEntity)
{
    pEntity->TraceAttack(pev, flDamageAmount * flDamageAdjust, vecDir, &tr, dmgFlags);
    if (CVAR_GET_FLOAT("sv_cheats") != 0 && CVAR_GET_FLOAT("dmgreport") > 1.0f)
        UTIL_ClientPrintAll(HUD_PRINTNOTIFY,
            UTIL_VarArgs("damage:%f\n", flDamageAmount * flDamageAdjust));
}
// ORIGINAL applies these unconditionally on a hit (outside the pEntity guard):
flDamageAmount *= ai->flFactor2;                    // caliber decay — gates the pierce step
if (pEntity && pEntity->IsPlayer())                 // through a body: halve the carry-over
    flDamageAmount *= 0.5f;
```

Do NOT add any new caliber flag or thickness clamp — the original has none; the caliber gate is the
`flFactor2` pre‑multiply, and the thickness gate is `min(2*damage, 48)` + the PointContents test,
all already present. The only change is *when* `flFactor2` is applied.

### Verify after the change (runtime)
1. Rebuild server (`scripts/build.sh amd64`) and load `ts` on the ReHLDS rig / xash.
2. `sv_cheats 1; dmgreport 2` prints `damage:` and `damage(piercing):` lines — confirm the ruger's
   printed damage at the wall is ~13.5 (decayed), not ~15.
3. Fire a **shotgun** (benelli/spas) point‑blank into a thin non‑breakable wall with a breakable
   behind: with the fix it must NOT break the thing behind (shells `flFactor2=0` → step 0).
4. Barrett must still pierce a ~30–45u wall that the ruger does not (unchanged; that band is correct).

---

## Confidence summary

| Finding | Confidence | Basis |
|---|---|---|
| Gate = `damage > 5.0`, no caliber flag | HIGH | decompile‑exact, `dc_5` |
| Caliber gate = `flFactor2` (+0x18) pre‑multiply | HIGH | decompile + verified table |
| step = `min(2*damage, 48)`, PointContents(step‑past) trick | HIGH | decompile‑exact, `fc_48`, `-2`=SOLID |
| Continuation: 1 leg, 4096u, `/300` falloff, no recursion (max 2 surfaces) | HIGH | decompile‑exact |
| barrett cal 8 flFactor2 1.0 → step 48; ruger cal 10 flFactor2 0.9 → step 27; shells 0.0 → step 0 | HIGH | binary table + Write/Sparta |
| Our `if(pEntity)` guard skipping `flFactor2` is the over‑penetration source | MEDIUM | structural diff vs original; depends on when `Instance()` returns null (world/edge cases). Fix is faithful regardless. |
