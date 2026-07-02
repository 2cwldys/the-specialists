# CBasePlayer::UpdateMeleeBar — analysis (ts_i386.so @ Ghidra 0x9650c, nm 0x8650c; PLT thunk @0x68b20)

## 1. SIGNATURE

From `reference/signatures-gcc2.txt` (mangled `UpdateMeleeBar__11CBasePlayerf`):

```cpp
void CBasePlayer::UpdateMeleeBar(float flAmount);   // flAmount = stamina COST to subtract
```

Ghidra decompile agrees: `void CBasePlayer::UpdateMeleeBar(CBasePlayer *this, float a0)`, 192 bytes,
source file `TS_player.cpp` (filemap plate comment).

## 2. BEHAVIOR

This is the single choke-point for the melee/kung-fu block bar ("KFuPower", 0..100 stamina): it applies a
drain, handles depletion (global attack lockout), and networks the new value to the owning client.

Step by step:

1. `fVar1 = m_flMeleeBar - flAmount` where **`m_flMeleeBar` = player field_0x774 (float)** — the melee
   stamina bar, 0..100. (members_auto.tsv has `CBasePlayer 0x774` typed int by Ghidra's integer wobble;
   every use is float — `GetMeleeMultiplier` @0x965d0 reads it as float. NOT in members.tsv → transient,
   not saved.)
2. **Depletion branch** — if `fVar1 < 0.01f` (fc_0_009999999776482582):
   - `m_flMeleeBar = 0`
   - `field_0x12c = 1.5f` (0x3fc00000). **field_0x12c = the stock SDK `CBasePlayer::m_flNextAttack`**
     (weapon-time-relative; compared vs `UTIL_WeaponTimeBase()` in CCombatBlock and set
     `WeaponTimeBase()+const` in the kick fragment @0x9c70c). I.e. draining the bar to (near) empty
     imposes a **1.5 s global attack lockout** — this IS the "exhausted" behavior; there is no separate
     cooldown member.
3. Else (`fVar1 >= 0.01`): `m_flMeleeBar = fVar1`. (The decompiled inner `else fVar1 < -0.001f → 0` arm is
   dead code under the outer condition — compiler artifact of a `<0.01 / <-0.001` cascade. Net effect:
   subtract; a result below the 0.01 floor becomes 0 + lockout.)
   - **No upper clamp at 100 inside this function.** If a negative flAmount (regen) is ever passed, the
     cap must live at the call site. VERIFY (see CALLERS — no regen call site captured in the refs dump).
4. **Networks the bar unconditionally, every call**:
   ```
   MESSAGE_BEGIN(MSG_ONE /*1*/, gmsgKFuPower, NULL /*0*/, this->pev->pContainingEntity);
   WRITE_CHAR((int)ROUND(m_flMeleeBar));   // 0..100 fits a signed char
   MESSAGE_END();
   ```
   `PTR_gmsgKFuPower_00177488` confirms the user message name **"KFuPower", 1 byte (CHAR)** — owner-only.

Constants: 0.01f (empty threshold), -0.001f (dead arm), 1.5f (m_flNextAttack lockout on depletion).
No sounds, no events, no other members touched in this function itself.

### The surrounding melee-bar member cluster (player +0x774, +0x79c..+0x7b0) — resolved from callers

| offset | type | meaning (RE) |
|---|---|---|
| 0x774 | float | m_flMeleeBar — block stamina 0..100 (this function; GetMeleeMultiplier scales melee dmg 0.5..1.0 from it) |
| 0x79c | float | whiff-lunge window/decay — set 0.3f (0x3e99999a) after backkick (frag @0x9cdb8), set to swing power on a MISSED melee swing (frag @0x86c8c); checked `>0` by VictimKungfuHold for the counter |
| 0x7a0 | float | whiff-lunge force/damage — swing damage (doubled if `KungFu(player)` powerup), sign-flipped when dir==3 (backward → backkick lunges backward, frag @0x86c8c `param_7 ^ 0x80000000`); zeroed on any successful block |
| 0x7a4 | char | whiff-lunge consumed/applied flag — zeroed when arming; VictimKungfuHold requires `== 0` |
| 0x7a8 | float | KUNGFU (bare-hands) block window — CCombatBlock sets **0.5** when no weapon; zeroed when the player attacks (frags @0x9c70c, @0x9cdb8, @0x86c8c). Checked `>0` by VictimKungfuHold |
| 0x7ac | float | KNIFE block window — CCombatBlock sets 0.5 when active iFireType==3 |
| 0x7b0 | float | KATANA block window — CCombatBlock sets 0.5 when active iFireType==10; VictimKnifeHold falls through to it (a katana block also deflects a knife) |
| 0x12c | float | stock m_flNextAttack (global attack gate) |
| 0x4f8 | float | last melee refire/rate stash (frag @0x86c8c) — VERIFY exact meaning |

