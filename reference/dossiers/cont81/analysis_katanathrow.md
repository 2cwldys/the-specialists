# CTSGun::KatanaThrow — analysis (original @ nm 0x73b68 / Ghidra 0x83b68, size 787)

## 1. SIGNATURE

From `reference/signatures-gcc2.txt:1131` + the typed decompile:

```
void CTSGun::KatanaThrow()            // KatanaThrow__6CTSGun, thiscall (gcc2 cdecl-this)
```

No parameters, no return. Source file (plate comment / filemap): `TS_gun.cpp`.
Key callee signature (signatures-gcc2.txt:1164):

```
CTSKnife::CreateKnife(CBasePlayer *, Vector, T2?, int)   // static, @ nm 0x6d9e4
//  = CreateKnife(CBasePlayer *pOwner, Vector vecPos, Vector vecVelocity, int iKnifeType)
```

## 2. BEHAVIOR

Step-by-step (all offsets confirmed against the raw objdump of 0x73b68–0x74040):

1. **Gate:** `if (m_pActiveItem == NULL) return;` (`this+0xcc`, `s_weapon_status*`). That is the
   ONLY gate — no button check, no clip check (SecondaryAttack's dispatch is the button gate,
   see §3). Note: unlike the grenade/knife branches of SecondaryAttack, the katana branch does
   NOT set `m_pActiveItem->+0x24 = 1` first.
2. **Re-arm timers:**
   - `m_flNextPrimaryAttack` (`+0x9c`) = `m_flNextSecondaryAttack` (`+0xa0`) =
     `UTIL_WeaponTimeBase() + m_pActiveInfo->+0x28`. `s_weapon_info+0x28` is the field our
     header calls `flSpread`; for the katana `Write_katana` stores **0.7f** there (0x3f333333 @
     0x772d2) — so both attacks re-arm at **+0.7 s** (moot in practice: the weapon is dropped at
     the end of this function).
   - `m_flTimeWeaponIdle` (`+0xa8`) = `UTIL_WeaponTimeBase() + 3.0` (double `dc_3`).
3. **Aim trace:** `UTIL_MakeVectors(m_pPlayer->pev->v_angle)`; let `eyes = origin + view_ofs`
   (`pev+0x08` + `pev+0x174`). Traces:
   - start = `eyes + 1.0*v_right`
   - end   = `eyes + 32.0*v_forward + 1.0*v_right`
   - `UTIL_TraceLine(start, end, dont_ignore_monsters /*0*/, player edict, &tr)`
   The spawn position is **`tr.vecEndPos`** (TraceResult+0x14) — i.e. clamped to the first hit
   within 32 units so the blade cannot spawn inside/behind a wall.
4. **Spawn the projectile:**
   `CTSKnife::CreateKnife(m_pPlayer, tr.vecEndPos, 1000.0f * v_forward, m_pActiveInfo->+0x20)`.
   - Throw velocity = **1000 u/s** along `v_forward` (float @ GOT-0x6deac = 1000.0). NO player
     velocity inheritance. (Contrast: `KnifeFire` throws at 1900.)
   - 4th arg `s_weapon_info+0x20` is a **knife-projectile-type code**, verified in the binary:
     `Write_katana` = **8** (@0x772f6), `Write_knife` = **6** (@0x774d1), `Write_sealknife` =
     **7** (@0x88f41). It is stored into `CTSKnife+0xa0` and drives model/gravity in
     `CTSKnife::Spawn` (8→w_katana + gravity 0.85, 6→w_knife + 0.75, else→w_sealknife + 0.3)
     and the pickup mapping in `CTSKnife::Use` (8→weapon 34 katana, 6→25 knife, else→35
     sealknife). **No damage value is passed** — CreateKnife derives damage internally (it reads
     `player+0x180` and makes vtable calls `knife->vtbl[+0x14]` / `player->vtbl[+0x10]`; VERIFY
     the exact damage mechanism — out of scope here).
