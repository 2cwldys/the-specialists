# CBasePlayer::ColdCockAttack — analysis (ts_i386.so @ file 0x86610 / Ghidra 0x96610, size 1158)

## 1. SIGNATURE

Mangled: `ColdCockAttack__11CBasePlayerG6VectorN21ifibN37` (exported `T` @ 0x86610; PLT stub @ 0x5abf0).
gnu-v2 decode: `G6Vector` = Vector **by value** (gcc2 passes a pointer to a caller-made copy — the
disasm confirms pointers-to-stack-copies are pushed), `N21` = two more Vectors, `i`, `f`, `i`, `b`,
`N37` = three more bools.

```cpp
// return typed CBasePlayer* by Ghidra; semantically the entity hit (or NULL) — CBaseEntity* in source
CBaseEntity *CBasePlayer::ColdCockAttack(
    Vector vecSrc,        // param_2 — trace start (gun position)
    Vector vecEnd,        // param_3 — trace end (src + v_forward * 35)
    Vector vecDir,        // param_4 — damage direction fed to TraceAttack (v_forward)
    int    iHull,         // param_5 — hull number for CheckHull's TraceHull pass (caller passes 3 = head_hull)
    float  flDamage,      // param_6
    int    iAnim,         // param_7 — body-animation id fed to CTSAnimation::Animate (caller: 39, or -1 mid-stunt)
    bool   bDropWeapon,   // param_8 — on player hit, make the victim drop their weapon
    bool   bPlayersOnly,  // param_9 — if the hit entity is not a player, return NULL (no damage)
    bool   bKnockback,    // param_10 — on player hit with flDamage > 30, shove the victim
    bool   bBlockTeammates// param_11 — block teammates even when mp_friendlyfire is on
);
```

Same family as `CloseCombatAttack__11CBasePlayerG6VectorN21ifibN37` @ 0x85f88 (identical arg list —
the kungfu/melee twin, which sets `glast_closecombat` instead). `signatures-gcc2.txt` has no demangled
entry for either (the `G…N21…N37` form is one of the ~26% the demangler skips); parameter roles above
are read directly from the typed decompile + the call-site disasm. Parameter NAMES are invented
(roles are certain from the code).

## 2. BEHAVIOR

Step by step (member offsets are the original gcc2 layout — provenance only):

1. **Attacker body anim**: `CTSAnimation::Animate(&this->m_TSAnim /*player+0x28c*/, this, iAnim)`.
   Runs BEFORE the trace, so the swing anim plays on miss too. iAnim from the only caller: 39 (0x27)
   normally, -1 while stunting.
2. **Trace**: `CBasePlayer::CheckHull(vecSrc, vecEnd, tr, iHull)` (@0x85e28,
   `CheckHull__11CBasePlayerG6VectorT1R11TraceResulti`). CheckHull = `UTIL_TraceLine(src, end,
   dont_ignore_monsters, attacker_edict, &tr)`; if that hit nothing (`flFraction >= 1`),
   `UTIL_TraceHull(src, end, dont_ignore_monsters, iHull /*3 = head_hull*/, attacker_edict, &tr)`;
   on a hull hit it further inspects `tr.pHit` (world fallback via `pfnPEntityOfEntOffset(0)`), a
   vtable check @slot 0xb8/4=46 on the hit entity, and re-derives a box by hull (VERIFY CheckHull's
   tail — it is its own RE target; ColdCockAttack only consumes `tr.flFraction`/`tr.pHit`).
3. `if (tr.flFraction >= 1.0) return NULL;` — whiff.
4. `pHit = tr.pHit ? tr.pHit : pfnPEntityOfEntOffset(0)` (world); `pTarget = GET_PRIVATE(pHit)`
   (edict+0x7c); `if (!pTarget) return NULL;`
5. **Damageability gates** → return NULL if any:
   - `pTarget->pev->takedamage == 0` (DAMAGE_NO)
   - `pTarget->pev->flags & 0x2000000` (FL_WORLDBRUSH, 1<<25)
   - `(char)pTarget->pev->effects < 0` (EF_NODRAW, 0x80)
6. **Player/team gates**:
   - `!pTarget->IsPlayer() && bPlayersOnly` → return NULL.
   - `pTarget->IsPlayer() && (bBlockTeammates || friendlyfire->value == 0 /*cvar_t+0xc*/)
      && g_pGameRules->PlayerRelationship(this, pTarget) == GR_TEAMMATE /*vtable slot 0xcc/4=51,
      result compared == 1*/` → return NULL (no sound, no damage at all).
     I.e. with friendly fire ON and bBlockTeammates false, teammates CAN be coldcocked.
