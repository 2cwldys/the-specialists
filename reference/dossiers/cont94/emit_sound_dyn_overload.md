# cont94 — TS 8-arg `EMIT_SOUND_DYN` overload: slow-motion pitch weight (RE + fix spec)

RE of `EMIT_SOUND_DYN__FP7edict_siPCcffiif` @ nm `0xd0a8c` (Ghidra `0xe0a8c`,
`sound.cpp`) in `reference/ts_i386.so`, the full call-site census (all 156
sites, arg-recovered), and the drop-in fix spec for our tree. Every claim is
instruction-verified (objdump; GOT base `0x165E10`; .rodata floats read from
the file at the link vaddr).

---

## 1. The overload — exact semantics (decompile + disasm verified)

```c
void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample,
                    float volume, float attenuation, int flags, int pitch,
                    float w /* slow-mo pitch WEIGHT, arg8 */)
{
    float scale = w + (1.0f - w);                 // == 1.0 when fuser1 unset
    if (entity->v.fuser1 != 0.0f)                 // strict FP compare vs 0.0
        scale = w * entity->v.fuser1 + (1.0f - w);
    pitch = (int)(scale * pitch);                 // TRUNCATING cast (fldcw RC=chop), not round
    /* then IDENTICAL to the stock 7-arg body: '!' sentence lookup via
       SENTENCEG_Lookup (ALERT at_aiconsole(2) "Unable to find %s in sentences.txt\n"
       on failure), then (*g_engfuncs.pfnEmitSound)(entity, channel, sample,
       volume, attenuation, flags, scaledPitch)  — engfuncs slot 0x74. */
}
```

**effective pitch = pitch × ((1 − w) + w · pev->fuser1)**, `pev` = the
**EMITTING edict's** entvars (disasm reads `0x2d4(%ebp)` off the `edict_t*`
arg1; edict header is 0x80, so +0x2d4 = entvars+0x254 = **fuser1** — the TS
per-entity timescale, 1.0 = normal, <1 = slowed).

- `w` is the **weight** (how much the pitch follows the timescale), NOT the
  floor. The **floor is `1 − w`** (value at fuser1→0⁺).
  - w=0.5 → floor 0.5 (combat/body sounds halve at full slow)
  - w=0.9 → floor 0.1 (breakable-bust sounds deepen hardest)
  - w=0.15 → floor 0.85 (pain VOICE barely deepens)
  - w=0 → scale ≡ 1.0 (never used; no site passes 0)
- **`fuser1 == 0.0` (never set) → scale forced to exactly 1.0** (the
  `fcomps`/`test $0x40,%ah`/`jne` equal-branch skips the multiply, leaving
  `w + (1−w)`). So an entity that never initialized fuser1 sounds normal —
  NOT floored. (Matches our tree's "fuser1==0 means unset, neutral=1.0"
  convention, player.cpp:2163.)
- At fuser1 == 1.0 (normal time) scale = `(1−w) + w` — algebraically 1.0 for
  every w → **zero audible change outside slow-motion**.
- Truncation: gcc's fldcw round-toward-zero idiom around `fistpl` → plain C
  `(int)` cast. `(int)(scale * pitch)` with the multiply done `scale`(float) ×
  `pitch`(int) via `fimull`.
- Constants: 1.0f @ file `0xF6A14`, 0.0f @ `0xF6A08` (fcomps operand);
  key disasm: `0xd0aa7` flds arg8 → `0xd0aab` flds 1.0 → `fsub` (1−w) →
  `0xd0ab3` flds 0.0, `fcomps 0x2d4(%ebp)` → eq-branch `0xd0ad4 faddp` /
  ne-branch `0xd0ac8 fmuls 0x2d4(%ebp); faddp`.

