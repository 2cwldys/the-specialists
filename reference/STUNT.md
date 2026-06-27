# STUNT.md — the CTSStunt system (RE map), cont.70

Complete reverse-engineering of **The Specialists' stunt/acrobatics system** from
`reference/ts_i386.so`: the server-side `CTSStunt` state machine **and** the
client-predicted `pm_shared` movement hooks that make dive/prone/wall moves feel
right (small hull + low view + launch velocity the CLIENT predicts).

This is the map for the faithful reconstruction. Source file in the original:
`TS_stuntmanager.cpp` (server) + the `TS*` additions to `pm_shared.c`.

> Addressing: nm address in `ts_i386.so`; Ghidra address = nm + 0x10000.
> `this` = the `CTSStunt` object; `this+0x40` = `m_pPlayer` (CBasePlayer*).
> `**(this+0x40)` = `m_pPlayer->pev` (entvars_t*, pev is at object offset 0 in the
> gcc2 original). Modern-SDK reconstruction: write members naturally; the offsets
> below are provenance only.

---

## 1. The networked contract: `pev->iuser4` (entvars @ 0x250)

The whole system is driven by one **networked** entvars int, `pev->iuser4`. The
engine copies `pev->iuser1..4 / fuser1..4` into `pmove->iuser1..4 / fuser1..4`
both ways, so `pm_shared` (compiled into BOTH client.dll and the server) reads the
SAME bits the server set → the client PREDICTS the stunt hull/view/velocity. This
is why forcing the hull in the DLL alone never stuck: the prediction owns it.

`pev->iuser4` bit field (a.k.a. the "stunt flags"; byte views at 0x250/0x251/0x252):

| bit | name | meaning | set by | consumed by |
|---|---|---|---|---|
|0x1|STOP|damp velocity ×0.5|HandleWheel/HandleSlide/Landing|pm `TSHandle` (vel×0.5)|
|0x4|PRONE|use ducking hull|GoDive/GoRoll/GoBulletTime/GoWallJump/GoWallClimb|pm `TSGoDuck`|
|0x8|STANDUP|leave ducking hull|(set when prone should end)|pm `TSStandUp`|
|0x10|DIVE|dive in progress|GoDive|Play→HandleDive|
|0x20|SLIDE|slide in progress|GoBulletTime/HandleWheel/Landing|Play→HandleSlide; pm `TSHandle` (vel.x=0, yz×0.3)|
|0x40|DIVE_HIGH|dive height tier|GoDive(CanDiveHigh)|pm `TSDive` (speed 90 / z 50)|
|0x80|DIVE_LOW|dive height tier|GoDive(CanDiveLow)/GoWheel|pm `TSDive` (speed 100 / z 10)|
|0x100|DIVE_MED|dive height tier|GoDive(CanDiveMedium)|pm `TSDive` (speed 90 / z 25)|
|0x400|WHEEL_HI|cartwheel high|GoWheel|Play→HandleWheel (0xc00)|
|0x800|WHEEL_LO|cartwheel low|GoWheel|Play→HandleWheel|
|0x1000..0x4000|WALLJUMP|wall-jump launch (0x7000)|GoWallJump|pm `TSWallJump`|
|0x8000|WALLCLIMB|wall-climb launch (sign bit of low short)|GoWallClimb|pm `TSWallJumpClimb`|
|0x10000|WALLJUMP_ACTIVE|byte 0x252&1|GoWallJump|Play→HandleWallJump|
|0x20000|WALLCLIMB_ACTIVE|byte 0x252&2|GoWallClimb|Play→HandleWallClimb|
|0x40000|ROLL|landing-roll|GoRoll/Landing||
|0x100000|WHEEL_DONE|wheel finished|Play (from 0x200000)||
|0x200000|WHEEL_TRIG|wheel trigger|HandleStunt (wall-dive→wheel)|Play (→0x100000+anim 0x39)|
|0x800000|WALLDIVE|dive faces forward not velocity (byte0x252&0x80)|HandleStunt/HandleWallClimb|GoDive / pm `TSDive`|

Common masks: `0xc30` = dive|slide|wheel active (HandleStunt entry guard);
`0x130000` = walljump/climb active; `0x130c30` = "any stunt running" (GoRoll/GoWheel
guards); `0x70c31` = Play ladder/water full reset; pm `PM_Duck` gate = `0x3CC3C`
(4|8|0x10|0x20|0xc00|0x10000|0x20000) → skip normal ducking while any stunt set.

Also networked: `pev->fuser1` (entvars @0x20c → pmove @0x20c) = the slow-motion
time-scale (PM_PlayerMove scales `frametime` by it when ≥0.1).

