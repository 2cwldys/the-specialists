# cont.96 — RE dossier: (A) bloody blade at idle · (B) glass-shard integrator

All client addresses are `client.dll` RVAs (Ghidra VA = 0x10000000+RVA; file offset = RVA —
paddr==RVA for every section). Server addresses are `ts_i386.so` nm addrs (Ghidra = +0x10000).
Live reads were taken from the running original (`hl.exe` pid 22494, client.dll base 0x0c070000).
NB: everything at VA ≥ 0x10099000 is BSS (raw .data ends at file 0x99000) — the weapon table and
all `DAT_10x9…/10x18…` globals are runtime-filled; values quoted below were read from LIVE memory.

---

## A. BLOODY BLADE AT IDLE — root cause: the `SVC_WEAPONANIM` body byte we never send

### A.0 TL;DR

The viewmodel's BODY submodel (0 = clean, 1 = bloody blade) is set ONLY by weapon-anim calls —
there is **no per-frame body refresh** in the original client (cont84's `DAT_10191d5c` recompute
runs only on viewmodel-MODEL change, and even that is attach-only). The idle repaint chain:

- The original **server** `CTSGun::WeaponIdle` (@0x754dc) calls
  `SendWeaponAnim(animblock[0], skiplocal=0, pev->body)` every ~3 s for any weapon with
  `item->clip != 0` (blades have `iClip = 1`).
- The original `CBasePlayerWeapon::SendWeaponAnim(int iAnim, int skiplocal, int body)`
  (@0x900d0, decompile-exact) does:
  ```c
  m_pPlayer->pev->weaponanim = iAnim;
  if (skiplocal && (*g_engfuncs.pfnCanSkipPlayer)(player->edict)) return;
  MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM /*0x23*/, 0, player->edict);
  WRITE_BYTE(iAnim);
  WRITE_BYTE(this->pev->body);        // ← the wire body is ALWAYS the weapon's pev->body;
  MESSAGE_END();                       //   the 3rd (body) PARAM is dead/ignored
  ```
  With `skiplocal = 0` the message is **sent even to predicting (cl_lw 1) clients**. Server
  `pev->body` is 1 after a flesh hit (KatanaFire @0x733f0 / KnifeFire @0x74044: `item+0x08 = 1;
  pev->body = 1` — cont81), so **every idle replay from the original server repaints the
  viewmodel body to 1**. That is the channel. The client's own predicted idle repaint
  (attach-only body = 0, see A.3) does fire at the timer crossing, but the server's unskipped
  message lands right behind it and re-asserts body 1 (imperceptible on a listen server).
- **OUR server**: `CBasePlayerWeapon::SendWeaponAnim` (`dlls/weapons.cpp:693-708`) computes
  `skiplocal = !m_ForceSendAnimations && UseDecrement()` → for predicting clients the message is
  **skipped**. The original client is left with only its own predicted idle repaint —
  `HUD_SendWeaponAnim(idleAnim, client pev->body)` where client `pev->body` is recomputed from
  the ATTACH mask only (A.1/A.3) = 0 for blades → the blade goes **clean at each idle-timer
  crossing** (re-bloodied by the next swing event = the observed "blink").
- **OUR client** additionally has an invented per-frame overwrite
  (`cl_dll/view.cpp:733-734  view->curstate.body = TS_PredictedViewmodelBody()` — attach-only,
  no bloody term) which erases ANY anim-set bloody body one frame later. The original has no
  such per-frame writer (verified: the fake-gun pev global `DAT_1009f23c` is referenced at
  exactly TWO sites, 0x3a92 / 0x3e93, both inside `HUD_WeaponsPostThink` FUN_10003830; the
  object global `DAT_1009f238` at 7 sites, all enumerated below — none copies body to the view
  entity per frame).

### A.1 Q1 — the per-weapon attach-body table rows (blades): NO bit maps to bloody

Client `GetBodyByItems` = FUN_10007110 (decompile-exact) == server `GetBodyByItems__Fii`
@0x6f0a4, byte-for-byte the same algorithm:

```c
body = 0;
if (attach & 1)                       body += (short)row[+0x128];      // silencer
if ((attach & 6) == 6 && row[+0x130] != -1)  body += row[+0x130];      // laser+flash combo
else { if (attach & 2) body += row[+0x12a];                            // laser
       if (attach & 4) body += row[+0x12c]; }                          // flashlight
if (attach & 8)                       body += row[+0x12e];             // scope
else if (type == 0x24 /*36 contender*/) body += 1;
```

