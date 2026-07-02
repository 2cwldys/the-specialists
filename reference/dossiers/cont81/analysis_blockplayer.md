# CBasePlayer::BlockPlayer — analysis

## 1. SIGNATURE

```
void CBasePlayer::BlockPlayer(float duration)
```
- gcc2 mangled: `BlockPlayer__11CBasePlayerf` — `T` @ file `0x85b58` (Ghidra `0x95b58`), size 24 bytes (signatures-gcc2.txt:154).
- Defined in `TS_player.cpp` (per the plate comment / funcfile map).
- Despite the name it is NOT the shield/deflect "block" — it is "block the player's MOVEMENT": a timed self-root/slow applied to the player. The stamina-bar mechanic is the separate `CBasePlayer::UpdateMeleeBar(float)` (`0x8650c`, field_0x774/gmsgKFuPower); the deflect window is player+0x7a8 (our `m_flKungFuBlock`). Three distinct members, three distinct mechanics.

## 2. BEHAVIOR

The function body (2 statements):

1. `this->field_0x4dc = duration` — **a raw 4-byte store of the float** (`mov %eax, 0x4dc(%edx)` @0x85b60). The decompile's `(int)a0` is an artifact: `members_auto.tsv` types the transient member `int`, but every other access is x87 (`flds/fcomps/fsubrs/fstps 0x4dc`), so the field is a **float** — the remaining block/stop time in seconds. It is transient (not in any save table; no recovered name). Suggested curated name `m_flBlockTime` — VERIFY, original name unknown.
2. `this->pev->iuser4 |= 1` — sets bit 0x1 = **TSSTUNT_STOP** (our `dlls/ts_stunt.h:20`). `iuser4` is a networked entvars field (offset 0x250), so the client's pm_shared sees it: `TSHandle` (pm) damps the move command (`cmd.forwardmove/sidemove/upmove *= 0.5`) every frame while the bit is set. Net effect: the player moves at half input authority for `duration` seconds.

No sounds, no events, no user messages, no other members. The *mechanic* is completed elsewhere:

### Timer lifecycle (the rest of the system)
- **Countdown — `CBasePlayer::PreThink` @0x808cc, block @0x80a2a–0x80a5a:** if `field_0x4dc != 0`: `field_0x4dc -= dec; if (field_0x4dc < 0.001f) field_0x4dc = 0.001f` (floors at 0.001, does not zero). The same loop counts down player+0x79c (the kung-fu whiff-lunge magnitude) just before it, and player+0x4ec (gravity/friction-restore timer) just after.
  - `dec` (stack local esp+0x1c) = `GiveSlowMul()` (virtual, CBasePlayer vtable +0x10 — the slow-motion multiplier) `* gpGlobals->frametime`; if `(m_iPowerups /*player+0x724*/ & 0x10)` it is additionally `* 1.5f` (const @GOT-0x6deec = 1.5) — i.e. with that powerup the stop wears off 50% faster. VERIFY: the non-powerup path's store into esp+0x1c happens earlier in PreThink (not re-derived here); the x1.5 branch is unambiguous.
- **Expiry — PreThink @0x80cda–0x80d21:** `if (field_0x4dc != 0 && (pev->iuser4 & 1) && field_0x4dc < 0.01f) { pev->iuser4 &= ~1; field_0x4dc = 0; }` (consts: 0.01 @GOT-0x6f3f0, 0.001 @GOT-0x6f3f4, 0.0 @GOT-0x6f408). So the floor value 0.001 trips the `< 0.01` test on the next frame and the STOP bit is cleared.
- **Resets:** `CBasePlayer::TSInit` (+0x2e7, 0x82d33) zeroes it on player init; `CTSStunt::Play` zeroes it at TWO points (+0x29a @0x8a2ae, +0x349 @0x8a35d) — starting a stunt cancels the melee self-stop (the stunt system then owns bit 1 itself: HandleWheel/HandleSlide/Landing/PostThink also set `iuser4 |= 1` for velocity damping, WITHOUT the 0x4dc timer).

### Numeric constants (whole system)
| value | where | meaning |
|---|---|---|
| 0.75f (@GOT-0x6df6c=0xF7EA4) | CCombatAttack call site | kung-fu: stop duration = refire x 0.75 |
| 0.9f (@GOT-0x6decc=0xF7F44) | KatanaFire inline | katana: stop duration = attack time x 0.9 |
| 1.5f (@GOT-0x6deec) | PreThink | powerup (m_iPowerups & 0x10) speeds the countdown x1.5 |
| 0.001f / 0.01f | PreThink | decrement floor / expiry threshold |
| 2.0f (@GOT-0x6df4c) | CCombatAttack tail | m_flTimeWeaponIdle = base + 2.0 |