---

## 2. CTSStunt member layout (object offsets, provenance only)

| off | type | name (reconstruction) | role |
|---|---|---|---|
|0x00|float|m_flStuntTime|main stunt timer; counts down in Play; `ROUND(10*)` → gmsgTSNextStunt; gates HandleStunt/GoWheel entry|
|0x04|float|m_flRollTime|dive-roll active window; decrements to min 0.001 in Play; while >0 blocks new dive|
|0x08|float|m_flStunCooldown|generic cooldown; counts to 0 in Play; >0 blocks roll/wheel|
|0x0c|float|m_flRollNextTime|earliest next roll (set time+0.3 on a failed roll)|
|0x10|float|m_flWheelNextTime|earliest next wheel (set time+0.3 on a failed wheel)|
|0x14|float|m_flWallClimbNextTime|wall-climb retry cooldown when airborne (CheckForWallClimb, +0.3)|
|0x18|float|m_flCheckBreakNext|CheckForBreakables throttle/sentinel (-1 disables)|
|0x1c|int|m_iField1c|Init = -1|
|0x20|Vector|m_vecStuntDir|stunt/dive direction (normalized velocity, ±v_forward)|
|0x2c|byte|m_bDirForward|GetDirection() != 2 (the move is forward-ish)|
|0x30|float|m_flSlideTime|slide/bullet-time duration timer|
|0x34|float|m_flBulletTimeEnd|gpGlobals->time + 0.55|
|0x38|float|m_flWallClimbFuel|climb budget; += 0.35 (walljump) / 0.3·0.5·0.75 (climb tiers); floored 0.5 → m_flStuntTime|
|0x3c|int|m_iWallClimbCount|# climbs this airtime (reset on ground in Play)|
|0x40|CBasePlayer*|m_pPlayer||

Player/pev fields referenced: `velocity`@0x20, `origin`@0x8, `v_angle`@0x74,
`gravity`@0x120, `friction`@0x11c, `punchangle`@0x174, `button`@0x180
(IN_JUMP=2, IN_ALT1=0x4000), `flags`@0x1a4 (FL_ONGROUND=0x200 → byte0x1a5&2),
`iuser4`@0x250. CBasePlayer: `+0x548` weight/slots (CanDive tiers), `+0x79c/0x7a0/
0x7a4` close-combat dmg/type, `+0x7b9` no-dive-limit override, `+0x814/0x815`
button-edge state (`&0x40` at 0x815 = +alt1 just pressed), `+0x2a0` gait/anim dir,
`+0x798` last close-combat victim, `+0x4dc` cleared on landing. Anim object at
`player+0x28c` (CTSAnimation) — `Animate(seq)` plays a stunt pose.

---

## 3. CTSStunt methods

### Init(player) @0x89fbc
Zero the timers; `m_iField1c = -1`; `m_pPlayer = player`.

### Play() @0x8a014 — the per-frame driver (called every PreThink)
1. `ts = player->GetTimeScale()` (vtable slot 4); `dt = ts * frametime`.
2. Decrement (floored): m_flRollTime(→.001 if active), m_flStuntTime(→0),
   m_flStunCooldown(→0), m_flSlideTime(→0) by `dt`.
3. If alive (`vtbl IsAlive` slot 0xb4) and not frozen (`player+0x244==0`):
   - `iuser4 & WHEEL_TRIG(0x200000)` → set WHEEL_DONE(0x100000), clear it,
     `Animate(0x39)`, `player/airwheel.wav`.
   - if FL_ONGROUND, not on a ladder, watertype!=3:
     - if m_flRollTime just ended (or `pev+0x78` set): clear dive/slide/stop bits,
       m_flRollTime=0, `player+0x4dc=0`, `gravity=1.0`, `view_ofs.z = 28` (or 16 if
       `flags&0x40` alt-wheel). [the "stand back up after a stunt" reset]
     - `iuser4&DIVE(0x10)` → **HandleDive**; `&SLIDE(0x20)` → **HandleSlide**;
       `&0xc00` → **HandleWheel**; byte0x252&1 → **HandleWallJump**; byte0x252&2 →
       **HandleWallClimb**; `button & (IN_JUMP|IN_ALT1)=0x4002` → **HandleStunt**.
   - else (on ladder / in water): if any stunt bit set (`&0x70c31`) clear them all,
     reset view_ofs.z=28 / gravity=1 / friction=1, m_flRollTime=0, player+0x4dc=0,
     anim block reset, `Animate(1)`.