Live-read rows (client table base `DAT_100aa050`, stride 0x3dc, shorts at row+0x128..0x130), and
identical in the server binary (`Write_katana` @0x7727c stores: +0x128..+0x12e = 0, +0x130 = -1):

| weapon | row RVA | +0x128 | +0x12a | +0x12c | +0x12e | +0x130 |
|---|---|---|---|---|---|---|
| knife (25) | 0xb00cc | 0 | 0 | 0 | 0 | −1 |
| katana (34) | 0xb2388 | 0 | 0 | 0 | 0 | −1 |
| sealknife (35) | 0xb2764 | 0 | 0 | 0 | 0 | −1 |

**`GetBodyByItems(blade, anyMask) == 0` always** — the attach channel can never produce the
bloody body. (Server hit branches store only `item+0x08 = 1` and `pev->body = 1`; no store to
`item+0x10` attach — cont81 katana/knife analyses; moot anyway given the zero rows.)

### A.2 Q2 — every writer/reader of the fake gun's `pev->body` (`DAT_1009f23c + 0x114`)

Object `DAT_1009f238` (fake `weapon_tsgun`, vtable **0x1007a6b4**, installed by the stub @0x34f0;
`obj+4` = fake entvars ptr `DAT_1009f23c`; `pev+0x114` = body). Constructed in FUN_100036f0 via
`FUN_10003550(&fakePlayer 0x1009e628, 0)` then `FUN_10003550(&fakeWeapon, fakePlayer)`;
class-init FUN_100083f0 (classname "weapon_tsgun", m_iId obj+0x78 = 0x10).

Writers of `pev->body`:
1. **HUD_WeaponsPostThink** (FUN_10003830), m_iId==0x10 branch, every predicted frame:
   `item->attach(+0x10) = ftol(cd->vuser3.z /*from+0x320*/);`
   `pev->body = FUN_10007110(type, item->attach);` → **attach-only, = 0 for blades.**
2. **Deploy** (vtbl+0x120 = FUN_10008720): `item+0x18 = GetBodyByItems(type, item->attach);
   pev->body = item+0x18;` and **`item+0x08 = 0`** (bloody cleared on every deploy) and
   `item+0x2c = 0`; anim block ptr `item+0x28 = row+0x252`; fires draw.sc locally.

Readers of `pev->body`:
1. The **weaponanim replay** in FUN_10003830 (runfuncs only):
   `if (HUD_GetWeaponAnim() != to->cd.weaponanim /*cd+0x64*/) HUD_SendWeaponAnim(anim,
   m_iId==0x10 && item ? pev->body : 2, force=1)` — fires whenever the SERVER-fed (or predicted)
   `pev->weaponanim` differs from the last engine-called anim.
2. The client **SendWeaponAnim** (vtbl+0x16c = FUN_10003680):
   `player->pev->weaponanim(+0x218) = anim; HUD_SendWeaponAnim(anim, this->pev->body, 0);`
   — called by the predicted **WeaponIdle** (vtbl+0x184 = FUN_1000a9a0) idle branch and the
   knife-throw branch.

The **swing events** don't touch `pev->body`: katana.sc handler FUN_10044230 keeps the local
global `DAT_10189e08` (= `args->iparam2`; forced 1 on a predicted 'F'-flesh trace hit) and plays
the swing viewmodel anim with body = `DAT_10189e08` directly (the only 3 refs to that global are
0x44561/0x445a6/0x446ca, all inside 0x44230..0x446fc). The predicted fire fns pass `item+0x08` as
the event's iparam2 (KnifeFire client FUN_1000a270; KatanaFire client FUN_10009ee0), and
`item+0x08` is restored from **`cd->vuser2.x`** each postthink frame (our server already ships
that — `dlls/client.cpp:2177`).

Vtable map used (read from file .rdata @0x7a6b4, confirmed live): +0x120 Deploy=0x8720,
+0x128 Holster=0xaaf0, +0x134 ItemPostFrame=0xb7b0, +0x16c SendWeaponAnim=0x3680,
+0x174 PrimaryAttack=0x89a0, +0x178 SecondaryAttack=0x88e0, +0x17c third=0x8aa0,
+0x180 Reload=0xa720, +0x184 WeaponIdle=0xa9a0.

