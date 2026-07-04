# CTSGun::CCombatAttack decode (cont85) — decompile-exact

Server binary `ts_i386.so`. **CCombatAttack = nm 0x76078 (Ghidra 0x86078)** —
the prompt's "nm 0x86078" was wrong (nm 0x86078 is mid-`CloseCombatAttack`).
CloseCombatAttack = nm 0x85f88. ColdCockAttack = nm 0x86610 (its own twin fn).

CCombatAttack **calls** CloseCombatAttack (direct call @0x76cf6) — it does NOT
inline it. cont84's decompile is the right function; only its *variable names /
call-arg slots* are shifted by Ghidra's stack corruption. Everything below is
re-derived from the raw objdump of 0x76078 and cross-checked against the callee
control flow and our own headers.

## ⚠️ Two corrections to the prompt's "established facts" (objdump-proven, HIGH)

1. **iAnim (param_7 to CloseCombatAttack) = the MOVE-ID, NOT local_104 (1/2/3).**
   The call pushes `[base+0x2c]` as param_7 (`push 0x40(esp)` @0x76ccc, esp=base-0x14).
   `[base+0x2c]` is the slot that gets `movl $0x0a..$0x19` per branch and is copied to
   `m_lastMeleeAnim` (gun+0xa04) @0x76a92. Its values (0x0a..0x19) are EXACTLY
   HandleCloseCombat's switch cases. cont84 mislabels this slot `local_100`/passes
   `local_104`.

2. **flDamage (param_6) = the speed-scaled LUNGE, NOT the 35/45/55 value.**
   The call pushes `[base+0x1c]` as param_6 (`push 0x34(esp)` @0x76cd0, esp=base-0x18).
   `[base+0x1c]` is the clamped speed-scaled value, later `*= GetMeleeMultiplier`
   (@0x76aaa) and `*= 2` if KungFu (@0x76ac7), and is the ± source of
   `m_flWhiffLungeForce` (player+0x7a0). The **35/45/55 value (`[base+0x18]`) is the
   TRACE REACH** — it is `fildl`'d @0x76c10 and multiplied by `v_forward` to build
   `vecEnd` (param_3) for the hull sweep. Our own header already agrees:
   `player+0x7a0 = m_flWhiffLungeForce (SIGNED, doubled if KungFu)` = ±`[0x1c]`.

## Definitive stack-slot map (base-relative, esp stable in the body)

| slot | meaning | cont84 name |
|---|---|---|
| 0x07 (byte) | BlockPlayer-enable flag (set 1 in onground kicks) | cVar24-ish |
| 0x08 (int) | **a3** = 0x3..0xb → PlaybackEvent **iparam2** (first-person view-anim index) | a3 |
| 0x0f (byte) | un-stunt enable (init 1; cleared→skip the iuser4-0x20000 clear) | a4>>0x18 |
| 0x10 (float)| speed = \|velocity\| (3-D) | fVar2 |
| 0x14 (int)  | **bVar9** = 2 + movingBack (2 or 3) | bVar9/fVar2 |
| 0x18 (int)  | **REACH** 0x23/0x2d/0x37 = 35/45/55 → fildl → vecEnd = vecSrc + reach·(±v_forward) | local_114 |
| 0x1c (float)| **LUNGE/damage**: clamped speed-scaled; `*=MeleeMult`; `*=2` if KungFu; = param_6 flDamage; ±→ m_flWhiffLungeForce | local_110 |
| 0x20 (float)| **whiff-lunge TIME** window → m_flWhiffLungeTime (player+0x79c) on miss | local_10c |
| 0x24 (float)| **refire** → gun+0x9c/+0xa0 & player+0x4f8; BlockPlayer scales this*0.75 (see note) | local_108 |
| 0x28 (int)  | **local_104** = 1/2/3 category → PlaybackEvent **iparam1**; !=0 gates the whole tail | local_104 |
| 0x2c (int)  | **MOVE-ID** = m_lastMeleeAnim (gun+0xa04) = param_7 iAnim; 0x0a..0x19 or 0xffffffff | local_100 |