### HandleStunt() @0x8b000 — ground/air move chooser
Guard: `m_flStuntTime<=0 && m_flRollTime<=0 && (iuser4 & 0xc30)==0`. Then reset
close-combat (`player+0x79c=0, +0x7a0=0, +0x7a4=0`). Branch on IN_JUMP:
- **IN_JUMP NOT held** + **+alt1 just pressed** (`player+0x815 & 0x40`):
  `CheckForWallJump()` → GoWallJump; else `CheckForWallDive()` → set WALLDIVE, GoDive.
- **IN_JUMP held**: `CheckForWallClimb()` → GoWallClimb.
- Then if **+alt1 pressed** && `(iuser4 & 0x130000)==0` && (FL_ONGROUND **or**
  (`velocity.z >= -680` && `time - m_flLastStunt <= 0.7/ts`)):
  `speed2d = |velocity.xy|` ≥ 80 → **GoDive**, else **GoBulletTime**.

### GoDive() @0x8b1a0
Pick a dive-height tier by **aim pitch** (`v_angle.x`) gated by CanDive*:
- pitch < -10 && CanDiveHigh → DIVE_HIGH(0x40), friction=0.75
- else pitch ≤ 10 && CanDiveMedium → DIVE_MED(0x100), friction=0.75
- else DIVE_LOW(0x80): if CanDiveLow → gravity=0.25, friction=0.75; else
  gravity=0.9, friction=1.0 (a flop).
Clear height-overwrite bits, set **PRONE(0x4)+DIVE(0x10)**, zero punchangle,
`Animate(0x28)`, m_flStuntTime = NextDive() (=0), send **gmsgTSNextStunt** (byte
`ROUND(10*time)`), maybe **gmsgTSMotionBlur**(idx,2) (RandomLong(0,10)+10*ts > 8),
direction = WALLDIVE? v_forward : normalized velocity, `player/dive.wav` pitch
95+rand(0,31).

### GoRoll(dir) @0x8cfe8  (returns bool; the post-dive LAND roll)
Many disqualifiers (all set local `roll=false`): not airborne unless still in roll
window; speed2d < 80; any stunt running (`&0x130c30`); m_flRollTime>0; on ladder;
m_flStunCooldown>0. If disqualified → m_flRollNextTime = time+0.3, return 0.
Else: dir = normalized velocity; `GetDirection()`; gravity=0.3; clear PRONE-overwrite,
set **ROLL|PRONE|STOP (0x40005)**; zero punchangle; m_flRollTime = 0.75; `Animate(0x2f)`;
`player/land.wav` (or textured footstep if `byte0x252&8`); m_flRollNextTime=0; maybe
gmsgTSMotionBlur (rand+10*ts > 9). Return 1.

### GoWallClimb() @0x8aa1c
dir = −v_forward; clear PRONE-overwrite; set **PRONE|WALLCLIMB(0x8000)|WALLCLIMB_ACTIVE
(0x20000)**, clear walldive; zero punchangle; `Animate(0x38)`; friction=1.0; fuel +=
(CanDiveHigh?0.3 : CanDiveMedium?0.5 : 0.75); m_flStuntTime = max(fuel,0.5); send
gmsgTSNextStunt; `player/airwheel.wav`; m_iWallClimbCount++.

### GoWallJump() @0x8a7d4
dir = −v_forward; set **PRONE|WALLJUMP(0x1000)|WALLJUMP_ACTIVE(0x10000)**, clear
walldive; zero punchangle; `Animate(0x37)`; friction=0.75; fuel += 0.35;
m_flStuntTime = max(fuel,0.5); gmsgTSNextStunt; `player/airwheel.wav`; maybe motionblur.

### GoBulletTime() @0x8a5b4  (the standing "matrix" dodge — only if `ts ≤ 0.99`)
dir = −v_forward (MakeVectors); m_bDirForward=1; m_flStuntTime=0.55;
m_flBulletTimeEnd=time+0.55; gmsgTSNextStunt; set **PRONE|SLIDE(0x20)**; `player+0x2a0
=3` (gait); m_flSlideTime=0.55; zero punchangle; `Animate(0x36)`; `player/land.wav`;
gmsgTSMotionBlur(idx,2).

### GoWheel() @0x8c9bc  (cartwheel; returns bool)
Guards like GoRoll (timers/ladder/`&0x130c30`/recent-stunt window). If qualified:
speed2d≥80 → dir=normalized velocity + GetDirection(); else dir=−v_forward, dir=3.
maybe motionblur. If CanDiveHigh && (dir==3 || aim pitch<-10): set STOP|WHEEL_HI(0x400),
zero punch, `Animate(0x31)`, DIVE_LOW, friction=0.75, m_flStuntTime=NextDive, gmsg,
airwheel; if dir==3 also `UpdateMeleeBar(22.5)`, kungfu-doubled close-combat dmg into
`player+0x79c/0x7a0`. Else CanDiveLow && dir!=3: STOP|WHEEL_LO(0x800), `Animate(0x32)`,
DIVE_LOW, friction=0.75 (gravity 0.25 if dir==2), gmsg, airwheel. Else fail → wheel
cooldown +0.3.