### A.3 Q3 — the weaponanim replay/idle body source, and why the original stays bloody

Client **WeaponIdle** FUN_1000a9a0 (== server @0x754dc, disasm-verified identical):
```c
m_iClip = item->clip;
item->animblock(+0x28) = holdPosition(item+0x2c) ? row+0x28c : row+0x252;
if (fireType(row+0x348)==1 && item->phase(+0x08)) { Reload(); return; }     // pump machine
if (item->mode(+0x02)==2 && nextPrimary<=0) { burst continue via item+0x08 }
if (m_flTimeWeaponIdle(+0x98) <= UTIL_WeaponTimeBase()) {
    if (fireType==2) { GrenadeIdle(0); return; }
    if (!InfiniteClips(player) && item->clip < 1) { Reload(); return; }
    if (item->clip != 0) {
        m_flTimeWeaponIdle = time + 3.0;                                     // dbl @0x7a8e8
        SendWeaponAnim(animblock[0] /*idle seq*/, [server: skiplocal=0,] pev->body);
    }
}
```
Blades: fireType knife/sealknife=3, katana=10; `iClip` (row+4) = **1** for all three (knife's
clip counts throwing knives) → the idle branch RUNS. Idle seq = animblock[0] = 0. Both sides
transmit real timers (server `GetWeaponData` @0xa00ac fills wd.m_fTimeWeaponIdle etc., countdown
style, `UTIL_WeaponTimeBase` @0xdbb80 returns a global = 0.0), so the client predicts the same
crossing.

So at each ~3 s idle crossing BOTH repaints happen:
- client predicted: `HUD_SendWeaponAnim(0, client pev->body = 0)` → clean;
- **original server**: unskipped `SVC_WEAPONANIM(0, server pev->body = 1)` → bloody again.