## Preamble (every call)
- `speed = |pev->velocity|` (0x10).
- **3 early-return gates** (all `if (UTIL_WeaponTimeBase() < X) return`):
  `X = m_pPlayer->m_flNextAttack (player+0x12c)`, then `gun+0x9c`, then `gun+0xa0`.
- `UTIL_MakeVectors(Vector(0, v_angle.y, v_angle.z))`.
- **movingBack** = `speed<=100 ? (button & IN_BACK)
  : dot(gpGlobals->v_forward, normalize(velocity)) <= -0.5`.  **bVar9 = movingBack + 2.**
- `cVar17 = (pev->iuser4 & 0x50c30) != 0`  (stunt/slowmo/duck-active gate).
- defaults: refire(0x24)=0.6, lunge(0x1c)=0, move-id(0x2c)=0xffffffff, local_104(0x28)=0.

## Deliverable 1 — branch table (SOURCE / first-match order)

Guards use: IN_ATTACK=1, IN_ATTACK2=0x800, IN_DUCK=4, IN_BACK=0x10;
FL_ONGROUND=0x200, FL_DUCKING=0x4000; `gunDuck` = `gun->pev->flags & FL_DUCKING`.
move-id → HandleCloseCombat string in ( ). "refire/whiff/reach/lunge" are the raw
per-branch scalars BEFORE the shared tail scaling.

| # | branch / GUARD (first match) | move-id (0x2c) | a3 (0x08) | l104 (0x28) | reach (0x18) | lunge (0x1c) | refire (0x24) | whiff-t (0x20) | bVar9 | side-effects |
|---|---|---|---|---|---|---|---|---|---|---|
| A | `cVar17` (iuser4 & 0x50c30) | 0xffffffff (none) | 4 | 1 | 35 | 15 | 0.55 | 0.15 | 2 | m_comboStep=0 |
| — | `!(button&IN_ATTACK)` → `local_10c=lunge`; if `!(button&IN_ATTACK2)` → **no move** (fall to tail, l104=0 ⇒ whole tail skipped) | — | — | 0 | — | — | — | — | — | idle/no-op |
| B1 | `IN_ATTACK2` & `FL_ONGROUND` & `(IN_DUCK \|\| gunDuck)` → **DUCK KICK** | 0x14 (duckkick) | 6 | 2 | 45 | 35 | 1.0 | 0.35 | **2 (forced)** | iuser4\|=4; iuser4\|=1; view_ofs=(0,0,16); stunt+0x4(player+0x4f4)=2.0f |
| B2 | else `bVar9==3` → **BACK/JUMP KICK** | 0x11 (backkick) | 7 | 3 | 55 | `speed*0.303` clamp[15,80] | 1.35 (→1.0 & uVar25=1 if FL_ONGROUND) | 0.3 | 3 | vecDir uses v_angle w/ z=0 (backkick, see D2) |
| B3 | else `!FL_ONGROUND & speed>270 & IN_DUCK & dot>0.5` → **SIDE KICK** | 0x10 (sidekick) | 8 | 2 | 45 | `speed*0.1538` clamp[15,45] | 1.0 | 0.25 | 2 | m_kickCombo=0; a4&=0xffffff (skip un-stunt); iuser4\|=4; view_ofs=(0,0,16); player+0x4f4=2.0f |
| B4 | else `FL_ONGROUND \|\| speed<=200` → GROUND KICK; sub-split: | | | 2 | 45 | | | | 2 | |
| B4a | · `(player+0x815 & 8) \|\| m_kickCombo>2` → **KICK FINISHER** | cycle {0x0a circfront,0x0e latback,0x0d circback,0x0f circspin} by m_lastMeleeAnim (no-repeat; RandomLong(0,3) fallback) | 0xa | 2 | 45 | 25 | 0.5 | 0.3 | 2 | m_kickCombo=0; uVar25=1 |
| B4b | · else → **KICK COMBO STEP** | cycle {0x0b circkick,0x0c circkicklow} by m_lastMeleeAnim (RandomLong(0,1)) | 9 | 2 | 45 | 15 | 0.3 | 0.2 | 2 | m_kickCombo++; uVar25=1 |
| B5 | else (airborne & speed>200) → **AIR KICK** | cycle {0x0f circspin,0x13 airkick2,0x12 airkick} by m_lastMeleeAnim (RandomLong(0,2)) | 0xa | 2 | 45 | `speed*0.1` clamp[15,30] | 0.75 | 0.25 | 2 | m_kickCombo=0; uVar25=0 |
| C | `IN_ATTACK` & `!FL_ONGROUND` → **AIR PUNCH** | 0x18 (punchair) | 5 | 1 | 35 | 25 | 0.5 | 0.15 | 2 | m_comboStep=0 |
| D1 | `IN_ATTACK` & FL_ONGROUND & `(m_comboStep>2 \|\| (IN_DUCK & !gunDuck))` → **UPPERCUT** | 0x19 (uppercut) | 0xb | 1 | 35 | 30 | 0.65 | 0.15 | 2 | m_comboStep=0 |
| D2 | else `((player+0x814 ^1)&1)==0 \|\| m_comboStep>2` → **PUNCH (reset)** | 0x17 (punchright) | 4 | 1 | 35 | 20 | 0.35 | 0.15 | 2 | m_comboStep=0 |
| D3 | else → **PUNCH COMBO STEP (jab)** | 0x16 (punchleft) | 3 | 1 | 35 | 10 | 0.25 | 0.10 | 2 | m_comboStep++ |