### Handle* (per-frame while a flag is active, only when FL_ONGROUND)
- **HandleDive** @0x8b7e4: `Landing()` then `Animate(2)`.
- **HandleSlide** @0x8b9f8: if grav-flag (flags&0x10) gravity=1; if still sliding
  (`button&0x4006` && cooldown) set STOP, clear SLIDE, build a forward push, friction=1,
  `Animate(0x29)`; else pick slide anim by movement keys (IN_BACK 0x10→0x2a, IN_MOVELEFT
  0x200→0x2b, else 0x3), gravity=1, dir from v_forward, `player+0x2a0=2`.
- **HandleWheel** @0x8b820: 1st frame sets SLIDE + reverses dir, `GetDirection`,
  `player+0x2a0`; later clears WHEEL_LO, sets WALLJUMP_ACTIVE byte, gravity=0.25;
  `land.wav`, friction=1.
- **HandleWallJump** @0x8b4fc: on the ground → clear WALLJUMP_ACTIVE, gravity=0.5,
  zero punch, m_flRollTime=0.262, end stunt timer, `land.wav`, friction=1.
- **HandleWallClimb** @0x8b644: airborne → re-check (CheckForWallJump→GoWallJump /
  CheckForWallDive→GoDive); on ground → clear WALLCLIMB_ACTIVE, gravity=0.5,
  m_flRollTime=0.262, end timer, `land.wav`, friction=1.
- **Landing** @0x8bbf4: end DIVE; if no move keys (`button&0x4006`==0) set SLIDE,
  `GetDirection`; else set ROLL|STOP, gravity=0.5, m_flRollTime=0.5625, `Animate(0x2e)`;
  friction=1, `land.wav`.

### CanDiveHigh/Medium/Low @0x8a494/0x8a4ec/0x8a544
`true` if `player+0x7b9` (no-limit override) else `0x51 - weight(player+0x548) -
SlotBonus < {0x2a, 0x34, 0x3e}` — i.e. lighter loadout unlocks higher dives.
`SlotBonus`@0x8a480 and `NextDive`@0x8a59c both **return 0** in the shipped binary.

### CheckForWallJump/Dive/Climb @0x8ac14/0x8ad44/0x8ae6c
Trace 48u fwd (jump) / 48u back (dive) / 54u fwd (climb) from origin along v_forward;
require a near wall (`trace.flFraction<1`), wall roughly vertical (`|plane.z|<0.5`),
and not falling too fast (`velocity.z > -370`). Climb also has a 0.3s retry throttle
(m_flWallClimbNextTime) and skips non-BSP hits.

### CheckForBreakables @0x8bd9c — the dive/run melee-shove (called every PreThink)
Throttled by m_flCheckBreakNext (-1 disables). Sweeps a hull 64u ahead (32u, or
`UNK` if a flag) along velocity / v_forward; if it hits a damageable entity, applies
melee damage scaled by `0.1*speed` (or `speed` standing-kick), routes through
`VictimKungfuHold/KnifeHold/KatanaHold` (deflect), `DropWeapon`, knockback (velocity
+ z=235), `PunchScreen`, and `TraceAttack`+ApplyMultiDamage. Sounds: `kungfuhit.wav`/
`knife_hitbody.wav`/`katana/hit-body.wav`. This is the "barge through people while
diving" mechanic (glass is handled by the weapon/bullet path, not here).

### CloseAttack @0x8c630
The kung-fu close-combat attack entry (see the cont.69y kung-fu ledger; dispatched
from Jump). Out of scope of the movement reconstruction.

---

## 4. pm_shared TS hooks (client-predicted; in BOTH client.dll + server)

`PM_PlayerMove` @0xf08d8 calls **`TSHandle()`** right before `PM_Duck()`. pmove
offsets: `velocity`@0x5c, `origin`@0x38, `angles`@0x44, `forward`@0x14,
`view_ofs.z`@0x88, `flags`@0xb8 (FL_DUCKING 0x4000), `usehull`@0xbc, `bInDuck`@0x90,
`flDuckTime`@0x8c, `onground`@0xe0 (-1 = airborne), `movetype`@0xdc,
`iuser4`@0x208, `fuser1`@0x20c, `player_mins[hull]`@0x4f4f4 / `player_maxs`@0x4f500.