5. **Orient the blade (if CreateKnife returned non-NULL):**
   - `dir = normalize(knife->pev->velocity)` (guard: zero-length → zeros)
   - `knife->pev->angles = UTIL_VecToAngles(dir)`
   - `knife->pev->angles.z = 90.0f` (0x42b40000 — blade rolled edge-on)
   - `knife->pev->avelocity.x = -700.0f` (0xc42f0000 — end-over-end spin).
     (KnifeFire does the same but with **-1300** = 0xc4a28000, @ 0x7483b/0x7485e.)
6. **Third-person throw animation**, gated:
   `if (!(player->pev->iuser4 & 0x30c30) && (player->pev->flags & FL_ONGROUND /*0x200*/))`
   → `CTSAnimation::Animate(&player->+0x28c /*m_TSAnim*/, player, 0x20)`.
   - `0x30c30` in our bit map (dlls/ts_stunt.h) = `TSSTUNT_DIVE|TSSTUNT_SLIDE|TSSTUNT_WHEEL_HI|
     TSSTUNT_WHEEL_LO|TSSTUNT_WALLJUMP_A|TSSTUNT_WALLCLIMB_A` — suppress the throw anim
     mid-stunt (equals our `TSSTUNT_ACTIVE_MASK 0x130c30` minus the 0x100000 WHEEL_DONE bit).
   - anim id `0x20` = our `TSA_CC_KNIFELAUNCH` (dlls/ts_animation.h:63) — the knife-launch gesture.
   - **No `SendWeaponAnim` / viewmodel animation at all** (the weapon is dropped immediately).
7. **Sound:** `EMIT_SOUND_DYN(player edict, 4 /*CHAN_BODY*/, "weapons/katana/miss.wav",
   1.0 /*vol*/, 0.8 /*ATTN_NORM*/, 0 /*flags*/, 95 + RANDOM_LONG(0,31) /*pitch 95..126*/, 0.5f)`.
   TS's EMIT_SOUND_DYN is a CUSTOM 8-arg version (`...iif`, nm 0xd0a8c): the trailing float is a
   slow-motion pitch sensitivity — the impl computes `pitch *= (1-f) + f*pev->fuser1` when
   `pev->fuser1 > 0` (edict+0x2d4 = entvars+0x254 = fuser1). VERIFY the fuser1 semantics
   (per-entity slowmo factor); here f = **0.5**.
8. **Consume the weapon** — `if (!InfiniteClips(player))`:
   - `player->+0x548 += (byte)m_pActiveInfo->+0xd0` — refund the katana's **slotCost = 10**
     (`Write_katana` @ 0x77330) into the player's free-slots counter (CBasePlayer+0x548).
   - `CBasePlayer::SetPlayerSpeed(player, GetSpeedBySlots(player))` — speed reflects freed slots.
   - `DropWeapon(this, m_weaponType /*+0xc8*/, true)` — arsenal bookkeeping removal +
     auto-select next-best weapon (this deploys the next gun; hence no viewmodel anim).
   With InfiniteClips, the katana is kept and can be re-thrown.

No user messages, no PLAYBACK_EVENT, no client events — server-side entity spawn + server sound.
Constants recap: refire 0.7 (info+0x28), idle +3.0, trace reach 32, right offset 1.0, speed
**1000**, roll 90°, avelocity.x **-700**, pitch 95+rand(0..31), slowmo arg 0.5, slotCost 10,
knife-type code 8.

## 3. CALLERS

`melee_refs.txt` contains no caller of KatanaThrow (it holds broader melee context:
VictimKatanaHold/VictimKnifeHold/CCombatBlock/ColdCock...). The actual caller was found by objdump:

- **Sole caller: `CTSGun::SecondaryAttack` (nm 0x717a8 / Ghidra 0x817a8), @ 0x71843**, via PLT.
  SecondaryAttack's dispatch (disasm 0x717bd–0x71860):
  - `m_pActiveItem == NULL` → `CCombatAttack(this)` (bare-hands kung-fu secondary)
  - `m_pActiveInfo->iFireType (+0x348) == 2` → set `item+0x24=1`, `GrenadeFire(this)`
  - `iFireType == 3` → set `item+0x24=1`, `KnifeFire(this)` (knife/sealknife — throw lives there)
  - **`iFireType == 10` (= our `TSFT_KATANA`) → `KatanaThrow(this)`** — no `item+0x24` write
  - `iFireType == 11` (flag) → nothing; then akimbo(4/5)/scope/hold-position branches follow.