Notes:
- move-id denormals decoded: 0x18=punchair, 0x19=uppercut, 0x17=punchright, 0x16=punchleft,
  0x14=duckkick, 0x11=backkick, 0x10=sidekick, 0x0f=circspin, 0x0e=latback, 0x0d=circback,
  0x0c=circkicklow, 0x0b=circkick, 0x0a=circfront, 0x13=airkick2, 0x12=airkick — all match
  HandleCloseCombat's cases exactly.
- The cVar17 branch (A) and air-punch (C), uppercut (D1) all `goto LAB_000862f9`
  (sets whiff-t=0.15, m_comboStep=0) then the shared `local_104=1; bVar9=2; reach=35`.
- The no-repeat cycles read/compare `this->m_lastMeleeAnim` (gun+0xa04) against the prior
  move-id and pick the "other" one, RandomLong on first use.

## Deliverable 2 — shared tail (LAB_000869a3, runs iff local_104 != 0)

1. **un-stunt block** (only if `[0x0f]!=0`, i.e. a4 high byte still 1 — SIDE KICK cleared it):
   if `iuser4 & 0x20000`: `iuser4 &= ~0x20000; iuser4 &= ~4;`
   view_ofs = `(FL_DUCKING) ? (0,0,16) : (0,0,28)`; `pev->friction=1.0; pev->gravity=1.0`.
2. `m_lastMeleeAnim (gun+0xa04) = move-id` (@0x76a92).
3. `lunge(0x1c) *= CBasePlayer::GetMeleeMultiplier(m_pPlayer)`; if `KungFu(m_pPlayer)` then `lunge += lunge`.
4. **vecDir**: copy `m_pPlayer->pev->v_angle` into local; if `bVar9==3` (backkick) **zero the z**;
   `UTIL_MakeVectors(that)`; the CheckHull/attack **vecEnd = vecSrc + (±1)·reach·v_forward**,
   where `±1 = (bVar9==3) ? -1 : +1` (backkick reverses), and reach = `[0x18]` (35/45/55).
   vecSrc = a Vector-returning player vmethod at vtbl offset 0x168 (vptr@0x60) — GetGunPosition/EyePos.
