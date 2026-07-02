# CTSGun::KatanaFire — analysis (ts_i386.so @ nm 0x733f0 / Ghidra 0x833f0)

Sources: typed decompile `melee_katanafire.c` + full objdump disassembly of 0x733f0–0x73b64
(the decompile had several Ghidra stack-slot merges; every constant below was re-derived from
the raw disasm + rodata reads, GOT base ebx=0x165E10, file offset = vaddr).

## 1. SIGNATURE

```
void CTSGun::KatanaFire()            // signatures-gcc2.txt:1130 "CTSGun::KatanaFire()"
                                     // gcc2 mangled: KatanaFire__6CTSGun, no args, void return
```
Original file (plate comment): `TS_gun.cpp`. Sibling methods: `KnifeFire__6CTSGun` @0x74044,
`KatanaThrow__6CTSGun` @0x73b68 — knife and katana have SEPARATE fire functions in the original.

## 2. BEHAVIOR

Local slot map (raw disasm; Ghidra merged several of these into `fStack_c4`/`pTStack_cc`):

| esp slot | meaning | per-branch values |
|---|---|---|
| 0xa | `bStunting` flag | `(player->pev->iuser4 & 0x70c30) != 0` |
| 0xb | `bLock` flag (post-swing STOP lock) | 1 for back-slash + duck-slash |
| 0xc | viewmodel/event anim (`iparam1` of the event) | 17 / 15 / 16 (see table) |
| 0x10 | swing REACH, int (units) | 50 default, 35 stunt, 55 back-slash |
| 0x14 | DAMAGE, float, **signed** (sign = swing direction) | 45 / 35 / **-75** / 65 / speed-scaled |
| 0x18 | full-body anim for `CTSAnimation::Animate` | 33/34/35/36/37, **-1** for stunt |
| 0x1c | attack DELAY (s) | 0.7 / 0.9 / 1.0 / 1.1 |

Step by step:

1. **Gate:** `if (m_pActiveItem == NULL) return;` (gun+0xcc — active arsenal slot; NULL = kung-fu).
   No button/next-attack checks here — those are in the caller (PrimaryAttack/ItemPostFrame).
2. `UTIL_MakeVectors(m_pPlayer->pev->v_angle)` (v_angle @ player pev+0x74).
3. **Stance classification:**
   - `bStunting = (player->pev->iuser4 & 0x70c30) != 0` — mask = DIVE(0x10)|SLIDE(0x20)|
     WHEEL_HI(0x400)|WHEEL_LO(0x800)|WALLJUMP_A(0x10000)|WALLCLIMB_A(0x20000)|ROLL(0x40000)
     (matches our `ts_stunt.h` bit names exactly).
   - `speed = |player velocity|`. **movingBack** = (speed <= 100.0) ? `(button & IN_BACK(0x10))`
     : `dot(v_forward, velocity.Normalize()) <= -0.5` (double const -0.5).