7. **Player-victim branch** (`pTarget->IsPlayer()`), with `bApplyDamage = true`:
   - `held = VictimKungfuHold(this, pTarget, flDamage, &tr)` (@0x66f50, TS_gamerules.cpp). The
     close-combat BLOCK check: victim is a player whose block window (`player+0x7a8`, our
     `m_flKungFuBlock`) is open (or the symmetric attacker-block case via +0x79c/+0x7a4), and whose
     melee bar `player+0x774 >= 0.1`. On a successful hold it plays "player/kungfuhit.wav"
     (CHAN_BODY, vol 1.0, attn 0.8, pitch 100, trailing float 0.5), zeroes `+0x7a0` on both, sets
     attacker `+0x12c = 0.5f` and blocker `+0x12c = 0.25f` (the player next-attack lock — VERIFY
     name, likely m_flNextAttack), calls `UpdateMeleeBar(attacker, 33.0)`, returns 1.
   - If **not held**:
     - `if (bDropWeapon && pTarget->field_0x71c < gpGlobals->time)` →
       `pTarget->DropWeapon((tsweapon_t)-2, flDamage)` (@0x84900; -2 = active-weapon sentinel).
       `CBasePlayer+0x71c` = drop-immunity/re-drop timer (transient, unnamed — VERIFY name).
       Not exercised by coldcock (bDropWeapon=false from the only caller).
     - `if (bKnockback && flDamage > 30.0)` → shove: `speed = |attacker.velocity|`;
       `dir = (target.origin − attacker.origin).Normalize()` (degenerate |Δ|==0 → dir z-comp with
       length 1); `target.velocity += dir * speed`; then **`target.velocity.z = 235.0`**
       (hard-set). Not exercised by coldcock (bKnockback=false).
     - `EMIT_SOUND_DYN(target_edict, 4 /*CHAN_BODY*/, "player/kungfuhit.wav", 1.0, 0.8 /*attn*/,
       0, 95 + RANDOM_LONG(0,31) /*pitch 95–126*/, 0.5)`. NB: TS's EMIT_SOUND_DYN
       (`…PCcffiif`, @0xd0a8c) takes an 8th float — a slow-motion pitch-scale weight (it reads a
       per-entity slowmo field at edict+0x2d4 and biases pitch by `(const − f8)`; VERIFY exact
       formula — every melee site passes 0.5).
   - If held: `bApplyDamage = false` (no drop/knockback/sound, no damage).
   - **Always** (held or not): `pTarget->PunchScreen(flDamage * 0.5, attacker.origin − target.origin)`
     — but `PunchScreen__11CBasePlayerfG6Vector` @0x85f70 is an EMPTY function in the shipped binary
     (disasm-confirmed no-op).
8. **Damage** (`bApplyDamage`, i.e. also for NON-player victims — glass/breakables take coldcock):
   - `glast_coldcock = 1` (byte global @0x104481, GOT slot 0x167e64; siblings glast_closecombat
     @0x104480, glast_headshot @0x104482);
   - `ClearMultiDamage(); pTarget->TraceAttack(this->pev, flDamage, vecDir, &tr, bitsDamageType)`
     (virtual). The bitsDamageType argument is obscured by decompiler garbage at the call — VERIFY
     (likely DMG_CLUB; confirm via raw disasm at 0x86610 or a live break);
   - `ApplyMultiDamage(this->pev, this->pev); glast_coldcock = 0;`
   - `glast_coldcock` is consumed by `CHalfLifeMultiplay::PlayerKilled` (@0x795e4, reads @0x79949 and
     @0x7a57e): a kill landed inside this window gets its death-notice weapon string rewritten to
     `strcpy(buf, get_weapon_info(gun->m_weaponType)+0x44 /*name*/); strcat(buf, " Stock")` (string
     " Stock" @file 0xF87FD; the sibling "Kung Fu" string sits just before it for glast_closecombat),
     and a broadcast HUD message `UTIL_VarArgs("#TS_Coldcock #TS_FrgFr %s!", victim_netname)` via
     `TSHudMessageAll(0, 0xff, 0, 1, 0x14, msg)` (string @0xF895A). Both are client-visible swap
     contract (DeathMsg weapon string + HUD text) — byte-faithful required.
9. Returns `pTarget` (non-NULL even when the hit was kungfu-held), NULL on any gate/miss.

### The only caller in full — CTSGun::ColdCock @0x71dd0 (TS_gun.cpp)