### TSHandle() @0xe96a0 — pmove dispatcher on `iuser4`
- (on a ladder-ish & airborne) → `PseudoClimb()`.
- `&STOP(1)` → velocity ×0.5.
- `&SLIDE(0x20)` → velocity.x=0, velocity.{y,z} ×0.3.
- `&0x1c0` (any dive-height) → **TSDive()**.
- `&0x7000` (walljump) → **TSWallJump()**.
- sign bit of low short (`&0x8000` wallclimb) → **TSWallJumpClimb()**.
- `&PRONE(4)` → **TSGoDuck()**.
- `&STANDUP(8)` → **TSStandUp()**.

### TSGoDuck() @0xe93a8 — predicted PRONE hull
If `usehull != 1`: set `usehull=1`, `flags|=FL_DUCKING`, and (if airborne) shift
`origin` down by `player_maxs[1]-player_maxs[0]`, `PM_FixPlayerCrouchStuck(1)`,
`PM_CatagorizePosition()`. (Prone uses the **ducking hull**; no extra-low view here —
the low eye comes from Play/StandUp setting `view_ofs.z`.)

### TSStandUp() @0xe91fc — leave the prone hull when there's room
Build stand origin (raise by hull delta if airborne), trace the standing hull; if
clear: `usehull=0`, `view_ofs.z=28`, restore origin, fix/catagorize, clear STANDUP(8).
If blocked: keep `usehull=1` (`view_ofs.z=16`), set FL_DUCKING, clear STANDUP.

### TSDive() @0xe9534 (body @0xf953f) — the dive launch impulse
Speed/zboost by height tier (reads bits, then CLEARS that height bit so it fires once):
- DIVE_MED(0x100): speed 90, zboost √(25·1600)=200
- DIVE_HIGH(0x40): speed 90, zboost √(50·1600)=283
- DIVE_LOW(0x80, default): speed 100, zboost √(10·1600)=126
dir = normalized velocity.xy (or `v_angle forward * 3` if WALLDIVE 0x800000);
velocity.xy += speed·dir; velocity.z = zboost; `onground=-1`; clear WALLDIVE;
`PM_FixupGravityVelocity()`.

### TSWallJump() @0xe9444 / TSWallJumpClimb() @0xe94e0
Set `onground=-1`. WallJump (if a sub-flag): velocity.xy = −angles_fwd·210,
velocity.z=√(value); clear the walljump bit. WallClimb (sign bit): velocity.z=
√112000≈334.7; clear the wallclimb bit. Both `PM_FixupGravityVelocity()`.

### PseudoClimb() @0xe8fc8 — ladder-style wall grab (usehull=2 traces)
When facing a wall within pitch ±60°, trace the point-hull forward; if it can grab,
zero gravity contribution and nudge `velocity.z` up (climb). Restores usehull.

### PM_Duck() @0xe9f7c — gated off during stunts
Reads `iuser4`@0x208; if `& 0x3CC3C` (any stunt) it SKIPS the normal duck logic, so
the stunt system fully owns the hull/view while a stunt is active.

### FixVelocity() @0xe8fc4
No-op in the shipped binary.

---

## 5. Reconstruction plan (modern SDK)

1. **`dlls/ts_stunt.{h,cpp}`** — `CTSStunt` as a real class, member `m_TSStunt` of
   `CBasePlayer`, `Init` in Spawn, `Play()` from `PreThink` (replacing the ad-hoc
   block at `dlls/player.cpp` ~2027-2137). All 25 methods per §3, writing the flag
   bits into `pev->iuser4`. Use natural members (not the offsets). Keep the
   gmsgTSNextStunt / gmsgTSMotionBlur sends + the `player/{dive,land,airwheel}.wav`
   pitches + the `CTSAnimation::Animate(seq)` poses.
2. **`pm_shared/pm_shared.cpp`** — add `TSHandle/TSGoDuck/TSStandUp/TSDive/
   TSWallJump/TSWallJumpClimb/PseudoClimb` (§4) reading `pmove->iuser4`; call
   `TSHandle()` from `PM_PlayerMove` before `PM_Duck`; gate `PM_Duck` on the stunt
   mask. Compiled into BOTH hl.so + client.so → predicted.
3. **Networking** — `iuser4` is stock-networked; nothing extra. `fuser1` slow-mo
   already wired. Verify the client copies `iuser4` into pmove (engine does it).
4. Replace the `m_bDiveRolling/m_flWallClimbFuel/m_bProneSize` approximations and
   their PreThink/PostThink code.

Status: RE complete (this doc). Implementation: in progress (cont.70).