Original server ⇒ net bloody. Our server (message skipped) ⇒ net clean = the bug. The wire
`cd->weaponanim` itself is NOT the carrier (its replay uses the CLIENT's attach-only body);
**the carrier is the SVC_WEAPONANIM body byte.**

### A.4 Fix spec (apply without re-RE)

1. **Server — force-send TS weapon anims (the actual fix).** The original passes `skiplocal=0`
   in `WeaponIdle` (@0x75646: `push pev->body; push $0; push anim`) and in `Deploy`
   (`DefaultDeploy(vm, pm, drawSeq, ext, 0, pev->body)` — cont89 §3). Our lever already exists:
   `m_ForceSendAnimations` (`dlls/weapons.h:286`, honored at `dlls/weapons.cpp:695`). In
   `CTSGun::WeaponIdle` (`dlls/ts_gun.cpp:2825` region) wrap the idle send:
   ```cpp
   m_ForceSendAnimations = true;                       // original: skiplocal = 0
   SendWeaponAnim(idleAnim, pev->body);
   m_ForceSendAnimations = false;
   ```
   and do the same around the deploy-anim send in `CTSGun::Deploy` (its DefaultDeploy path).
   EXCEPTION — keep the knife-THROW anim skipped: the original calls it with `skiplocal=1`
   (client FUN_1000a270 line: `vcall+0x16c(throwAnim, 1, 0)`); our throw site should NOT force.
   The wire body is already correct (`weapons.cpp:706` writes `pev->body`, and our
   KnifeFire/KatanaFire already set `pev->body = 1` on flesh hits — `ts_gun.cpp:1371/2124`).
   // VERIFY (LOW): other CTSGun server SendWeaponAnim sites (reload anims etc.) — the original's
   // reload/pump anims are event/predicted-driven; if any of ours also SendWeaponAnim, mirror
   // per-site skiplocal by checking that site's vcall args in the client decompile first.
2. **Our client — remove the invented per-frame body overwrite.** `cl_dll/view.cpp:733-734`
   (`view->curstate.body = TS_PredictedViewmodelBody()` every frame) has no original counterpart
   — the original recomputes the viewmodel body ONLY when the viewmodel MODEL changes
   (FUN_10068f70, `DAT_10191d5c`, cont84 §5) and otherwise leaves the body to anim calls. Gate
   it the same way: keep the last body; recompute (attach-only) only when
   `view->curstate.modelindex`/viewmodel changes (the deploy case that motivated it — cont89 §3
   deploy-flicker — stays fixed because EV_TSDraw + the model-change recompute cover the deploy
   frame). Without this, the server's body=1 repaint (fix #1) is erased one frame later on OUR
   client.
3. **Our client — keep** `hl_weapons.cpp:643` (`it.bloody = vuser2.x`) and the predicted-idle
   send `hl_weapons.cpp:835` (`HUD_SendWeaponAnim(to->client.weaponanim, pWeapon->pev->body,…)`).
   The cont96 seed `g_TSGun.pev->body = it.bloody` (`hl_weapons.cpp:650`) is NOT what the
   original does (original client pev->body never carries bloody) — with #1+#2 in place it is
   redundant; drop it for strict parity, or keep it consciously as a latency-hiding divergence
   (it pre-empts the clean flash between the predicted repaint and the server message on
   high-ping links). Recommend: drop (parity law), note the flash is original behavior.
4. **No change** to the vuser3.z attach channel, the events, or the weapon table — A.1 proves
   attach can never encode bloody.

Both-directions check (drop-in): original client + our server = bloody stays (server message);
our client + original server = bloody stays (needs #2, else our per-frame overwrite erases the
original server's repaint — this pairing is broken TODAY for the same reason).

---

## B. GLASS SHARD SYSTEM — the exact integrator

### B.0 System layout

A custom client particle system, singleton at `DAT_1018bde8` (NOT a list head — the system
object): `+0` count?, `+4` pool size (init `FUN_100507e0(sys, DAT_1018ca5c=2048)`, clamped
[0x100..0x1000]), `+8` pool ptr (`new[size*300]`), `+0xc..` 64 sprite-registry entries of 0x28
bytes each (`+0x0` name…, `+0x20` model ptr, `+0x24` render mode short, `+0x26` frame-count
short), `+0xc0c` first-free hint, `+0xc10` high-water index, `+0xc14/+0xc18` push-list.
Per-frame entry: **FUN_10056dd0** (GL frame hook) → `FUN_100520d0(&sys)` = update+draw.
Alloc `FUN_1004ffe0` (returns a zeroed 300-byte staging buffer `DAT_1018a720`), commit
`FUN_10050b60(sys, p, cb1=0, cb2=0)` (copy into a free slot + init), free `FUN_10050b20`.
Sprite registration `FUN_100509d0(sys, "sprites/glassfrag.spr", mode=5 /*kRenderTransAdd*/)`;
`FUN_100509a0(sys, idx)` = RandomLong(0, frames−1) (random start frame).

Particle struct (300 = 0x12c bytes; dword index [n] = byte n*4):

| off | field | glass value |
|---|---|---|
| +0 b | active | 1 |
| +1 b | **frozen** (settled; physics/spin/collide skipped) | 0 |
| +2 b | visible (spriteIdx≥0, then PVS-checked in collide) | — |
| +4 b | type: 3 = world-colliding, 4 = precomputed quad | **3** |
| +5 b | spin mode: 1 = billboard roll | **1** |
| +6 b | attached player idx+1 | 0 |
| +7 b | attach mode | 0 |
| +0x08 v | prevpos (commit: = pos) | — |
| +0x14 v | pos | grid point |
| +0x20 v | angles (+0x28 = roll accum) | 0 |
| +0x38 v | **velocity, in units/(10·s)** | see below |
| +0x4c f | roll rate | RandomLong(−90,90) |
| +0x68 v | wander state (commit: = vec3_origin `DAT_10098a28`) | — |
| +0xd0 f | gravity coeff [0x34] | **1.0** |
| +0xd4 f | spawn time (commit: GetClientTime) | — |
| +0xd8 f | life remaining [0x36] | RandomFloat(**2,5**) |
| +0xdc f | brightness/alpha [0x37] (draw color r=g=b) | **0.7** |
| +0xe0 f | post-death fade rate [0x38] | **0.05** |
| +0xe4 f | scale-grow rate [0x39] | 0 |
| +0xe8 f | wander strength [0x3a] | 0 |
| +0xec f | next collision-trace time [0x3b] (commit: 0) | — |
| +0xf0 f | fade-in duration [0x3c] | 0 |
| +0xf4 f | drag [0x3d] (commit floors to ≥ **0.1**) | 0.1 |
| +0xf8 f | sprite frame (float) [0x3e] | random 0..frames−1 |
| +0xfc f | framerate [0x3f] (advances only AFTER death) | 0 |
| +0x100 f | player-push factor [0x40] | 0.5 (gated OFF by default, `DAT_1018ca5a`=0) |
| +0x104 f | wind factor [0x41] | 0 |
| +0x108 f | scale [0x42] | (a+b)·0.7/18, clamp ≤ 20.0 (dbl @0x7cce8) |
| +0x110 f | next env-interaction time [0x44] | — |
| +0x114 s | sprite idx; +0x116 s slot idx | — |
| +0x118 p | per-frame callback(p) [0x46] | 0 |
| +0x11c p | collision callback(p,&tr) [0x47] | 0 |
| +0x120 i | **collision enable** [0x48] (0 = never trace) | **2** |

### B.1 Spawner FUN_10047cb0 (glass shatter; called from the TSFrags handler FUN_1004bd50)

Wire (matches our `MsgFunc_TSFrags` exactly): origin×3, size×3, material BYTE; material 0 →
refPoint×3 (impact), dir×3, dmgType LONG, velMag COORD → FUN_10047cb0(origin, size, 0, dir,
refPoint, dmgType, velMag). (material 1 = explosion debris FUN_100481b0; **material 2 = metal
sparks** `sprites/scintille_metal.spr` FUN_10046f50/FUN_100492b0 — our handler ignores 2. // VERIFY
whether any TS server sends material 2 — the server @0x752e0 side should be checked before adding.)

- Zero the SMALLEST size dim (the pane normal); 9×9 grid: `pos_axis = origin_axis −
  size_axis×0.5 + size_axis×(1/9)×i` (i = 0..8) on the two remaining axes.
- scale [0x42] = (a+b) × (1/18) × 0.7 (dbl @0x7a8e0), clamped ≤ 20.0 — **the original's quad
  half-extent units**, not engine-sprite scale. // VERIFY the exact FUN_10050080 semantics if
  self-drawing; with engine tempents keep the user-tuned equivalent.
- velocity (stored, = world/10):
  - normal: per component `RandomFloat(dir_c×velMag×0.5, dir_c×velMag + 15.0)`;
  - `dmgType & 0x8000002`: x,y = `RandomFloat(−9,9)`, z = `RandomLong(0,10)`, then
    `+= dir × velMag/(|pos−refPoint|+1.0) × 10.0`.
- life = RandomFloat(2,5); roll rate = RandomLong(−90,90); frame = random; commit.

### B.2 Per-frame update FUN_100520d0 + integrator FUN_10050ce0 — the equations

**dt = `DAT_100963e8` (client slow-mo factor, 1.0 normal) × `DAT_10191478` (frametime)**
(FUN_100520c0). THIS is the moon-grav coupling: in slow-mo the whole simulation — fall included —
runs at the slow factor (real-time fall accel ∝ slow²). Gravity reads the LIVE `sv_gravity`
mirror `DAT_100963ec` (800.0 live).

Per live particle, per frame (FUN_100520d0 → FUN_10050ce0), in this order:

```
# FUN_10050ce0, non-attached (+6==0), not frozen (+1==0):
vel.z += dt × grav[+0xd0] × 0.1 (dbl @0x7a540) × (−sv_gravity)        # gravity → velocity
zdamp  = max(1 − |grav|, 0.0)                                          # = 0 for glass
d      = min(1.0, drag[+0xf4] × dt)                                    # drag 0.1/s
vel   -= (vel.x·d, vel.y·d, vel.z·d·zdamp)                             # NO z-drag for glass
if (time > p[+0x110]) FUN_10051270(p)                                  # env interactions (B.4)
pos   += vel × (dt × 10.0)                                             # ★ the ×10 integration
# spin (mode +5 == 1):
roll[+0x28] += rollRate[+0x4c] × dt × 10.0 (dbl @0x7a538)              # deg, ±900°/s max
# collision (type +4 == 3, not frozen): FUN_10051d50 (B.3)
# back in FUN_100520d0:
if (life ≤ 0 && fadeRate[+0xe0] > 0):  alpha[+0xdc] = clamp(alpha − dt×fadeRate×100.0, 0, 1)
if (scaleRate[+0xe4] != 0):            scale[+0x108] += dt × scaleRate × 100.0
if (framerate[+0xfc] != 0 && life < 0.001): frame[+0xf8] += dt × framerate
life[+0xd8] -= dt
free the slot when: life < 0.001 AND anim-finished AND (fadeRate ≤ 0.001 OR alpha < 0.001)
                    AND (scaleRate ≥ −0.001 OR scale < 0.001)
```

Effective world numbers for glass: initial speed = stored×10 u/s; fall accel = 0.1×800×10 =
**full sv_gravity**; horizontal decay ≈ 10 %/s; z undamped; after life (2–5 s) expires the shard
fades 0.7→0 at 5.0 α/s (~0.14 s) and is freed.

Draw (FUN_100520d0): TriAPI, RenderMode = registry mode (**5 = kRenderTransAdd** for glassfrag),
`Color4f(b, b, b, fadeIn)` with b = alpha[+0xdc] (0.7 → gray-additive) and fadeIn = 1 unless
within [+0xf0] s of spawn; quad built by FUN_10050080(pos, angles(roll), scale, vel).

### B.3 World collision FUN_10051d50 (type-3 only) — bounce/settle, distance-throttled

```
if (p[+0x120] == 0) { prevpos = pos; return; }
if (|prevpos − pos|² < 0.001 or !visible) return;
p.visible = FUN_100570b0(&pos);           # PVS check — offscreen shards don't trace
if (time < p[+0xec]) return;              # throttle:
lod      = (FOV > 39) ? 0.5 : 1.0         # DAT_100cf978 = FOV (80 live)
interval = |pos − viewer| × lod × 0.0001220703 (=1/8192); if (dist > 1024) interval ×= 2;
interval = max(interval, 0.016);  p[+0xec] = time + interval;
EV_SetTraceHull(2); EV_PlayerTrace(prevpos, pos, 2, −1, &tr);
if (tr.fraction == 0):                    # spawned/stuck in solid
    (time ≥ spawnTime+0.5) ? freeze() : free_slot()
elif (tr.fraction < 1):                   # HIT
    if (PointContents(prevpos) == CONTENTS_SOLID) free_slot();
    pos = tr.endpos;
    f   = max(0, 1 − interval×7.0);
    vel *= 0.3 (@0x7a8b0) × f;            # ★ bounce damping (≈0.267 at the 16 ms floor)
    vel += tr.plane.normal × (dot(vel, normal) × −2.0);      # standard reflect
    if (slowFactor × |pos − prevpos| < drag[+0xf4](0.1) × 3.0 (@0x7a848)):
        freeze()                          # ★ settle: FUN_10051d10 → frozen=1, vel=0,
                                          #   rollRate=0, roll=0 — lies flat till life+fade
    if (p[+0x11c]) callback(p, &tr);
prevpos = pos;
```

### B.4 Env interactions FUN_10051270 (runs each `max(dt, 8×0.0333)` ≈ 0.27 s) — glass mostly OFF

Three optional behaviors, ALL gated `DAT_1018ca5a` (live **0** — a cvar toggle, default off)
except the first: wander/jitter (`[0x3a]`, glass 0 — skipped), player-push (`[0x40]` = 0.5:
players within 128 u shove shards, scaled ×0.0078125), wind (`[0x41]`, glass 0). Safe to omit
entirely for parity with default settings. `FUN_100524c0`/`FUN_10052720` (called from the tracer
FUN_100450c0) push/scatter particles along bullet paths — also `DAT_1018ca5a`-gated, omit.

### B.5 Fix spec for `cl_dll/ts_debris.cpp` (replace the tempent approximation)

Current state (`ts_debris.cpp:96-125`): engine `R_TempSprite`, vel×10, FTENT_GRAVITY (full g),
bounceFactor 0.2, FTENT_FADEOUT, ±500°/s roll. Deltas to exact:

1. **Self-integrate** (keep spawning via one client-side list walked from `HUD_TempEntUpdate`
   or the redraw hook — a tempent with `flags |= FTENT_CLIENTCUSTOM` + `callback` also works:
   zero out FTENT_GRAVITY/COLLIDEWORLD and do the motion in the callback). Per frame use
   **dt = frametime × g_flClientSlow** (both position AND gravity — that reproduces the
   moon-grav slow-mo automatically and removes the ad-hoc scaling comments at lines 96-106).
2. Motion, per B.2 (keep velocities in the RE'd wire units, ×10 only in the position step):
   `vel.z −= dt × 0.1 × sv_gravity` (read `sv_gravity` cvar; 800 default) · `vel.x/y ×= (1 −
   min(1, 0.1×dt))` (no z drag) · `pos += vel × dt × 10`.
3. Collision per B.3: throttle traces by `max(0.016, dist_to_view/8192 [×2 if >1024])` s
   (`gEngfuncs.pEventAPI` SetTraceHull(2)+PlayerTrace prevpos→pos); on hit `vel ×= 0.3 × max(0,
   1−interval×7)`, reflect −2·dot; **settle-freeze** when `g_flClientSlow × |step| < 0.3` →
   vel=0, spin=0, roll=0, lie still (do NOT kill); kill if stuck-in-solid (fraction 0 within
   0.5 s of spawn) or PointContents(prevpos)==SOLID on hit.
4. Lifetime/fade: life = RandomFloat(2,5) decremented by dt (slow-mo stretches life ✓);
   after death fade alpha 0.7→0 at 5.0/s then remove. Render additive with color
   (0.7, 0.7, 0.7) — our current 1.0 brightness is hotter than the original.
5. Spin: roll += RandomLong(−90,90) × dt × 10 (our ±500°/s becomes exactly ±900 max).
6. Velocity init and 9×9 grid: already exact (`ts_debris.cpp:65-94` matches B.1) — keep,
   but drop the `vel × 10.0f` pre-scale at line 106 if switching to the ×10 position step
   (equivalent; just don't do both).
7. Scale: original quad half-extent = (a+b)×0.7/18 clamp ≤20 in world units. Our sprite-scale
   0.08..0.3 was tuned to LOOK equivalent under engine sprite semantics. If self-drawing quads,
   use the original formula; if keeping engine sprite rendering, keep the tuned mapping.
   // VERIFY visually (FUN_10050080's exact scale→quad mapping was not decompiled).
8. Optional (default-off in the original): player-push/wind/bullet-push (B.4) — skip.

Provenance recap: spawner 0x47cb0 · commit 0x50b60 · update 0x520d0 · integrator 0x50ce0 ·
collide 0x51d50 · freeze 0x51d10 · free 0x50b20 · dt 0x520c0 · frame entry 0x56dd0 ·
TSFrags handler 0x4bd50 · sprite reg 0x509d0. Constants (client.dll .rdata, file==RVA):
0.1 dbl@0x7a540 · 10.0 dbl@0x7a538 · 0.7 dbl@0x7a8e0 · 20.0 dbl@0x7cce8 · 1/18 f@0x7d744 ·
1/9 f@0x7d740 · 0.5 f@0x7a870 · 15.0 f@0x7a890 · 3.0 dbl@0x7a8e8 · 0.3 f@0x7a8b0 ·
−2.0 f@0x7ccd0 · 3.0 f@0x7a848 · 7.0 f@0x7d7dc · 1/8192 f@0x7d7e4 · 0.016 f@0x7d7e0 ·
1024 f@0x7d6d8 · 0.001 f@0x7a884 · 100.0 f@0x7a910.

---

## C. Bonus corrections to earlier dossiers (found in passing)

- cont89 §4 table: **cd+0x6c (`ammo_shells`) is NOT a "queued weaponanim"** — HUD_WeaponsPostThink
  line `item[pair].clip = from+0x1c0` shows it carries the **paired akimbo/single variant's
  clip** (written into `DAT_1009f340 + pairType×0x30`).
- `item+0x0c` (the cont89 "second-clip"/vuser4.z VERIFY): ItemPostFrame FUN_1000b7b0 lines 35-43
  prove it is the **reload-in-progress flag** — on completion `clip = wi->iClip; leftClip =
  clip/2; rightClip = clip − leftClip; item+0x0c = 0`. (Matches the coordinator's m_fInReload
  mapping; the VERIFY can be closed.)
- `item+0x18` = cached attach-body (written by Deploy) — a previously unmapped item field.
- GetBodyByItems quirk both sides: `type == 0x24` (36, contender) gets `body+1` when scope-bit
  is CLEAR (unscoped body offset).