- `SecondaryAttack` is the virtual `CBasePlayerItem::SecondaryAttack` slot, dispatched from stock
  `CBasePlayerWeapon::ItemPostFrame` when `(pev->button & IN_ATTACK2) &&
  m_flNextSecondaryAttack <= time` — that is where the button/timer gating lives; KatanaThrow
  itself has none. Right-click with the katana out ⇒ throw (at most every 0.7 s, moot since the
  weapon leaves the inventory unless InfiniteClips).

## 4. OURS

Equivalent logic exists but is an **invented merge under the wrong name**:

- **`CTSGun::ThrowKnife()` — dlls/ts_gun.cpp:1518** (decl ts_gun.h:97). Annotated as "KnifeFire
  throw branch" but handles BOTH `TSFT_KNIFE` and `TSFT_KATANA` (routed from our ItemPostFrame at
  ts_gun.cpp:1877-1878). Differences vs the original KatanaThrow:
  - **Gates we invented:** `m_afButtonPressed & IN_ATTACK2` one-per-press gate (1521), clip
    check + `PlayEmptySound` + `m_iClip--` (1523-1529). The original katana has NO clip logic.
  - **Spawn position:** ours = `GetGunPosition() + 16*v_forward` (1532); original = TraceLine
    endpoint of `eyes+1*v_right → eyes+32*v_forward+1*v_right` (wall-safe).
  - **Velocity:** ours = `1900*v_forward` for all knives (1533); original katana = **1000**
    (1900 is KnifeFire's number).
  - **CreateKnife contract:** ours passes `(player, TSW_* id, damage, pos, vel)`
    (ts_knife.cpp:291,331); original passes `(player, pos, vel, knifeTypeCode 6/7/8)` — the
    type-code field `s_weapon_info+0x20` is MISSING from our `s_weapon_info`, and damage is NOT
    a CreateKnife parameter in the original (computed inside CreateKnife).
  - **Orientation:** ours sets `angles = VecToAngles(velocity)`, `avelocity = (-1300,0,0)`, no
    roll, inside `CTSKnife::Spawn` (ts_knife.cpp:74-75) for all types; original sets it in the
    THROWER after CreateKnife: `angles = VecToAngles(normalize(vel))`, `angles.z = 90`,
    `avelocity.x = -700` (katana) / `-1300` (KnifeFire). Our worldspawn-stick code already knows
    the roll-90 (ts_knife.cpp:171).
  - **Viewmodel anim:** ours `SendWeaponAnim(7/5)` (1538) — the original KatanaThrow sends NONE
    (weapon dropped immediately; the VERIFY at 1538 about the katana seq resolves to "no anim").
  - **Third-person anim:** ours has none; original = `Animate(TSA_CC_KNIFELAUNCH)` gated on
    `!(iuser4 & 0x30c30) && FL_ONGROUND`.
  - **Sound:** ours CHAN_WEAPON + `TSSlowPitch(player)` (1539-1540, right wav name); original
    CHAN_BODY(4), vol 1.0, attn 0.8, pitch `95+RANDOM_LONG(0,31)`, slowmo-arg 0.5 via the TS
    8-arg EMIT_SOUND_DYN.
  - **Timers:** ours +0.5/+0.5/idle+5 (1541-1543); original `+wi->flSpread` (0.7 katana) for both
    attacks, idle +3.0.
  - **Consume path:** ours `m_items[type].owned=false; GetBetterWeapon(type)` only when
    `m_iClip<=0` (1546-1550) — no InfiniteClips gate, no slot refund, no speed update; original
    always (unless `InfiniteClips`, dlls/ts_player.cpp:124) refunds slotCost into the slots
    counter, calls `SetPlayerSpeed(GetSpeedBySlots(player))` (both exist: ts_gun.cpp:121,
    ts_player.cpp:465) and `DropWeapon(m_weaponType, true)` (exists: ts_gun.cpp:2273 — same
    semantics as the original `DropWeapon__6CTSGunib`).
- There is **no `CTSGun::SecondaryAttack` method in our code** — its dispatch is inlined into our
  ItemPostFrame (ts_gun.cpp:1870-1930), and no `CTSGun::KatanaThrow`/`CTSGun::KnifeFire` exist
  (audit.py will show these as MISSING; `ThrowKnife` is INVENTED).

## 5. RECONCILE

Concrete plan (source-parity + drop-in behaviour; no protocol impact — see below):

1. **Create `CTSGun::KatanaThrow()`** in dlls/ts_gun.cpp as a faithful port of §2: activeItem
   gate only; timers `+wi->flSpread`/idle+3; the 32-unit eye trace for spawn pos; CreateKnife at
   `1000*v_forward`; orient (`VecToAngles(normalize(vel))`, roll 90, `avelocity.x=-700`);
   `Animate(TSA_CC_KNIFELAUNCH)` under the `!(iuser4 & 0x30c30) && FL_ONGROUND` gate; CHAN_BODY
   miss.wav at pitch 95+rand(0,31); `if (!InfiniteClips(...))` → slot refund +
   `SetPlayerSpeed(GetSpeedBySlots(...))` + `DropWeapon(m_weaponType, true)`. No SendWeaponAnim,
   no clip touch.
2. **Rename/split `ThrowKnife`:** the knife/sealknife path belongs to the original
   **`KnifeFire`** (fire-type 3, @ nm 0x74044, ~2.5 KB — it also contains the melee slash and
   sets `item+0x24=1`; needs its own RE pass before renaming ours to it — VERIFY; until then
   keep ThrowKnife for TSFT_KNIFE only and stop routing TSFT_KATANA through it).
3. **Restore the original dispatch shape:** add `CTSGun::SecondaryAttack()` with the fire-type
   switch (activeItem NULL→CCombatAttack, 2→GrenadeFire, 3→KnifeFire, 10→KatanaThrow, 11→none,
   then akimbo/scope/hold branches), and let ItemPostFrame call it — absorbs the inline block at
   ts_gun.cpp:1870-1930. Match the original's `item+0x24 = 1` writes for the grenade/knife
   branches (VERIFY what +0x24 gates elsewhere before wiring behaviour to it).
4. **CreateKnife contract:** add the `s_weapon_info+0x20` knife-projectile-type field
   (knife=6, sealknife=7, katana=8) to `ts_weapon_info.h/.cpp`, change
   `CTSKnife::CreateKnife(pOwner, pos, vel, int knifeType)` to store it in `m_knifeType`
   (translating 6/7/8 ↔ TSW_* where our pickup code needs it, as the original does in Use()),
   and move damage determination INSIDE CreateKnife per the original (VERIFY the `player+0x180`
   / vtable-slot damage source with re.sh/live trace before implementing — do not guess).
   Move `angles/avelocity` assignment out of `CTSKnife::Spawn` into the throwers (KatanaThrow
   -700, KnifeFire -1300, both roll 90).
5. **Absorbs/replaces:** our invented `ThrowKnife` katana branch (replaced by KatanaThrow), the
   `m_items[..].owned=false + GetBetterWeapon` inline (replaced by `DropWeapon(type,true)` +
   slot/speed refund), the `TSThrowKnife` helper's katana use.
6. **Protocol implications: none.** KatanaThrow emits no user messages, no events, no
   client-predicted weapon anim — everything is server-side (entity + EMIT_SOUND_DYN). Removing
   our katana `SendWeaponAnim` IMPROVES fidelity against the original client. The only
   client-visible surfaces are the knife entity's model/angles/avelocity and the sound, both
   engine-networked.

**VERIFY list:** (a) CreateKnife's internal damage source (`player+0x180`, `knife->vtbl[+0x14]`,
`player->vtbl[+0x10]`); (b) TS EMIT_SOUND_DYN's 8th float = slowmo pitch sensitivity vs
`pev->fuser1`; (c) `s_weapon_status+0x24` meaning (set for grenade/knife branches, not katana);
(d) KnifeFire's full structure before renaming ThrowKnife; (e) whether the original CLIENT dll's
SecondaryAttack for fire-type 10 also does nothing viewmodel-wise (CTSGun is client-predicted).