VERIFY: the block windows 0x7a8/0x7ac/0x7b0 are stored as the plain VALUE 0.5, not `time+0.5` — something
must count them down/expire them per-frame (PreThink/ItemPostFrame). The decay site was not in the provided
refs; do NOT assume our `gpGlobals->time + 0.5f` absolute-time form is equivalent without finding it.

## 3. CALLERS (from melee_refs.txt — 8 referencing functions; all captured calls pass a POSITIVE cost)

- **`CTSGun::CCombatBlock` @0x85ebc** (melee RELOAD = raise block). Gates: `player->m_flNextAttack(0x12c)`,
  gun `0x9c/0xa0/0xa4` (next primary/secondary/tertiary) all `<= WeaponTimeBase()`,
  `pev->iuser4 & 0x70c30 == 0` (stunt-lock bits), **`m_flMeleeBar >= 0.01`** (can't block on empty). Sets the
  matching block window (0x7a8=0.5 bare / 0x7b0=0.5 katana / 0x7ac=0.5 knife), fires
  **PLAYBACK_EVENT_FULL(flags=1, owner, event handle gun+0x96c, delay 0, g_vecZero×2,
  fparam1=(float)m_weaponType, fparam2=0, iparam1=0, iparam2 = 1 (bare) / 0x12 (armed), bparams 0,0)** —
  the block anim/sound rides an EVENT, not a server EMIT_SOUND — then
  `CTSAnimation::Animate(player, 0x26 /*TSA_CC_BLOCK*/)`, **`UpdateMeleeBar(33.0)`**, next
  primary/secondary = +0.5, idle = +3.0. So raising a block COSTS 33.
- **`VictimKatanaHold` / `VictimKnifeHold` @0x76c8c/@0x76de4 (TS_gamerules.cpp)** — called when a melee hit
  lands on a target holding the corresponding block window; require target `IsPlayer()`, the window `>0`, and
  **the target's bar > 0.1**. On deflect: `UTIL_Sparks` at the two players' midpoint +20z,
  `EMIT_SOUND_DYN(attacker edict, CHAN_BODY=4, "weapons/knife/knife_hit.wav", 1.0, attn 0.8, 0, pitch 100, 0.5)`
  (TS's EMIT_SOUND_DYN has a trailing extra float — VERIFY its meaning), attacker `0x7a0=0`,
  attacker `m_flNextAttack = 0.75` (katana) / `0.5` (knife), victim `m_flNextAttack = 0.35` / `0.25`, then
  **`UpdateMeleeBar(a0, 33.0)` — drains the ATTACKER** (being blocked costs the attacker 33; the blocker
  already paid 33 at CCombatBlock time). Returns 1 (hit deflected).
- **`VictimKungfuHold` @0x76f50** — same shape for bare-hands, sound `"player/kungfuhit.wav"`, plus the
  COUNTER twist: if the target has no window but the ATTACKER has 0x7a8>0 while the target is in whiff state
  (0x79c>0, 0x7a4==0), the roles swap. Requires bar (of the checked side) > 0.1; zeroes both 0x7a0s, attacker
  next-attack 0.5, blocker 0.25, `UpdateMeleeBar(33.0)`. VERIFY which side's bar drains after the swap
  (decompile drains `a0` post-reassignment).
- **Kick/backkick fragment @0x9c70c** (TS_player.cpp, just below GoWheel @0x9c9bc; Ghidra-truncated) — the
  close-combat kick: damage scaled from `pev->velocity` magnitude, **doubled if `KungFu(player)`**,
  `0x7a8=0`, `CloseCombatAttack(player, src, end, dir, 3, dmg, -1, …)`; ON HIT: random body anim
  (0xe/0xd/0xa via RANDOM_LONG(0,2)), `m_flNextAttack = WeaponTimeBase()+const` if expired, kungfu-hit sound
  at pitch `0x5f + RANDOM_LONG(0,0x1f)` (95..126), **`UpdateMeleeBar(dmg * const)`** — melee attacks drain
  the bar proportionally to power.
- **Backkick tail fragment @0x9cdb8** — when direction==3 (BACKWARD, the GoWheel backkick combo):
  **`UpdateMeleeBar(constA*constB)`** (the ts_stunt.cpp note records this product as 22.5), then arms the
  whiff-lunge: `0x79c=0.3f`, `0x7a0=dmg` (doubled if KungFu), `0x7a4=0`, `0x7a8=0`. **This is the call chain
  that BLOCKS the GoWheel backward-backkick combo in our build** (we never drain/arm any of it).
- **Melee swing fragment @0x86c8c** (TS_gun.cpp, near ColdCockAttack @0x86610) — on a MISSED swing with power
  above a threshold, arms the whiff-lunge (0x79c=power, 0x7a0=±force sign-flipped for param_5==3, 0x7a4=0);
  conditionally `BlockPlayer(victim, rate*const)`; plays the fire event; sets gun 0x9c/0xa0=+rate, 0xa8=+idle,
  `0x7a8=0`, `0x4f8=rate`. (Uses the cluster; calls BlockPlayer/CloseCombatAttack, not UpdateMeleeBar itself.)
- **REGEN — NOT FOUND in the provided refs.** Our ts_gun.cpp:1229 comment claims "the original regens
  field_0x774 via UpdateMeleeBar with a negative amount", but no negative-amount caller appears in
  melee_refs.txt, and no other WRITE to +0x774 exists in the recovered dumps (only UpdateMeleeBar writes it;
  CTSGun only READS it — `ROUND(player+0x774)` in UpdateClientData; GetMeleeMultiplier reads it).
  **VERIFY the regen site + rate** (candidates: a PreThink/ItemPostFrame caller of the PLT thunk @0x68b20
  that the refs dump didn't decompile, passing `-rate*frametime`; or client-side visual regen with a server
  regen elsewhere). Do not ship a guessed rate as final — our current 25/sec is user-tuned, marked as such.
- Related (recovered/server/CTSGun.c ~line 2338): **`CTSGun::UpdateClientData` sends
  `gmsgWeaponInfo(weaponType, ROUND(player->0x774), SHORT 0, 0, 0)` when switching TO a melee weapon** —
  for melee weapons the WeaponInfo "clip" byte carries the melee bar, seeding the HUD; KFuPower then carries
  the live updates.

Dispatch context: CCombatBlock is reached from CTSGun::Reload/ItemPostFrame when a melee weapon is active
(reload = block); the Victim*Hold helpers from CloseCombatAttack/ColdCockAttack hit resolution
(TS_gamerules.cpp); the kick fragments from the close-combat attack path invoked by
PrimaryAttack/GoWheel(dir==3).

## 4. OURS

**`CBasePlayer::UpdateMeleeBar` itself: absent.** The behavior exists piecemeal, on the WRONG class, with
invented names/mechanisms:

- Bar storage: `CTSGun::m_blockBar` (dlls/ts_gun.h:148) — original keeps it on **CBasePlayer** (+0x774).
  dlls/ts_player.cpp:518 (`GetMeleeMultiplier`) even documents this and reaches into the gun for it
  (inverted dependency vs the original, where the gun reaches into the player).
- Regen: `CTSGun::RegenBlockBar()` (dlls/ts_gun.cpp:1230–1240) — invented name, invented mechanism
  (time-delta ×25/sec cap-100, plus a `m_blockBarTime` bookkeeping member the original does not have).
- Drain + exhaustion: inline in `CTSGun::CCombatBlock()` (dlls/ts_gun.cpp:1247–1280): `kCost=33` matches,
  but exhaustion is an invented `m_blockCooldown = time+1.5` + `breathe2.wav` puff + 0.5 lockout
  (ts_gun.cpp:1256–1264). Original: depletion simply sets **player `m_flNextAttack = 1.5`** inside
  UpdateMeleeBar; no cooldown member, no breathe2 in the original CCombatBlock decompile (VERIFY where
  breathe2.wav actually plays before deleting it). Ours also plays `player/block.wav` + `SendWeaponAnim(4)`
  server-side; the original fires the gun's **event** (handle gun+0x96c, iparam2=1/0x12) instead.
- Networking: dlls/ts_gun.cpp:1752–1767 sends KFuPower(CHAR) from `UpdateClientData` with change-detect
  (`m_iClientBlockBar`, ts_gun.h:151). Original sends from INSIDE UpdateMeleeBar, every call, no
  change-detect; wire format identical (1 CHAR, MSG_ONE). Ours also writes `m_iClip` into the WeaponInfo
  clip byte where the original writes ROUND(bar) for melee weapons (ts_gun.cpp:1783).
- Attack-drain (kick @0x9c70c `UpdateMeleeBar(dmg*const)`): **absent** — our `CCombatAttack`
  (dlls/ts_gun.cpp:1000+) never drains the bar on attack.
- Blocked-attacker drain (Victim*Hold `UpdateMeleeBar(33)` on the attacker): **absent** — our simplified
  deflect (dlls/ts_gun.cpp:1180–1190) stuns the attacker 0.5s via weapon timers and pays nothing; only the
  kungfu window exists (`m_flKungFuBlock`, dlls/player.h:418 = original 0x7a8 but stored as absolute time),
  and the knife/katana windows 0x7ac/0x7b0 don't exist at all — our knife/katana can't block; the original's
  sparks / knife_hit.wav / asymmetric next-attack times (0.75/0.5 attacker vs 0.35/0.25 victim) are missing.
- Backkick combo (frag @0x9cdb8): **absent**, explicitly flagged at dlls/ts_stunt.cpp:454–455
  ("UpdateMeleeBar is not implemented yet") and dlls/ts_gun.cpp:994 (whiff-lunge 0x79c documented, not
  ported). None of 0x79c/0x7a0/0x7a4 exist as members.
- Client side: cl_dll/ts_ammo.cpp:160 `MsgFunc_KFuPower` reads the CHAR (compatible); draw at
  ts_ammo.cpp:283 (melee-only bottom bar). No client-side regen smoothing in ours — VERIFY whether the
  original client.dll regens the bar locally between KFuPower messages (matters for regen design, step 6).

## 5. RECONCILE

1. **Write `CBasePlayer::UpdateMeleeBar(float)` fresh in dlls/ts_player.cpp** (original file: TS_player.cpp)
   — exact transcription: subtract; `<0.01f` → bar=0 + `m_flNextAttack=1.5f`; else keep; then send
   `gmsgKFuPower` (MSG_ONE, WRITE_CHAR(round)) unconditionally. Move the `gmsgKFuPower` registration to
   player scope. Name `KFuPower`, size 1 — already byte-faithful.
2. **Move the bar onto CBasePlayer** as `m_flMeleeBar` (provenance +0x774, transient/not saved), init at
   spawn (VERIFY the spawn-init value/site — likely 100 in PlayerSpawn/Spawn). Fix `GetMeleeMultiplier`
   (dlls/ts_player.cpp:519) to read the player member directly — removes the GetPlayerTSGun reach-around
   and matches @0x965d0 exactly.
3. **Delete the inventions** (audit.py INVENTED): `CTSGun::RegenBlockBar`, `m_blockBar`, `m_blockBarTime`,
   `m_blockCooldown`, `m_iClientBlockBar`, the UpdateClientData KFuPower send, and the breathe2/cooldown
   branch in CCombatBlock. CCombatBlock keeps cost 33 + timers +0.5/+3.0, gains the missing gates
   (`m_flNextAttack`, tertiary timer 0xa4, `iuser4 & 0x70c30`, `bar >= 0.01`), sets the per-weapon block
   window (0x7a8/0x7ac/0x7b0 = 0.5 by iFireType none/3/10), calls `UpdateMeleeBar(33.0f)`. Replace the
   server-side block.wav/SendWeaponAnim with the original's PLAYBACK_EVENT (fparam1=weaponType,
   iparam2=1 bare / 0x12 armed) — **protocol implication: a wire-visible event; VERIFY WHICH .sc rides
   gun+0x96c (precache table) before switching — the original client must render it.**
4. **Add the member cluster to CBasePlayer**: 0x79c whiff-lunge window, 0x7a0 lunge force, 0x7a4 flag,
   0x7a8/0x7ac/0x7b0 kungfu/knife/katana block windows. Convert `m_flKungFuBlock` (absolute-time invention)
   to the original 0x7a8 value semantics — **VERIFY the countdown/expiry site first** (decompile
   PreThink/ItemPostFrame or live-watch player+0x7a8; don't guess the decay).
5. **Wire the missing drains**: attacker drain 33 + asymmetric next-attack (0.75/0.5 vs 0.35/0.25) + sparks
   at midpoint+20z + knife_hit/kungfuhit on CHAN_BODY attn 0.8 in the Victim*Hold deflect path; attack drain
   `dmg*const` in the CCombatAttack hit path; backkick `UpdateMeleeBar(~22.5)` + whiff-lunge arm
   (0x79c=0.3, 0x7a0=±dmg doubled-if-KungFu, 0x7a4=0, 0x7a8=0) in GoWheel dir==3 — unblocks the
   ts_stunt.cpp:454 TODO. VERIFY the exact rodata constants in the drain products (Ghidra @0x9c70c
   EBX-relative -0x6deec / -0x6de8c; file offset = Ghidra addr − 0x10000, many are doubles).
6. **Regen: VERIFY before rewriting.** Find the real regen call site/rate (sweep the thunk @0x68b20's
   callers, or live-watch player+0x774 while idle in the original). Our 25/sec-in-UpdateClientData is
   user-tuned, not RE. Note the original's per-call KFuPower send means a per-frame regen through
   UpdateMeleeBar would spam messages — the actual design (server regen locus + possible client smoothing)
   must be confirmed, not assumed.
7. **WeaponInfo melee seed**: when switching to a melee weapon the original writes ROUND(bar) where ours
   writes m_iClip (dlls/ts_gun.cpp:1783 vs recovered/server/CTSGun.c ~2338) — same wire size, different
   value; fix for drop-in HUD correctness against the original client.