5. **`iVar11 = CloseCombatAttack(m_pPlayer, vecSrc, vecEnd, vecDir_normalized,
     iHull=(bVar9!=3)?3:1, flDamage=lunge, iAnim=move-id,
     bDisarm=1, bPlayersOnly=0, bKnockback=1, bForceTeamSafe=0)`**
   (all 4 bools are compile-time constants: 1,0,1,0).
6. **miss-lunge store** — `if (iVar11==0 && 0.001 < [0x20])`:
   `player+0x79c (m_flWhiffLungeTime) = [0x20]` (whiff-t);
   `player+0x7a0 (m_flWhiffLungeForce) = (bVar9==3) ? (lunge ^ 0x80000000) : lunge`  (sign-flip on backkick);
   `player+0x7a4 (m_iWhiffLungeType) = 0` (byte).
7. **BlockPlayer** — `if ([0x07]!=0 && ((iuser4 ^ 1) & 1)!=0)  BlockPlayer(m_pPlayer, X*0.75)`.
   Blocks **the attacker himself (m_pPlayer)**, gated on the onground-kick flag AND `iuser4&1==0`.
   `X` = the block-stun scalar (cont84 `local_10c`). See OPEN QUESTION on X's exact slot.
8. **PlaybackEvent(m_usCloseCombat)**:
   `PlaybackEvent(flags=1, m_pPlayer->edict, gun+0x96c (m_usCloseCombat), delay=0,
      origin=g_vecZero, angles=g_vecZero, fparam1=0, fparam2=0,
      iparam1=[0x28]=local_104 (1/2/3 category), iparam2=[0x08]=a3 (0x3..0xb view-anim), bparam1=0, bparam2=0)`.
9. **timers** (always, even if local_104==0 the fn jumps straight here):
   `t = UTIL_WeaponTimeBase();`
   `gun+0xa0 = gun+0x9c = t + refire(0x24)`;  `gun+0xa8 = UTIL_WeaponTimeBase() + 2.0`;
   `player+0x7a8 (m_flKungFuBlock) = 0`;  `player+0x4f8 (m_flLastMeleeRate) = refire(0x24)`.

## Deliverable 3 — CTSAnimation::Animate + activity mapping (ANSWER)

**iAnim is NOT 1/2/3 — it is the MOVE-ID (0x0a..0x19).**
`CloseCombatAttack` calls `CTSAnimation::Animate(&player+0x28c, player, iAnim)` at its top
(@0x95f88 line 1143). `Animate`'s `a1 == iAnim`. `IsInCloseCombat(a1)` is
`a1==0x3a || (unsigned)(a1-10) < 0x1e` (i.e. 0x0a..0x27, or 0x3a) — every CCombatAttack
move-id qualifies, so Animate immediately routes to **`HandleCloseCombat(this, player, a1)`**,
whose `switch(a1)` maps the move-id → the 3rd-person body sequence string:
`0x0a circfront · 0x0b circkick · 0x0c circkicklow · 0x0d circback · 0x0e latback ·
0x0f circspin · 0x10 sidekick · 0x11 backkick · 0x12 airkick · 0x13 airkick2 ·
0x14 duckkick · 0x16 punchleft · 0x17 punchright · 0x18 punchair · 0x19 uppercut`.
Animate does **not** use local_104 or a3 for the close-combat body — the move-id alone
selects it. `move-id == 0xffffffff` (stunt branch A) makes Animate `return` at `a1<0`
(no body anim). **So the orchestrator is right to pass a move-id-range code as iAnim; the
value must equal the HandleCloseCombat case (0x0a..0x19), i.e. `m_lastMeleeAnim`.** The
1/2/3 (`local_104`) and 0x3..0xb (`a3`) are CLIENT event params only (PlaybackEvent
iparam1/iparam2 → first-person view-model), never the server iAnim. HIGH.