- Function-local statics (gcc2 guard pattern `__tmp_N`): `vecSrc_1301`, `vecAiming_1303`,
  `vecDir_1305`, `rate_1307`. **`vecDir_1305` is never written** — the event below reads its .x/.y,
  which are therefore always 0.0 (original-source bug to replicate).
- Gate: `m_pActiveItem /*+0xcc*/ != NULL && get_weapon_info(m_weaponType /*+0xc8*/)->coldcock
  /*info+0x10*/ != 0`.
- `stunting = (m_pPlayer->pev->iuser4 & 0x70c30) != 0` — mid-stunt mask = DIVE|SLIDE|WHEEL_HI|
  WHEEL_LO|WALLJUMP_A|WALLCLIMB_A|ROLL in our TSSTUNT_* terms (0x10|0x20|0x400|0x800|0x10000|
  0x20000|0x40000).
- `UTIL_MakeVectors(m_pPlayer->pev->v_angle)`.
- If `m_pActiveInfo /*+0xd0*/`: if `iFireType /*info+0x348*/ == 2` (grenade) → `GrenadeIdle(this, 1)`;
  then `StopReload(this)`.
- `rate_1307 = 0.725`; if stunting `rate *= 1.5` → 1.0875.
- `+0x9c = +0xa0 = +0xa4 = UTIL_WeaponTimeBase() + rate` (the gun's next-attack timer trio — VERIFY
  exact names) and `m_pPlayer->+0x12c = same` (player next-attack gate);
  `+0xa8 = UTIL_WeaponTimeBase() + 2.0` (idle timer).
- `Unzoom(this)`.
- `vecSrc_1301 = m_pPlayer->GetGunPosition()` (vtable +0x168, slot 90); `vecAiming_1303 =
  gpGlobals->v_forward`; `damage = (float)info->coldcock(+0x10)`, `if (stunting) damage *=
  0.6666667`; `vecEnd = vecSrc + v_forward * 35.0` (0x23 int → fild).
- `hit = m_pPlayer->ColdCockAttack(vecSrc_1301, vecEnd, v_forward, 3, damage,
  stunting ? -1 : 0x27 /*39*/, false, false, false, false)` — **all four bools false** (disasm:
  four `pushw $0`; anim = `sete/dec/or $0x27`).
- `if (hit)`: `EMIT_SOUND_DYN(m_pPlayer_edict, 4 /*CHAN_BODY*/, "player/kevlarhit.wav", 1.0, 0.8,
  0, 60 + RANDOM_LONG(0,31) /*pitch 60–91*/, 0.5)` — the attacker-side hit confirm.
- `if (m_pActiveItem && m_pActiveItem->bOwned /*item+0x0*/)`:
  `pfnPlaybackEvent(1 /*FEV_NOTHOST*/, m_pPlayer_edict, m_usColdCock /*+0x966*/, 0.0, g_vecZero,
  g_vecZero, fparam1 = vecDir_1305.x (=0), fparam2 = vecDir_1305.y (=0), iparam1 = m_weaponType,
  iparam2 = m_pActiveItem->attachEquipped /*item+0x10*/, bparam1 = 0, bparam2 = 0)` —
  **fires even on a miss** (only gated on owning the item).
- Event handle: `CTSGun::Precache` @0x70660 assigns `+0x966 = PRECACHE_EVENT(1,
  "events/coldcock.sc")` — 2nd in the block: 0x964 fire.sc, **0x966 coldcock.sc**, 0x968 empty.sc,
  0x96a pump.sc, 0x96c closecombat.sc, 0x96e firelinked.sc, 0x970 reload.sc, +4 more.

### Per-weapon coldcock damage (s_weapon_info+0x10, from every `Write_<weapon>` immediate store)

Zeroed by all Write_ functions, then overridden by these (weapon can coldcock iff nonzero — the
field is BOTH the capability gate and the damage; Sparta()/SetSpartaWeapon does NOT touch +0x10):

aug 35 · barrett 70 · benellim3 35 · desert eagle 25 · m16 35 · m4 35 · m60 50 · mac10 30 ·
mp5sd 30 · sawedoff 50 · usas12 50 · all others 0 (no coldcock).

(VERIFY: only `c7 40/4x 10` immediate stores were scanned; a register-indirect store elsewhere
would be missed.)

### Constants recap

- Range **35** units, CheckHull with hull **3** (head_hull), TraceLine first.
- Damage: per-weapon 25–70; **× 2/3** while stunting (0x70c30 mask); knockback path (unused by
  coldcock) needs damage > **30** and sets victim `velocity.z = 235`.