### ⚠ Correction to `traceattack_followups.md` D14
D14 wrote *"effective pitch = pitch * (f + (1−f) * fuser1)"* — **the roles of
f and (1−f) are swapped**. The binary multiplies the PASSED float by fuser1.
Consequence: pain voice (0.15) deepens the LEAST (floor 0.85), not the most.
(D14's 0.5 case is symmetric so its TraceAttack body-hit conclusion stands.)
Also D10's const list has a typo: the pain-voice weight const is @`0xF7F10`
(= 0.15f), not `0xF6F10`; attn 1.25f @`0xF92CC`, vol 1.0f @`0xF6A14`.

### fuser1 writers (context — who feeds the formula)
Exhaustive `0x254(%reg)` store scan: Spawn-time `= 1.0f` inits in
`CBreakable::Spawn` @0x65118, `CTSShrapnel::Spawn` @0x6c186,
`CM61Grenade::Spawn` @0x6ca69, `CTSKnife::Spawn` @0x6dca4,
`CBasePlayer::TSInit` @0x82a85, `CTSFlag::CreateFlag` @0x8d964,
`CTSWorldGun::CreateWeapon` @0x908f8; runtime writers
`CBasePlayer::PostThink` @0x81c3b (the InterpolateSlowMotion ease),
`CBaseEntity::GoSlow(float,char)` @0x9c5ea, and **`CBreakable::Die` @0x653f1
copies `BreakableLastpevAttacker->fuser1` into the breakable** right before
playing the bust sound (see §4.2).

---

## 2. No sibling overloads; the client side

- **`EMIT_SOUND` (non-DYN): no TS function exists.** nm shows only
  `EMIT_SOUND_DYN__FP7edict_siPCcffiif` + `EMIT_SOUND_SUIT__FP7edict_sPCc`.
  The whole `.so` has exactly TWO `call *0x74(%reg)` sites: one inside the
  overload itself (@0xd0ba1), one a **vtable** call in `PlayerKilled`
  (@0x79c09, `mov (%edx),%eax` — not engfuncs). **Every server sound emit in
  the original routes through the 8-arg overload.** The `EMIT_SOUND` /
  `STOP_SOUND` inlines therefore forward to it (their call sites appear in the
  §3 census, all with 0.5), as do `EMIT_SOUND_SUIT` (@0xd0bb0, internal call
  @0xd0c34 → 0.5) and all four `SENTENCEG_*` emitters (0.5).
- **Client (`client.dll`): no equivalent.** Server-emitted sounds reach every
  client (including a stock original client) with the FINAL scaled pitch in
  the normal engine sound message — **server-side scaling IS the swap
  contract**. Client-local sounds (event playback: gunfire, melee whooshes)
  use a *different*, already-RE'd pitch model driven by the local smoothed
  slow factor — implemented in `cl_dll/ev_ts.cpp` (fire: `100 + 300*g_flClientSlow`
  clamp 255; melee: `25 + 75*timescale + rand(-5,5)`; see
  SLOWMO_PRESENTATION_RE). Nothing further needed client-side beyond the
  link-stub signature (§4.1).

---

## 3. Call-site census — all 156 sites, recovered 8th arg

Method: `objdump -d`, every `call …EMIT_SOUND_DYN__FP7edict_siPCcffiif@plt`
(the `call +0xc` PIC get-PC thunk excluded), containing function via bisect
over sorted `nm`, arg8 = the 8th outgoing push found by a **backward
stack-machine walk** (push = 4 bytes of args; `add $N,%esp` after an inner
`RANDOM_LONG`/`RANDOM_FLOAT`/`SENTENCEG` call = N bytes to skip; forks over
`jmp`-convergence so branch-selected sample variants are handled). Immediates
(`push $0x3f000000`) and .rodata pushes (`push -0x6ec2c(%ebx)` → GOT−off =
`0xF71E4` = 0.5f) both decoded. Six sites whose volume is an
`RANDOM_FLOAT(0.92,1.0)`-`fstps` slot were hand-verified (all `push
$0x3f000000` = 0.5f first): 0x7168f, 0x7c992, 0x7d052, 0x7d1b5, 0x903be,
0x95fcd.

**Distribution: 149 × 0.5 · 6 × 0.9 (all `CBreakable::Die`) · 1 × 0.15
(TraceAttack pain voice). No other value anywhere.**

Stock-SDK functions the TS devs never touched (CXenTree, CFuncPlat,
PlayLockSounds, CItemSoda, …) all "pass" 0.5 → **0.5 was a DEFAULT ARGUMENT
on the shared declaration** (gcc materializes the default at each call site);
only `CBreakable::Die` (0.9) and the TraceAttack pain voice (0.15) pass it
explicitly in source. That is the reconstruction shape.

### Full table (addr · function · sample(s)¹ · arg8)

¹ representative — converged branches select among variants (e.g. Die's one
call covers bustglass1+2; TraceAttack's covers bodyhit1–4 / pain2–4).
`0.5*` = one of the six hand-verified sites. `VAR`/`array:` = runtime sample.

| addr | function | sample | w |
|---|---|---|---|
| 0x65494 | CBreakable::Die | debris/bustglass1/2.wav | **0.9** |
| 0x65508 | CBreakable::Die | debris/bustcrate1/2.wav | **0.9** |
| 0x65588 | CBreakable::Die | debris/bustmetal1/2.wav | **0.9** |
| 0x65608 | CBreakable::Die | debris/bustflesh1/2.wav | **0.9** |
| 0x65688 | CBreakable::Die | debris/bustconcrete1/2.wav | **0.9** |
| 0x656c4 | CBreakable::Die | debris/bustceiling.wav | **0.9** |
| 0x66d9c | VictimKatanaHold | weapons/knife/knife_hit.wav | 0.5 |
| 0x66f07 | VictimKnifeHold | weapons/knife/knife_hit.wav | 0.5 |
| 0x670f6 | VictimKungfuHold | player/kungfuhit.wav | 0.5 |
| 0x6bf68 | CTSGrenade::BounceTouch | weapons/grenbounce*.wav (vol 0.25, pitch 100) | 0.5 |
| 0x6c62e | CTSShrapnel::BounceTouch | knife_hit.wav (pitch 100+RAND(20,30)) | 0.5 |
| 0x6e4f1 | CTSKnife::BounceTouch | knife_hit.wav (pitch fp-computed) | 0.5 |
| 0x6e6a2 | CTSKnife::BounceTouch | knife_hit.wav (pitch fp-computed) | 0.5 |
| 0x6e9a4 | CTSKnife::BounceTouch | knife_hit.wav (world tail: vol 1.0, attn 0.8, pitch 100) | 0.5 |
| 0x7168f | CTSGun::SwitchItems | weapons/switch.wav (vol RF(0.92,1), pitch 98+RL(0,3)) | 0.5* |
| 0x72091 | CTSGun::ColdCock | player/kevlarhit.wav | 0.5 |
| 0x73a5e | CTSGun::KatanaFire | weapons/katana/hit-body.wav | 0.5 |
| 0x73fb3 | CTSGun::KatanaThrow | weapons/katana/miss.wav | 0.5 |
| 0x743fc | CTSGun::KnifeFire | weapons/knife/knife_hitbody.wav | 0.5 |
| 0x748ec | CTSGun::KnifeFire | weapons/knife/knife_miss.wav | 0.5 |
| 0x7c992 | CTSBomb::WaitThink | misc/beep2.wav (vol RF(0.92,1), pitch 98+RL(0,3)) | 0.5* |
| 0x7d052 | CTSHack::Use | misc/hacking.wav (") | 0.5* |
| 0x7d1b5 | CTSHack::Use | misc/hacked.wav (") | 0.5* |
| 0x8079a | CBasePlayer::TraceAttack | headshot/kevlarhit/bodyhit1-4 (CHAN_BODY, 1.0, 1.25, 95+RL(0,31)) | 0.5 |
| 0x80828 | CBasePlayer::TraceAttack | player/pain2-4.wav (CHAN_VOICE, 1.0, 1.25, pitch 100) | **0.15** |
| 0x817ba | CBasePlayer::PostThink | common/bodysplat.wav | 0.5 |
| 0x819cc | CBasePlayer::PostThink | player/superjump-land.wav | 0.5 |
| 0x832ce | CBasePlayer::PickUpWeapon | items/gunpickup2.wav | 0.5 |
| 0x838cc | CBasePlayer::TakeFlag | items/gunpickup2.wav | 0.5 |
| 0x83d7e | CBasePlayer::DropArmor | weapons/weapondrop.wav | 0.5 |
| 0x83e39 | CBasePlayer::TakeArmor | weapons/items.wav | 0.5 |
| 0x842fc | CBasePlayer::BuyAmmo | items/9mmclip1.wav | 0.5 |
| 0x843fa | CBasePlayer::BuyAmmo | items/9mmclip1.wav | 0.5 |
| 0x84531 | CBasePlayer::AddItem | weapons/items.wav | 0.5 |
| 0x851e5 | CBasePlayer::DropWeapon | weapons/weapondrop.wav | 0.5 |
| 0x863e8 | CBasePlayer::CloseCombatAttack | player/kungfuhit.wav | 0.5 |
| 0x869bd | CBasePlayer::ColdCockAttack | player/kungfuhit.wav | 0.5 |
| 0x87de1 | PlayerAddPowerUp | items/gunpickup2.wav | 0.5 |
| 0x8a16f | CTSStunt::Play | player/airwheel.wav | 0.5 |
| 0x8a771 | CTSStunt::GoBulletTime | player/land.wav | 0.5 |
| 0x8a98c | CTSStunt::GoWallJump | player/airwheel.wav | 0.5 |
| 0x8ac03 | CTSStunt::GoWallClimb | player/airwheel.wav | 0.5 |
| 0x8b4ef | CTSStunt::GoDive | player/dive.wav | 0.5 |
| 0x8b634 | CTSStunt::HandleWallJump | player/land.wav | 0.5 |
| 0x8b77c | CTSStunt::HandleWallClimb | player/land.wav | 0.5 |
| 0x8b9e6 | CTSStunt::HandleWheel | player/land.wav | 0.5 |
| 0x8bd8a | CTSStunt::Landing | player/land.wav | 0.5 |
| 0x8c512 | CTSStunt::CheckForBreakables | weapons/knife/knife_hitbody.wav | 0.5 |
| 0x8c985 | CTSStunt::CloseAttack | player/closecombat.wav | 0.5 |
| 0x8cddf | CTSStunt::GoWheel | player/airwheel.wav | 0.5 |
| 0x8cfb1 | CTSStunt::GoWheel | player/airwheel.wav | 0.5 |
| 0x8d32c | CTSStunt::GoRoll | player/land.wav | 0.5 |
| 0x8de0e | CTSFlag::BounceTouch | items/weapondrop1.wav | 0.5 |
| 0x8dfb8 | CTSFlag::Fall | items/weapondrop1.wav | 0.5 |
| 0x8e466 | CTSFlag::PlayerTakeFlag | items/gunpickup2.wav | 0.5 |
| 0x903be | CBasePlayerWeapon::DefaultDeploy | weapons/draw.wav (CHAN_WEAPON, vol RF(0.92,1), pitch 98+RL(0,3)) | 0.5* |
| 0x90d6e | CTSWorldGun::BounceTouch | items/weapondrop1.wav | 0.5 |
| 0x90e2c | CTSWorldGun::Fall | items/weapondrop1.wav | 0.5 |
| 0x9186c | CTSAmmoPack::WaitTouch | items/9mmclip1.wav | 0.5 |
| 0x921c1 | CTSGroundWeapon::Fall | items/weapondrop1.wav | 0.5 |
| 0x92204 | CTSGroundWeapon::Fall | items/weapondrop1.wav | 0.5 |
| 0x9264f | CTSGroundWeapon::GetWeapon | items/9mmclip1.wav | 0.5 |
| 0x92ddd | CTSGroundAmmo::Fall | items/weapondrop1.wav | 0.5 |
| 0x92e20 | CTSGroundAmmo::Fall | items/weapondrop1.wav | 0.5 |
| 0x930d5 | CTSGroundAmmo::WaitTouch | items/9mmclip1.wav | 0.5 |
| 0x95fcd | CFuncRotating::RampPitchVol | VAR (pev->noise) | 0.5* |
| 0x96214/0x9640c/0x96525/0x965f6 | CFuncRotating SpinUp/SpinDown/RotatingUse | VAR | 0.5 |
| 0x98473/0x98b91/0x98d08/0x98d96 | CBaseButton TakeDamage/Use/Touch/Activate | VAR | 0.5 |
| 0x98a84 | DoSpark | buttons/spark*.wav | 0.5 |
| 0x99845 | CMomentaryRotButton::PlaySound | VAR | 0.5 |
| 0xa1522 | CBaseMonster::GibMonster | common/bodysplat.wav | 0.5 |
| 0xa1be1 | CBaseMonster::Killed | common/null.wav | 0.5 |
| 0xa5025/0xa5129 | PlayLockSounds | VAR | 0.5 |
| 0xa59dc–0xa5bdc (6) | CBaseDoor::TraceAttack (glass shards) | debris/bust*.wav | 0.5 |
| 0xa6ea8–0xa741b (6) | CBaseDoor GoUp/HitTop/GoDown/HitBottom | VAR | 0.5 |
| 0xa83d3 | CMomentaryDoor::Use | VAR | 0.5 |
| 0xadcc9 | CMessage::Use | VAR | 0.5 |
| 0xae0fd | CItemSoda::CanThink | weapons/g_bounce3.wav | 0.5 |
| 0xb0810 | CBreakable::MaterialSoundRandom | array | 0.5 |
| 0xb0c33 | CBreakable::DamageSound | array | 0.5 |
| 0xb12ca | CBreakable::TraceAttack | buttons/spark*.wav | 0.5 |
| 0xb1edf | CPushable::Move | array | 0.5 |
| 0xb3ec8/0xb3f23 | CFuncTank Start/StopRotSound | VAR | 0.5 |
| 0xb709f–0xb72b0 (4) | CRecharge Use/Off | items/suitcharge*.wav | 0.5 |
| 0xb8a1a | CHealthKit::MyTouch | items/smallmedkit1.wav | 0.5 |
| 0xb8dc8–0xb8fdc (5) | CWallHealth Use/Recharge/Off | items/med*.wav | 0.5 |
| 0xbb13d | CItem::Materialize | items/suitchargeok1.wav | 0.5 |
| 0xbb7ca | CItemBattery::MyTouch | items/gunpickup2.wav | 0.5 |
| 0xc2b5b–0xc2f07 (7) | CFuncPlat GoDown/HitBottom/GoUp/HitTop/Blocked | VAR | 0.5 |
| 0xc35a4–0xc3a71 (9) | CFuncTrain Use/Wait/Next | VAR | 0.5 |
| 0xc46a2/0xc476e/0xc47d0 | CFuncTrackTrain Stop/UpdateSound | plats/ttrain*.wav | 0.5 |
| 0xc69ae | CFuncTrackChange::Use | buttons/button11.wav | 0.5 |
| 0xc8507 | CBasePlayer::Pain | player/pl_pain*.wav | 0.5 |
| 0xc873b | CBasePlayer::DeathSound | player/pl_pain7/death*.wav | 0.5 |
| 0xc9109/0xc93ac | CBasePlayer::WaterMove | pl_wade/pl_swim | 0.5 |
| 0xc9a52/0xc9cf8 | CBasePlayer::PlayerUse | train_use1/wpn_select | 0.5 |
| 0xcb1b6 | CSprayCan::Spawn | player/sprayer.wav | 0.5 |
| 0xd0496/0xd0541/0xd05ce/0xd066c | SENTENCEG_PlayRndI/PlayRndSz/PlaySequentialSz/Stop | VAR | 0.5 |
| 0xd0c34 | EMIT_SOUND_SUIT | VAR | 0.5 |
| 0xd7896 | CBaseTrigger::ActivateMultiTrigger | VAR | 0.5 |
| 0xe18c0 | CBasePlayerItem::FallThink | items/weapondrop1.wav | 0.5 |
| 0xe1935 | CBasePlayerItem::Materialize | items/suitchargeok1.wav | 0.5 |
| 0xe1bf6 | CBasePlayerItem::DefaultTouch | items/gunpickup2.wav | 0.5 |
| 0xe2147/0xe21ce | CBasePlayerWeapon Add(Primary/Secondary)Ammo | items/9mmclip1.wav | 0.5 |
| 0xe235b | CBasePlayerWeapon::PlayEmptySound | weapons/357_cock1.wav | 0.5 |
| 0xe2541 | CBasePlayerAmmo::Materialize | items/suitchargeok1.wav | 0.5 |
| 0xe2d16 | CWeaponBox::Touch | items/gunpickup2.wav | 0.5 |
| 0xe5e55/0xe6215 | CXenTree Attack/HandleAnimEvent | array | 0.5 |

---

## 4. Fix spec (our tree)

### 4.1 The overload itself — a defaulted 8th parameter (3 files)

**`dlls/util.h:534`** — change the declaration to:
```cpp
void EMIT_SOUND_DYN(edict_t* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch, float flSlowWeight = 0.5f);
```
(One function, defaulted — NOT a second overload. This reproduces the binary
exactly: 149/156 sites incl. all untouched stock-SDK code compile to 0.5.
`EMIT_SOUND` / `STOP_SOUND` inlines at util.h:539/545 need **no change** —
they forward and inherit the default, matching the census. `PLAY_DISTANCE`
etc. unaffected; there is no SDK macro named EMIT_SOUND_DYN to collide with —
the only other spelling is `EMIT_SOUND_DYN2` (enginecallback.h:50, the raw
engine macro), which is what the implementation itself calls.)

**`dlls/sound.cpp:1413`** — the implementation (no default repeated here):
```cpp
void EMIT_SOUND_DYN(edict_t* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch, float flSlowWeight)
{
	// TS 8-arg overload (RE @0xd0a8c): pitch follows the EMITTING entity's timescale
	// (pev->fuser1). scale = (1-w) + w*fuser1; fuser1==0 (never set) => scale 1.0;
	// at fuser1==1 (normal time) scale==1.0 for every w. TRUNCATING int cast.
	float scale = 1.0f;
	if (entity->v.fuser1 != 0.0f)
		scale = (1.0f - flSlowWeight) + flSlowWeight * entity->v.fuser1;
	pitch = (int)(scale * pitch);
	/* ...existing '!'-sentence branch + EMIT_SOUND_DYN2 calls unchanged,
	   now passing the scaled pitch... */
}
```
(Keep the two `EMIT_SOUND_DYN2(...)` forwards as they are — just make them use
the scaled `pitch`. Do the scale ONCE at the top, before the sentence branch,
exactly like the original.)

**`cl_dll/hl/hl_baseentity.cpp:33`** — the client link-stub must match the new
signature or `client.so` fails to link (it includes `dlls/util.h`, and shared
`dlls/ts_gun.cpp` code references the function even in guarded-out paths):
```cpp
void EMIT_SOUND_DYN(edict_t* entity, int channel, const char* sample, float volume, float attenuation, int flags, int pitch, float flSlowWeight) {}
```
No client behaviour change — the stub stays empty; client-local sound pitch is
the separate ev_ts.cpp model (§2). `EMIT_SOUND_PREDICTED` (sound.cpp:1428):
**no change** — its else-branch forwards to EMIT_SOUND_DYN and inherits the
0.5 default; its `PM_PlaySound` skip-branch is a pre-existing prediction
divergence (the original had no such function — its PlayEmptySound @0xe235b is
a plain 8-arg call), noted, out of scope.

### 4.2 Explicit non-default sites (the only per-site arg edits)

**`dlls/func_break.cpp` — `CBreakable::Die` (orig 0x653db–0x656c4):**
1. Immediately BEFORE the material-switch bust sound (i.e. before line ~660),
   add the original's attacker-timescale copy (RE @0x653db):
   ```cpp
   // RE Die @0x653db: the bust sound deepens per the BREAKER's timescale — copy it
   // onto the breakable before emitting (weight 0.9 below).
   if (g_BreakableLastpevAttacker)
       pev->fuser1 = g_BreakableLastpevAttacker->fuser1;
   ```
   (`g_BreakableLastpevAttacker` already exists — func_break.cpp:34, set/cleared
   at 897/899 around the Killed→Die window.)
2. Add `, 0.9f` to ALL ELEVEN bust `EMIT_SOUND_DYN` calls in Die — lines
   673, 676, 686, 689, 700, 703, 713, 716, 727, 730, 737 (bust*1+bust*2 ×5
   materials + bustceiling).
3. `CBreakable::Spawn`: add `pev->fuser1 = 1.0f;` (RE @0x65118). Without it a
   GoSlow'd/attacker-copied path is fine, but parity says init it; audibly
   neutral either way (0 and 1 both yield scale 1.0).
4. `DamageSound` (line 473), `MaterialSoundRandom`, `CBreakable::TraceAttack`
   spark: **no edit** (0.5 default).

**`dlls/player.cpp` — `CBasePlayer::TraceAttack` pain voice (orig 0x80828,
w = 0.15):** this block is NOT implemented yet (traceattack_followups.md D10,
work-listed). When D10 lands, the call must be:
```cpp
EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, psz /* pain2/3/4 */, 1.0, 1.25, 0, 100, 0.15f);
```
→ **update the D10 spec line**: add the trailing `0.15f` and fix its const
typo (0.15f @0xF7F10, not 0xF6F10). The D9 body-hit/headshot/kevlar sounds in
the same function are 0.5 = default → no explicit arg.

### 4.3 Remove the ad-hoc server-side slow-pitch emulations (now superseded)

These were interim stand-ins invented before this overload was RE'd. With the
overload landed they DOUBLE-scale (ad-hoc pitch × fuser1 scaling) and must be
reconciled:

- **`dlls/ts_grenades.cpp:657`** (`CTSKnife::BounceTouch` world tail,
  orig 0x6e9a4): original is `vol 1.0, attn 0.8, flags 0, pitch 100, w 0.5`.
  Replace `KnifeHitPitch(this)` with plain `100` (the deepening now comes from
  the overload + the KNIFE edict's own fuser1, not the thrower's perceived
  slow). Update the line comment.
- **`dlls/ts_grenades.cpp:632, 643`** (orig 0x6e4f1/0x6e6a2): drop the
  `* KnifeHitPitch(this) / 100` factor, keep the `100 + RANDOM_FLOAT(…)` base
  (that matches the original's fp-computed pitch); w = default.
- **`KnifeHitPitch()` (ts_grenades.cpp:556)** then has no callers → delete.
  NOTE the model shift: the original keys off the knife ENTITY's fuser1 (init
  1.0 at Spawn @0x6dca4, slowed via `CBaseEntity::GoSlow`), not the thrower's
  live slow factor. Add `pev->fuser1 = 1.0f;` to our `CTSKnife::Spawn` (line
  ~475) and `CTSShrapnel::Spawn` (line ~154) — both missing, both in the
  original (§1 writers). If the thrower-slow inheritance is wanted for
  knives (as ts_grenades.cpp:324 does for grenades), that's a separate
  GoSlow-side RE — do NOT re-add a play-time thrower read here.
- **`dlls/ts_bullet.cpp:258`** (ric via `TSRecordedSlowPitch(owner)`): **no
  original counterpart** — the census has NO bullet-manager/ric site (the
  original ric is client-side event playback). To keep its current audible
  behaviour once the default lands, pass an explicit `0.0f` weight (scale ≡ 1;
  the ad-hoc pitch already encodes the slow) + a comment. Flag for a later RE
  pass of whether the server should emit this at all.
- **`TSSlowPitch`/`TSRecordedSlowPitch` (ts_gun.cpp:85/94)**: keep — still the
  correct CLIENT-model helpers (ev_ts.cpp) and ts_bullet's interim use; they
  are no longer used to fake the SERVER slow-pitch anywhere after the above.

### 4.4 Comment/spec cleanups
- `dlls/ts_gun.cpp:2218` — resolve `(VERIFY L12)`: verified, w = 0.5 = default,
  no explicit arg needed; drop the VERIFY.
- `dlls/TS_gamerules.cpp:260` — note stands (0.5 = default; no edit needed).
- `traceattack_followups.md` D14 — annotate with the §1 correction (weight vs
  floor swapped) if that dossier is revised.

### 4.5 Known gaps this fix does NOT close (note, don't paper over)
- **`CBaseEntity::GoSlow` virtual isn't built** (TS_gamerulesclass.cpp:775 —
  only CBasePlayer has GoSlow). Consequence: non-player entities (breakables,
  knives, doors…) never get fuser1 < 1 from slow-zones/globalslow, so their
  sounds won't deepen under global slow even with this overload in — except
  Die's attacker-copy path (§4.2), which works immediately. Closing that is
  the GoSlow-RE work item, not this one.
- **`DefaultDeploy` draw sound missing**: the original plays
  `weapons/draw.wav` (CHAN_WEAPON, vol RANDOM_FLOAT(0.92,1.0), pitch
  98+RANDOM_LONG(0,3), w default) inside `CBasePlayerWeapon::DefaultDeploy`
  @0x903be; our `dlls/weapons.cpp:806` DefaultDeploy plays nothing (the wav is
  only precached, TS_util.cpp:107). Future site — default weight when added.
- Our knife/shrapnel Spawn fuser1 inits (§4.3) are the only missing Spawn
  writers; grenades/worldguns/flag/player are already initialized
  (ts_grenades.cpp:34, ts_worldgun.cpp:56/77, ts_flag.cpp:65, ts_player.cpp:667).

### 4.6 Build notes
- Signature change only (defaulted param) → no new files; all four build
  source lists unaffected. Both `hl.so` AND `client.so` must rebuild
  (`scripts/build.sh amd64 all`) — the stub signature change is client-side.
- No wire/protocol impact: the scaling is applied before the ordinary
  `pfnEmitSound`, so stock-client compatibility is automatic.

## 5. Verification protocol (user playtest)
1. Normal time: NO pitch change anywhere (scale is algebraically 1.0 at
   fuser1 1.0 — regression-check gunpickup/door/weapon sounds sound identical).
2. Enter slow-mo (fuser1 ≈ 0.35), get hit: body-hit thunk noticeably deeper
   (×~0.675); pain voice only slightly deeper (×~0.90).
3. In slow-mo, break a crate/glass you shot: bust sound MUCH deeper (×~0.415 —
   the 0.9 weight + Die's attacker-fuser1 copy).
4. Slow-mo melee/knife-throw impacts deepen ×~0.675 (and no double-deepening
   vs before — the KnifeHitPitch removal).
5. A second, non-slowed player nearby hears the same deepened pitches (server
   scaling, not client-local).