## 3. CALLERS

Exactly ONE PLT call (`call 592c0 <BlockPlayer@plt>` @0x76d90) plus one inlined copy:

1. **`CTSGun::CCombatAttack` (kung-fu), shared tail @0x76d5b–0x76dff** (Ghidra mis-split this tail off as `FUN_00086c8c`; file addr 0x76c8c–0x76e2b is inside `CCombatAttack__6CTSGun` @0x76078, next sym `GetCloseCombatVectors` @0x76e2c — the fragment's `unaff_EBP` = the CTSGun `this`, `+0x80` = `m_pPlayer`). Sequence:
   - `CloseCombatAttack(m_pPlayer, vecs, ...)` performs the actual hit; on a miss with lunge > 0.001: `player+0x79c = lunge magnitude`, `player+0x7a0 = lunge value` (sign-flipped when attack-type param == 3 — the back-kick pushes backward), `player+0x7a4 (byte) = 0`.
   - **Gate:** `if (rootFlag /*per-move bool, stack base+0x7*/ && !(m_pPlayer->pev->iuser4 & 1)) BlockPlayer(m_pPlayer, refire * 0.75f);` — the float arg loads from stack slot base+0x24, **the SAME local** later used for `m_flNextPrimaryAttack(+0xa0)`/`m_flNextSecondaryAttack(+0x9c) = UTIL_WeaponTimeBase() + refire` (verified in disasm: `flds 0x30(%esp)` after `sub $0xc` = base+0x24; `fadds 0x24(%esp)` at base level). So the self-stop is 0.75x the move's refire, NOT the separate "block-stun 0.1–0.35" column our comments recorded — see OURS/RECONCILE.
   - The target is **`m_pPlayer` — the ATTACKER** (self-root while swinging), not the victim.
   - Then `pfnPlaybackEvent` (engfuncs +0x1ec, the fire event), idle = base + 2.0, `player+0x7a8 = 0` (closes the kung-fu deflect window), `player+0x4f8 = refire`.
   - VERIFY: which of the ~13 kung-fu moves set the `rootFlag` bool (the per-move dispatch is upstream in the huge function; not traced here).
2. **`CTSGun::KatanaFire` @0x73a9e–0x73adb (inline copy, not a call):** `if (localBool /*esp+0xb*/ && !(pev->iuser4 & 1)) { player+0x4dc = attackTime /*esp+0x1c*/ * 0.9f; pev->iuser4 |= 1; }`, immediately preceded by `player+0x7a4 (byte) = 2` (katana lunge mode). VERIFY: whether esp+0x1c is the katana refire (likely, mirrors kung-fu) and which katana paths set the bool.
3. **Related but distinct `iuser4 |= 1` sites (no timer, NOT BlockPlayer):** `KatanaFire+0x284` and `CCombatAttack+0x47b` do `iuser4 |= 4|1` (PRONE+STOP) on the duck/sweep attack paths; `CTSKnife::Spawn+0x186` sets bit 1 on the thrown-knife entity's own pev; `HandleWheel/HandleSlide/Landing/PostThink` set it for stunts. These are the stunt/pose uses of the same bit.
4. **NOT callers:** `KnifeFire` (no self-stop for the knife), `Victim*Hold` (they run the deflect/stamina system: `UpdateMeleeBar(33)` + fields 0x7b0/0x774/0x12c).

## 4. OURS

- **`CBasePlayer::BlockPlayer` — ABSENT.** No method, no `player+0x4dc` member, no PreThink countdown/expiry of it. `dlls/player.cpp:1959` `PreThink` has the TS spread-decay/health-regen/etc. additions but nothing decrements a block/stop timer, and nothing clears `TSSTUNT_STOP` on a timer.
- **The bit's consumer EXISTS:** `pm_shared/pm_shared.cpp:3178` (`TSHandle`) implements `TSPM_STOP` -> `cmd.forwardmove/sidemove/upmove *= 0.5f` — exactly the original pm behaviour. The stunt-side producers/clears of bit 1 exist too (`dlls/ts_stunt.cpp:96,163,326,498,586`).
- **Kung-fu call site:** `dlls/ts_gun.cpp:1000` `CTSGun::CCombatAttack` — dispatch/damage/refire are ported, but there is **no BlockPlayer call**; the per-move "block-stun" values exist only as comments (`ts_gun.cpp:1042–1114`, `/*block-stun 0.25*/` etc.). The header comment at `ts_gun.cpp:988` says "local_10c (0.1-0.35) is the VICTIM block-stun (BlockPlayer(player, *0.75))" — **two errors vs the disasm:** (a) the target is `m_pPlayer`, the attacker (self-stop), not the victim; (b) at the shared tail the arg is `refire * 0.75`, the same stack slot as the next-attack time, not a separate 0.1–0.35 local. (VERIFY whether some per-move path stores a distinct value into that slot before the tail; the tail itself is unambiguous.)
- **Katana call site:** ours is `CTSGun::MeleeAttack` (`dlls/ts_gun.cpp:909`, shared knife/katana) — no x0.9 self-stop, no `player+0x7a4 = 2` lunge-mode byte.
- **`CTSGun::RegenBlockBar` (`ts_gun.cpp:1230`) is NOT a fragment of BlockPlayer.** It is our invented stand-in for the regen half of `UpdateMeleeBar` (field_0x774 / gmsgKFuPower stamina bar) — a different member and subsystem. Same for `m_flKungFuBlock` (`player.h:418` = player+0x7a8, the deflect window) — keep, different field.
- **Stunt reset:** our `CTSStunt::Play` cannot zero the (nonexistent) timer; the original zeroes player+0x4dc at two points in Play.

## 5. RECONCILE

1. **Write fresh** `void CBasePlayer::BlockPlayer(float duration)` in `dlls/player.cpp` (declare in `player.h`; original TU is TS_player.cpp): set a new transient member (suggest `m_flBlockTime`, provenance comment `player+0x4dc`; VERIFY name — not recoverable) `= duration`; `pev->iuser4 |= TSSTUNT_STOP;`. Do NOT save/restore it (transient in the original).
2. **Write fresh the PreThink lifecycle** (in `CBasePlayer::PreThink`, near the existing TS timer decays): countdown `m_flBlockTime -= GiveSlowMul()*frametime` (x1.5 when the m_iPowerups&0x10 powerup is active — VERIFY which powerup bit 0x10 is; likely kung-fu), floor at 0.001f; then `if (m_flBlockTime != 0 && (pev->iuser4 & TSSTUNT_STOP) && m_flBlockTime < 0.01f) { pev->iuser4 &= ~TSSTUNT_STOP; m_flBlockTime = 0; }`. Note our slow-mo virtuals: use whatever maps to GiveSlowMul (the slow factor), keeping game-time semantics. This same decrement loop is where the whiff-lunge (player+0x79c) countdown belongs when that SLICE is ported — implement them together if convenient.
3. **Wire the kung-fu call:** in `CTSGun::CCombatAttack`, at the common post-attack tail: `if (rootFlag && !(m_pPlayer->pev->iuser4 & TSSTUNT_STOP)) m_pPlayer->BlockPlayer(refire * 0.75f);` — VERIFY per-move which moves set `rootFlag` (needs the per-move dispatch re-read; until verified, flag rather than guess — do not just apply it to all moves).
4. **Wire the katana:** in the katana branch of `MeleeAttack` (or when KatanaFire is split out to match the original file/function structure — audit.py will want `KatanaFire` as its own method anyway): the inline form, duration x0.9f, plus `player+0x7a4 = 2` once the lunge fields (0x79c/0x7a0/0x7a4) exist. Knife gets nothing.
5. **Stunt cancel:** zero `m_flBlockTime` in `CTSStunt::Play` at the reset points (where ours already strips TSSTUNT_STOP from iuser4).
6. **Fix the drifted comment** at `ts_gun.cpp:988` (attacker self-stop, x0.75 of refire — not "VICTIM block-stun") and the per-move `/*block-stun N*/` comments if the single-slot finding holds after the per-move VERIFY.
7. **Do NOT rename `RegenBlockBar` to BlockPlayer** — it belongs to the `UpdateMeleeBar` reconciliation (separate task; the original has no RegenBlockBar, so audit.py flags it as INVENTED against UpdateMeleeBar, not against BlockPlayer).
8. **Protocol implications:** none on the wire format — no user messages or events are emitted by BlockPlayer. But `pev->iuser4` bit 1 is part of the networked entity state consumed by pm_shared on BOTH sides: the original client damps movement when the server sets bit 1. Our server currently never sets it from melee, so against an original client the half-speed swing feel is missing (behavioural drop-in gap); adding it restores parity and our pm_shared already predicts it identically.