- Cadence **0.725 s** (× 1.5 = 1.0875 s stunting); weapon idle +2.0 s.
- Sounds: victim "player/kungfuhit.wav" CHAN_BODY 1.0/0.8 pitch 95–126; attacker confirm
  "player/kevlarhit.wav" CHAN_BODY 1.0/0.8 pitch 60–91; both with slowmo float 0.5.
- Death notice: weapon string + `" Stock"`; HUD broadcast `#TS_Coldcock #TS_FrgFr %s!`.
- Victim PunchScreen(damage × 0.5, …) — no-op in shipped binary.

## 3. CALLERS

`ColdCockAttack` has exactly one caller: **`CTSGun::ColdCock` @0x71dd0** (call @0x72042 via PLT
0x5abf0) — argument detail above. Nothing else references the PLT stub (melee_refs.txt confirms:
the other decompiled refs in that file — VictimKatana/Knife/KungfuHold, CCombatBlock,
UpdateMeleeBar, BlockPlayer — belong to the sibling melee functions; the kungfu/knife paths go
through `CloseCombatAttack` @0x85f88 instead).

Chain: **`CTSGun::AlternateAttack` @0x71ba8** (the +alt2 input, virtual over
`CBasePlayerWeapon::AlternateAttack` weak @0xe367c) → `ColdCock()`. Its gate is
`m_pActiveItem != NULL && m_pActiveInfo->coldcock(+0x10) > 0` (signed `jle` skip) — **not clip**,
**no timer check inside AlternateAttack** (disasm-confirmed; the cadence must be enforced by
whatever dispatches AlternateAttack, presumably the player command path checking the player
`+0x12c` next-attack gate that ColdCock sets — VERIFY the dispatcher).
`ColdCockAttack` itself is a plain non-virtual method — no vtable/vrefs concerns.

## 4. OURS

- **`CBasePlayer::ColdCockAttack` — absent.** Our `dlls/ts_gun.cpp:1298 CTSGun::ColdCock()` inlines
  a simplified version of caller+helper in one function.
- Diffs of our `CTSGun::ColdCock` (ts_gun.cpp:1298–1322) vs original:
  - Trace: `UTIL_TraceLine` only, range **40** (orig: CheckHull = line + head-hull fallback,
    range **35**). No `CheckHull` anywhere in our tree.
  - Damage: fixed `25.0f * TSDamageMult()` (orig: per-weapon `info+0x10` 25–70, ×2/3 stunting).
    Our `s_weapon_info` (dlls/ts_weapon_info.h:73) has **no +0x10 field** — the coldcock
    damage/capability column is missing from the weapon table.
  - Cadence: `0.5f`, idle `+5.0f` (orig 0.725/×1.5 stunting, idle +2.0); we don't set the other
    two gun timers or the player next-attack gate; no `Unzoom()`, `StopReload()`, `GrenadeIdle()`
    (fire-type 2), and no stunting (iuser4 & 0x70c30) handling at all.
  - Anim: `m_TSAnim.Animate(m_pPlayer, TSA_FIRE)` after the trace (orig: anim id **39** — or −1
    stunting — before the trace, inside ColdCockAttack).
  - Sounds: we play "player/kungfuhit.wav" on the **attacker**, CHAN_WEAPON, `TSSlowPitch()` (orig:
    kungfuhit on the **victim**, CHAN_BODY, pitch 95–126, players only; plus the missing
    **"player/kevlarhit.wav"** attacker confirm pitch 60–91 — we precache kungfuhit at
    ts_gun.cpp:467 but never kevlarhit).
  - Gates: we skip the FL_WORLDBRUSH / EF_NODRAW / takedamage checks and the
    friendlyfire+PlayerRelationship teammate gate entirely.
  - No `VictimKungfuHold` block-absorb call (partial inline block check only in
    `CTSGun::MeleeAttack`, ts_gun.cpp:1177–1186, against our `m_flKungFuBlock` = orig +0x7a8).
  - No `glast_coldcock` (absent repo-wide), so no " Stock" death-notice suffix and no
    `#TS_Coldcock` HUD broadcast in our PlayerKilled.
  - No `events/coldcock.sc`: server precaches only fire/draw/knife (ts_gun.cpp:464–466), and
    cl_dll hooks no coldcock event (grep cl_dll: zero hits).
  - Our TraceAttack uses `DMG_CLUB | DMG_NEVERGIB` — original's bits unverified.
- **`CTSGun::AlternateAttack`** ts_gun.cpp:1284: gates on `m_flNextPrimaryAttack` +
  `m_iClip > 0` — both wrong vs original (`m_pActiveItem && coldcockDamage > 0`); the clip
  comment at :1288–1289 is an RE error.