4. **Branch select** (priority order), setting {reach, dmg, delay, vmAnim, bodyAnim, bLock}:

   | branch | condition | reach | damage | delay | vmAnim (iparam1) | body anim | extra |
   |---|---|---|---|---|---|---|---|
   | STUNT slash | bStunting | 35 | 35.0 | 1.0 | 17 (0x11) | **-1** (keep stunt anim) | — |
   | BACK slash | movingBack | 55 | **-75.0** (negative → swings BEHIND) | 1.0 | 15 (0xf) | 35 (0x23) | bLock=1 |
   | DUCK slash | on ground AND (`button & IN_DUCK(4)` OR `this->pev->flags & FL_DUCKING(0x4000)` — the **GUN's own pev**, see VERIFY) | 50 | 65.0 | 0.9 | 17 | 37 (0x25) | bLock=1; `iuser4 |= PRONE(4)|STOP(1)`; `view_ofs = (0,0,16)`; `player+0x4f4 = 2.0f` |
   | NORMAL slash | on ground, not ducking | 50 | 45.0 | 0.7 | 17 or 16 | 33/34 (0x21/0x22) alternating | `m_lastMeleeAnim` (gun+0xa04): if not in {1,2} → `RANDOM_LONG(1,2)`; ==1 → vmAnim 17, body 33, set 2; ==2 → vmAnim 16, body 34, set 1 |
   | AIR slash | not FL_ONGROUND (pev flags & 0x200 clear) | 50 | `clamp(speed * 0.1538461, 35, 55)` (0.1538461 = 2/13 ≈ speed/6.5) | 1.1 | 17 | 36 (0x24) | — |

5. `CTSAnimation::Animate(&player+0x28c, player, bodyAnim)` — full-body third-person anim
   (called even with -1 in the stunt branch).
6. **`damage *= CBasePlayer::GetMeleeMultiplier(player)`** (block-bar stamina multiplier —
   the fmul was dropped by Ghidra; it's in the disasm @0x737d7-0x737e0).
7. **Trace:** `start = player->GetGunPosition()` (virtual, player vtbl +0x168 — the decompile's
   bogus `vtbl[1].TraceAttack` = 0x114 struct + 0x54 = 0x168).
   `dir = v_forward * (float)reach`; `sign = (damage < 0) ? -1 : 1`; `end = start + sign*dir`
   (so the back-slash traces 55 units BEHIND the player).
   `CBasePlayer::CheckHull(start, end, tr, (damage<0) ? 1 : 3)` (gcc2 sig
   `CheckHull(Vector, T1, TraceResult&, int)`; last int = 3 forward / 1 backward — VERIFY its
   exact semantics, likely number of hull sweeps).
8. **Hit resolution:** if `tr.flFraction < 1.0`:
   - hit edict = `tr.pHit`; if NULL → `pfnPEntityOfEntOffset(0)` (world). Victim =
     `edict->pvPrivateData` (+0x7c).
   - Skip unless victim `IsAlive()` (vtbl +0xb4) and NOT `pev->effects & EF_NODRAW (0x80)`.
   - **Block check:** `VictimKatanaHold(player, victim, damage)` (@0x66c8c, TS_gamerules.cpp).
     Returns 1 (swing deflected, NO damage) iff victim `IsPlayer()` AND victim's katana-block
     window `victim+0x7b0 > 0` (set by `CTSGun::CCombatBlock` when the victim's active weapon
     iFireType==10) AND victim block-bar `victim+0x774 > 0.1`. On block:
     `UTIL_Sparks(midpoint(origins) + (0,0,20))`, `EMIT_SOUND_DYN(attacker edict, CHAN_BODY(4),
     "weapons/knife/knife_hit.wav", 1.0, 0.8, 0, 100, 0.5f)`, attacker pending-melee dmg
     `+0x7a0 = 0`, attacker `+0x12c = 0.75f`, victim `+0x12c = 0.35f` (melee-lock/cooldown
     stamps — VERIFY units, our m_blockCooldown), `UpdateMeleeBar(attacker, 33.0)` (bar drain,
     nets the gmsgKFuPower update).
   - **Damage:** if not blocked: `ClearMultiDamage(); victim->TraceAttack(player->pev,
     fabsf(damage), gpGlobals->v_forward, &tr, 0x1080 /* DMG_CLUB|DMG_NEVERGIB */);
     ApplyMultiDamage(player->pev, player->pev);`
   - **Bloody blade:** if victim `IsPlayer()` (vtbl +0xc8): `m_pActiveItem->+8 = 1` (per-item
     phase slot = bloody-blade state) and `this->pev->body = 1` (bloody katana submodel), then
     `EMIT_SOUND_DYN(VICTIM's edict, CHAN_BODY(4), "weapons/katana/hit-body.wav", vol 1.0,
     attn 0.8, flags 0, pitch 95+RANDOM_LONG(0,31), trailing float 0.5f)` — note the sound is
     on the victim, and TS's EMIT_SOUND_DYN has an extra trailing float arg (VERIFY meaning,
     0.5 everywhere in the melee code).
9. **Whiff-lunge (miss or dead/nodraw target; NOT on a block):** `player+0x79c = 0.25f` (window),
   `player+0x7a0 = damage` (SIGNED — the back-slash stores -75 → the PreThink lunge pushes
   backward), `player+0x7a4 = (byte)2` (melee type katana; kung-fu path writes 0 there).
10. **Post-swing lock:** if `bLock && !(iuser4 & STOP(1))`: `player+0x4dc = delay * 0.9`
    (STOP-timer, cleared by the stunt system per STUNT.md) and `iuser4 |= STOP(1)`
    (velocity-damp lock) — back-slash and duck-slash root the player briefly.
11. **Client event:** `pfnPlaybackEvent(FEV_NOTHOST(1), player edict, gun+0x974, 0.0,
    g_vecZero, g_vecZero, fparam1=0, fparam2=0, iparam1=vmAnim(15/16/17),
    iparam2=m_pActiveItem->+8 (bloody 0/1), 0, 0)` — gun+0x974 = the **`events/katana.sc`**
    handle (EVENTS.md: +0x974 KatanaFire, client handler LAB_10044230). The viewmodel anim and
    the swing/whoosh presentation are chosen CLIENT-side from iparam1; the bloody viewmodel from
    iparam2. FEV_NOTHOST because CTSGun melee is client-predicted for the shooter.
12. **Timers:** `m_flNextPrimaryAttack (gun+0x9c) = m_flNextSecondaryAttack (gun+0xa0)
    = UTIL_WeaponTimeBase() + delay`; `m_flTimeWeaponIdle (gun+0xa8) = UTIL_WeaponTimeBase() + 3.0`;
    `player+0x4f8 = delay` (player-side melee-busy/anim time — VERIFY consumer).

Engine funcs used (GOT): pfnRandomLong (+0x168), pfnPEntityOfEntOffset (+0x114),
pfnPlaybackEvent (+0x1ec). Sounds referenced server-side: `weapons/katana/hit-body.wav`
(+ `weapons/knife/knife_hit.wav` inside VictimKatanaHold). NO server-side miss/swing sound —
that comes from katana.sc on the client.

## 3. CALLERS

- **`CTSGun::PrimaryAttack` @0x71a20 is the ONLY static caller.** Disasm @0x71aa2:
  `cmpb $0xa, 0x348(%eax)` — dispatches on `m_pActiveInfo->iFireType (+0x348) == 10` →
  `call KatanaFire@plt` (@0x71aaf), right after the `iFireType == 3 → KnifeFire` dispatch
  (@0x71a91). So katana = fire-type 10, knife = 3, exactly matching our `TSFT_KATANA = 10`.
- `melee_refs.txt` contains no other caller (its `refs` are for VictimKatanaHold/CCombatBlock/
  ColdCock etc.); `PrimaryAttack` itself is driven by `CBasePlayerWeapon::ItemPostFrame` holding
  IN_ATTACK — i.e., the katana slashes full-auto while held, gated only by
  m_flNextPrimaryAttack (+delay set in step 12).
- Related dispatch context: `CTSGun::CCombatBlock` @0x85ebc (RELOAD) is what arms the victim's
  katana-block window (+0x7b0 = 0.5 when the blocker's own weapon iFireType==10) that
  `VictimKatanaHold` tests.

## 4. OURS

Equivalent logic: **`CTSGun::MeleeAttack(const s_weapon_info*)` — dlls/ts_gun.cpp:909**
(declared ts_gun.h:92, self-described as "@ KnifeFire/KatanaFire ... SLICE"). Dispatch:
`CTSGun::PrimaryAttack` ts_gun.cpp:608-610 (`TSFT_KNIFE || TSFT_KATANA → MeleeAttack(wi)`) —
same call site shape as the original but MERGED knife+katana into one invented method.

What matches already:
- Caller shape (PrimaryAttack fire-type dispatch; full-auto while held — ts_gun.cpp:911 comment).
- `UTIL_MakeVectors(v_angle)` + `GetGunPosition()` start (ts_gun.cpp:914-915).
- Damage bits `DMG_CLUB | DMG_NEVERGIB` (= 0x1080) (ts_gun.cpp:970).
- `ClearMultiDamage / TraceAttack / ApplyMultiDamage` sequence (ts_gun.cpp:968-971).
- Base damage 45 / rate 0.7 (our weapon table ts_weapon_info.cpp:183) — equals the original's
  NORMAL-slash constants (but the original hardcodes per-stance values in KatanaFire, it does
  NOT read the table here).
- `m_lastMeleeAnim` member exists (ts_gun.h:145) but is used only by kung-fu CCombatAttack.
- `GetMeleeMultiplier` exists (player.h:479) but MeleeAttack doesn't apply it (uses
  `TSDamageMult()` slow-mo scale instead — different thing).
- CCombatBlock / block-bar / m_flKungFuBlock infrastructure exists (ts_gun.cpp:1247, 1279)
  but only the KUNG-FU block deflect is wired (ts_gun.cpp:1177-1188, an inline approximation
  of VictimKungfuHold); `Victim*Hold` themselves are absent (player.cpp:2040 notes them
  "unbuilt").

Divergences (mechanism level):
1. **No stance system at all** — the 5 branches (stunt/back/duck/normal/air) with per-stance
   damage 35/-75/65/45/clamp(speed*2/13,35,55), reach 35/55/50, delay 1.0/1.0/0.9/0.7/1.1,
   body anims 33-37 and vm anims 15/16/17 are entirely missing. Ours: one branch, range 64
   (wrong — orig 50), damage 45 flat, delay = table 0.7 flat.
2. **Reverse back-slash missing** (negative damage → trace behind, backward whiff-lunge).
3. **`UTIL_TraceLine` instead of `CBasePlayer::CheckHull(start,end,tr,3|1)`** (hull sweep;
   CheckHull is the original's melee tracer, also used by kung-fu).
4. **VictimKatanaHold block missing** — a blocking victim (CCombatBlock window +0x7b0 + bar
   +0x774>0.1) deflects the swing: sparks/knife_hit.wav/UpdateMeleeBar(33)/mutual melee locks.
5. **Bloody blade missing** — `m_pActiveItem->+8 = 1` + `pev->body = 1` + event iparam2.
6. **Whiff-lunge missing** (player+0x79c=0.25 / +0x7a0=signed dmg / +0x7a4=2) — flagged as a
   documented SLICE in ts_gun.cpp:994 for kung-fu; katana uses the same pending block.
7. **Post-swing STOP lock missing** (iuser4 STOP + player+0x4dc = delay*0.9; duck-slash also
   PRONE + view_ofs 16 + player+0x4f4=2.0).
8. **Protocol divergence (swap-contract!):** original plays **`events/katana.sc`** (gun+0x974)
   EVERY swing with `(iparam1=vmAnim 15/16/17, iparam2=bloody)`, FEV_NOTHOST. Ours plays
   `events/knife.sc` (m_usMelee) only on surface hits with `(origin=tr.vecEndPos,
   angles=v_angle)` and flags 0 (ts_gun.cpp:963-967), plus server-side SendWeaponAnim +
   EMIT_SOUND miss/hit sounds (ts_gun.cpp:929-956) that the original does NOT emit (server only
   emits hit-body.wav, on the VICTIM's edict, and only for player victims).
9. Idle timer +5.0 vs original +3.0; original also sets m_flNextSECONDARYAttack (+delay) and
   player+0x4f8 (ours sets neither).
10. Our TSA_FIRE Animate vs the original's per-stance anims 33-37 (and -1 during stunts).

## 5. RECONCILE

Plan (server side, dlls/ts_gun.cpp):
- **Split `MeleeAttack` into the original's `KnifeFire()` and `KatanaFire()`** (audit/coverage
  match by NAME; KnifeFire needs its own RE pass — do not assume it mirrors KatanaFire).
  `PrimaryAttack` dispatch: `iFireType==3 → KnifeFire()`, `iFireType==10 → KatanaFire()`
  (replaces the merged TSFT_KNIFE||TSFT_KATANA branch at ts_gun.cpp:608).
- Write `KatanaFire` fresh from §2: the 5-stance table (constants above are exact rodata/imm
  values), signed-damage reverse trace, `CheckHull(start, end, tr, dmg<0?1:3)`,
  `GetMeleeMultiplier` damage scale, `VictimKatanaHold` block gate, bloody-blade state
  (`pev->body=1` + item phase slot — reuse our per-item `m_shotgunPhase`-style slot or add
  `m_bloodyBlade`, provenance `m_pActiveItem+8`), whiff-lunge writes, STOP/PRONE lock writes,
  timers (+delay on BOTH next-attacks, idle +3.0).
- **Absorbs/replaces our invented:** `MeleeAttack` (katana half), the server-side
  swing/hit EMIT_SOUNDs and `SendWeaponAnim`/`m_slashStep` cycling for the katana (the original
  drives viewmodel anim through katana.sc iparam1), and `TS_TriggerViewShake`'s katana usage
  should be re-verified against the client katana.sc handler (LAB_10044230) rather than the
  knife handler. Keep `m_lastMeleeAnim` (shared member, gun+0xa04) — katana NORMAL slash now
  also cycles it 1↔2.
- **New/ported pieces needed:** `VictimKatanaHold(CBasePlayer*, CBaseEntity*, float)` in the
  gamerules/TS_gamerules.cpp home (its knife/kungfu siblings exist in the .so — port together);
  the pending-melee/whiff-lunge PreThink consumer of player {+0x79c,+0x7a0,+0x7a4} (currently a
  documented SLICE; without it the miss-lunge writes are inert); the katana-block arm in our
  CCombatBlock already exists — wire its fireType-10 window to the same member VictimKatanaHold
  reads (victim +0x7b0 → name it e.g. `m_flKatanaBlock`, sibling of m_flKungFuBlock).
- **Client↔server protocol (byte-faithful):** precache + play `events/katana.sc` with EXACTLY
  `PLAYBACK_EVENT_FULL(FEV_NOTHOST, player edict, m_usKatana, 0, g_vecZero, g_vecZero, 0, 0,
  vmAnim, bloody, 0, 0)`; client_dll needs the katana.sc hook (original handler client.dll
  RVA 0x44230) honoring iparam1 anim + iparam2 bloody body. Drop the server EMIT_SOUND
  whoosh for the katana (miss sound belongs to the event) and keep only hit-body.wav
  (victim edict, CHAN_BODY, pitch 95+rand(0,31), attn 0.8) — plus knife_hit.wav in the block
  path. Our knife.sc usage stays for KnifeFire only (verify separately).

VERIFY list (explicit unknowns — do NOT guess):
- The duck-slash condition tests `this->pev->flags & FL_DUCKING` on the **GUN's** entvars
  (disasm @0x73624 reads this+0), which is normally never set on a carried item — likely an
  original-source `pev` vs `m_pPlayer->pev` slip, making IN_DUCK the only live trigger. Decide
  replicate-as-is (recommended for fidelity; harmless) after a live gdb check (break mp.dll
  KatanaFire while ducked WITHOUT holding duck mid-swing).
- `CheckHull` 4th arg semantics (3 vs 1) — needs its own decompile before porting (it's also
  the kung-fu tracer).
- Consumers/meaning of player+0x4f4 (=2.0 duck-slash), +0x4f8 (=delay), +0x4dc (=delay*0.9
  STOP timer — STUNT.md says the stunt system clears it), and the +0x12c melee-lock stamps in
  VictimKatanaHold (0.75/0.35 — units look like durations but CCombatBlock compares +0x12c
  against WeaponTimeBase as an absolute time).
- The trailing 0.5f arg of TS's 8-arg `EMIT_SOUND_DYN(edict*,int,char*,float,float,int,int,float)`.
- Whether vmAnim 15/16/17 map 1:1 to v_katana.mdl sequences or are remapped inside katana.sc
  (decompile client LAB_10044230 via `dc.sh client 0x44230` when the Ghidra project is free).
- Stunt-branch `Animate(player, -1)` — confirm CTSAnimation::Animate treats -1 as no-op.