## Deliverable 4 — field-offset map

Gun (CTSGun, `this`):
- `gun+0x9c` / `gun+0xa0` — melee refire gates (both read as early-return, both set = t+refire). VERIFY vs m_flNextPrimary/SecondaryAttack analog.
- `gun+0xa4` — m_flNextAlternateAttack (NOT touched here; ColdCock path).
- `gun+0xa8` — set to t+2.0 (idle/next-melee window). 
- `gun+0x96c` — **m_usCloseCombat** event handle.
- `gun+0x9fc` — **m_comboStep** (punch combo).
- `gun+0xa00` — **m_kickCombo** (kick combo).
- `gun+0xa04` — **m_lastMeleeAnim** = the move-id (no-repeat cycling + this frame's iAnim).
- `gun+0x80` — m_pPlayer (ebp holds `this`; `0x80(ebp)` = m_pPlayer throughout).

Player (m_pPlayer):
- `player+0x12c` — **m_flNextAttack** (first early-return gate).
- `player+0x250` — **pev->iuser4** (bits: `0x50c30` stunt/slowmo/duck gate; `|=1`, `|=4`, clear `0x20000`/`0x4`; low bit `&1` gates BlockPlayer).
- `player+0x4f0` — **CTSStunt** (embedded sub-object). Its `+0x4` = `player+0x4f4`.
- `player+0x4f4` — **stunt+0x4 timer = 2.0f** (0x40000000) — written in DUCK KICK & SIDE KICK.
- `player+0x4f8` — **m_flLastMeleeRate** = refire.
- `player+0x79c` — **m_flWhiffLungeTime** = whiff-t `[0x20]` on miss.
- `player+0x7a0` — **m_flWhiffLungeForce** = ±scaled-lunge `[0x1c]` on miss (sign-flip if bVar9==3).
- `player+0x7a4` — **m_iWhiffLungeType** = 0 on miss.
- `player+0x7a8` — **m_flKungFuBlock** = 0 (cleared at tail end).
- `player+0x814` — RESOLVED (cont.86): stock `m_afButtonPressed` (PreThink edge trio +0x810/814/818); D2 = pressed & IN_ATTACK.
- `player+0x815` — RESOLVED (cont.86): byte 1 of m_afButtonPressed; &8 = IN_ATTACK2 (0x800) freshly pressed (B4a).
- `player+0x510/0x514/0x518` — stored move-dir used by Animate (not CCombatAttack).
- `player+0x1a5` (flags byte+1) — FL_ONGROUND (bit 0x2) / FL_DUCKING (bit 0x40) tests.

Two early-return timer gates confirmed: **player m_flNextAttack (0x12c)** then **gun+0x9c**
then **gun+0xa0** (the gun's two melee timers — NOT the alternate 0xa4).

## OPEN QUESTIONS (recommend live-gdb on the running original)
- **BlockPlayer's scalar slot.** Raw objdump reads `[base+0x24]` (the *refire* slot) for the
  BlockPlayer arg (`flds 0x30(esp)` @0x76d7d after `sub $0xc`), yet cont84 and the distinct
  per-branch values say it should be the block-stun scalar (`[base+0x20]`, the whiff-t). The two
  slots are adjacent and I could not fully disambiguate a ±4 esp offset by hand. The orchestrator
  should mirror cont84 (`BlockPlayer(m_pPlayer, local_10c * 0.75f)`) but this specific value is
  MED confidence — a one-line live trace of the 2nd BlockPlayer arg settles it.
- **PlaybackEvent iparam1 vs the existing `m_usCloseCombat // iparam1 = move id` comment.**
  Objdump has iparam1 = local_104 (1/2/3) and iparam2 = a3 (0x3..0xb); neither is the move-id.
  The client fire.sc reads these for the view-model — confirm which int the client anim uses.