- **`CBasePlayer::PunchScreen`** dlls/ts_player.cpp:486 + player.h:478 — exists as a no-op,
  matching the binary (disasm-confirmed). Correct; ColdCockAttack should still call it.
- **`VictimKungfuHold`** — absent as a function (partial inline in ts_gun.cpp:1177; the
  ts_stunt.cpp:720 comment acknowledges the subsystem isn't built).
- **`CBasePlayer::DropWeapon(tsweapon_t, float)`** @0x84900 — VERIFY presence in ours; coldcock
  doesn't exercise it (bDropWeapon=false) but the CloseCombatAttack callers likely do.

## 5. RECONCILE

1. **Write fresh `CBasePlayer::ColdCockAttack`** with the exact signature above, plus the pieces
   it needs: `CBasePlayer::CheckHull` (fresh; VERIFY its tail with its own decompile pass),
   `VictimKungfuHold` (fresh; absorbs/replaces the partial inline block in `CTSGun::MeleeAttack`
   ts_gun.cpp:1177–1186 — re-wire that to call it), globals `glast_coldcock`/`glast_closecombat`,
   and keep calling the no-op `PunchScreen`. Check `reference/coverage/funcfile.tsv` for the
   original .cpp placement before choosing the file.
2. **Rewrite `CTSGun::ColdCock`** as the original thin wrapper: stunt mask 0x70c30 → rate/damage/
   anim adjustments, the four timers + player next-attack, Unzoom/StopReload/GrenadeIdle, range 35,
   per-weapon damage, `ColdCockAttack(..., 3, dmg, stunting?-1:39, false,false,false,false)`,
   kevlarhit confirm (PRECACHE it — currently missing), coldcock event playback with the
   always-zero fparams (replicate the never-written static as literal 0.0f + provenance comment).
3. **Fix `CTSGun::AlternateAttack`**: gate = `m_pActiveItem && wi->iColdCockDamage > 0`; drop the
   invented clip gate; VERIFY the original cooldown dispatcher (live break on the mp.dll twin via
   binmap while spamming +alt2) before keeping our timer check.
4. **Weapon table**: add `int iColdCockDamage; // s_weapon_info+0x10` to ts_weapon_info.h and
   populate the 11 nonzero rows (aug/benelli/m16/m4 35, barrett 70, desert 25, m60/sawedoff/usas
   50, mac10/mp5sd 30).
5. **Death-notice/HUD (swap contract)**: add the `glast_coldcock` branch to our PlayerKilled —
   weapon string `<name> + " Stock"` (byte-exact, drives the client kill feed) and
   `TSHudMessageAll(0,0xff,0,1,0x14, UTIL_VarArgs("#TS_Coldcock #TS_FrgFr %s!", victim))`.
   Same pattern later serves glast_closecombat ("Kung Fu") and glast_headshot.
6. **Client↔server protocol**: precache+play `events/coldcock.sc` under `m_usColdCock` (provenance
   member order: 0x964 fire / 0x966 coldcock / 0x968 empty / 0x96a pump / 0x96c closecombat /
   0x96e firelinked / 0x970 reload), wire format exactly: FEV_NOTHOST, g_vecZero origin/angles,
   fparam1=fparam2=0.0, iparam1=weaponType, iparam2=attachEquipped, bparams 0. Hook it in cl_dll
   (ev_ts.cpp) — **VERIFY the original client.dll coldcock handler via dc.sh client before
   writing it** (viewmodel anim/sound behavior not covered here). Our invented `m_usMelee`/knife.sc
   stays separate (maps to the original's knife event further down the precache block).
7. **Sounds**: kungfuhit.wav moves to the victim/CHAN_BODY, pitch 95+RANDOM_LONG(0,31); model the
   trailing 0.5 slowmo float through our EMIT_SOUND_DYN equivalent (VERIFY TSSlowPitch vs the
   wrapper @0xd0a8c before claiming parity).
8. **VERIFY list (do not guess)**: TraceAttack bitsDamageType; CBasePlayer +0x71c and +0x12c names;
   the +0x9c/+0xa0/+0xa4 timer-trio names; the AlternateAttack dispatcher's cooldown gate;
   CheckHull's post-hull-hit tail (incl. vtable slot 46 check); EMIT_SOUND_DYN 8th-param formula;
   non-immediate writes to weapon_info+0x10; PlayerKilled's full glast_coldcock conditions
   (KO vs normal death handling around 0x7ac30's +0x728/+0x724 fields).
